#include "StarBehaviorState.hpp"
#include "StarRandom.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

// node parameter types supported by the blackboard
List<NodeParameterType> BlackboardTypes = {
  NodeParameterType::Json,
  NodeParameterType::Entity,
  NodeParameterType::Position,
  NodeParameterType::Vec2,
  NodeParameterType::Number,
  NodeParameterType::Bool,
  NodeParameterType::List,
  NodeParameterType::Table,
  NodeParameterType::String
};

Blackboard::Blackboard(LuaTable luaContext) : m_luaContext(move(luaContext)) {
  for (auto type : BlackboardTypes) {
    m_board.set(type, {});
    m_input.set(type, {});
  }
}

void Blackboard::set(NodeParameterType type, String const& key, LuaValue value) {
  if (value.is<LuaNilType>())
    m_board.get(type).remove(key);
  else
    m_board.get(type).set(key, value);

  for (auto& input : m_input.get(type).maybe(key).value({})) {
    m_parameters.get(input.first).set(input.second, value);
  }

  // dumb special case for setting number outputs to vec2 inputs
  if (type == NodeParameterType::Number) {
    for (pair<uint64_t, LuaTable>& input : m_vectorNumberInput.maybe(key).value()) {
      input.second.set(input.first, value);
    }
  }
}

LuaValue Blackboard::get(NodeParameterType type, String const& key) const {
  return m_board.get(type).maybe(key).value(LuaNil);
}

LuaTable Blackboard::parameters(StringMap<NodeParameter> const& parameters, uint64_t nodeId) {
  if (auto table = m_parameters.maybe(nodeId))
    return *table;

  LuaTable table = m_luaContext.engine().createTable();
  for (auto const& p : parameters) {
    if (auto key = p.second.second.maybe<String>()) {
      auto& typeInput = m_input.get(p.second.first);
      if (!typeInput.contains(*key))
        typeInput.add(*key, {});

      typeInput.get(*key).append({nodeId, p.first});
      table.set(p.first, get(p.second.first, *key));
    } else {
      Json value = p.second.second.get<Json>();
      if (value.isNull())
        continue;

      // dumb special case for allowing a vec2 of blackboard number keys
      if (p.second.first == NodeParameterType::Vec2) {
        if (value.type() != Json::Type::Array)
          throw StarException(strf("Vec2 parameter not of array type for key %s", p.first, value));
        JsonArray vector = value.toArray();
        LuaTable luaVector = m_luaContext.engine().createTable();
        for (int i = 0; i < 2; i++) {
          if (vector[i].isType(Json::Type::String)) {
            auto key = vector[i].toString();
            if (!m_vectorNumberInput.contains(key))
              m_vectorNumberInput.add(key, {});

            m_vectorNumberInput[key].append({i+1, luaVector});
            luaVector.set(i+1, get(NodeParameterType::Number, vector[i].toString()));
          } else {
            luaVector.set(i+1, m_luaContext.engine().luaFrom(vector[i]));
          }
        }
        table.set(p.first, luaVector);
        continue;
      }

      table.set(p.first, value);
    }
  }

  m_parameters.add(nodeId, table);

  return table;
}

void Blackboard::setOutput(ActionNode const& node, LuaTable const& output) {
  for (auto p : node.output) {
    auto out = p.second.second;
    if (auto boardKey = out.first) {
      set(p.second.first, *boardKey, output.get<LuaValue>(p.first));

      if (out.second)
        m_ephemeral.add({p.second.first, *boardKey});
    }
  }
}

Set<pair<NodeParameterType, String>> Blackboard::takeEphemerals() {
  return take(m_ephemeral);
}

void Blackboard::clearEphemerals(Set<pair<NodeParameterType, String>> ephemerals) {
  for (auto const& p : ephemerals) {
	  if (!m_ephemeral.contains(p))
	    set(p.first, p.second, LuaNil);
  }
}

DecoratorState::DecoratorState(LuaThread thread) : thread(move(thread)) {
  child = make_shared<NodeState>();
}

CompositeState::CompositeState(size_t childCount) : index() {
  for (size_t i = 0; i < childCount; i++)
    children.append(make_shared<NodeState>());
}

CompositeState::CompositeState(size_t childCount, size_t begin) : CompositeState(childCount) {
  index = begin;
}

BehaviorState::BehaviorState(BehaviorTreeConstPtr tree, LuaTable context, Maybe<BlackboardWeakPtr> blackboard) : m_tree(tree), m_luaContext(move(context)) {
  if (blackboard)
    m_board = *blackboard;
  else
    m_board = make_shared<Blackboard>(m_luaContext);

  LuaFunction require = m_luaContext.get<LuaFunction>("require");
  for (auto script : m_tree->scripts)
    require.invoke(script);

  for (String const& name : m_tree->functions)
    m_functions.set(name, m_luaContext.get<LuaFunction>(name));
}

NodeStatus BehaviorState::run(float dt) {
  m_lastDt = dt;
  NodeStatus status;

  auto ephemeral = m_board.maybe<BlackboardPtr>().apply([](auto const& b) { return b->takeEphemerals(); });

  status = runNode(*m_tree->root, m_rootState);

  if (ephemeral)
    m_board.get<BlackboardPtr>()->clearEphemerals(*ephemeral);

  return status;
}

void BehaviorState::clear() {
  m_rootState.reset();
}

BlackboardWeakPtr BehaviorState::blackboardPtr() {
  if (auto master = m_board.maybe<BlackboardPtr>())
    return weak_ptr<Blackboard>(*master);
  else
    return m_board.get<BlackboardWeakPtr>();
}

BlackboardPtr BehaviorState::board() {
  if (auto master = m_board.maybe<BlackboardPtr>())
    return *master;
  else
    return m_board.get<BlackboardWeakPtr>().lock();
}

LuaThread BehaviorState::nodeLuaThread(String const& funcName) {
  LuaThread thread = m_threads.maybeTakeLast().value(m_luaContext.engine().createThread());
  thread.pushFunction(m_functions.get(funcName));
  return thread;
}

NodeStatus BehaviorState::runNode(BehaviorNode const& node, NodeState& state) {
  NodeStatus status = NodeStatus::Invalid;
  if (node.is<ActionNode>())
    status = runAction(node.get<ActionNode>(), state);
  else if(node.is<DecoratorNode>())
    status = runDecorator(node.get<DecoratorNode>(), state);
  else if(node.is<CompositeNode>())
    status = runComposite(node.get<CompositeNode>(), state);
  else
    StarException("Unidentified behavior node type");

  if (status != NodeStatus::Running)
    state.reset();

  return status;
}

NodeStatus BehaviorState::runAction(ActionNode const& node, NodeState& state) {
  uint64_t id = (uint64_t)&node;

  auto result = ActionReturn(NodeStatus::Invalid, LuaNil);
  if (state.isNothing()) {
    LuaTable parameters = board()->parameters(node.parameters, id);
    LuaThread thread = nodeLuaThread(node.name);
    try {
      result = thread.resume<ActionReturn>(parameters, blackboardPtr(), id, m_lastDt).value(ActionReturn(NodeStatus::Invalid, LuaNil));
    } catch (LuaException e) {
      throw StarException(strf("Lua Exception caught running action node %s in behavior %s: %s", node.name, m_tree->name, outputException(e, false)));
    }

    auto status = get<0>(result);
    if (status != NodeStatus::Success && status != NodeStatus::Failure)
      state.set(ActionState{thread});
  } else {
    LuaThread const& thread = state->get<ActionState>().thread;

    try {
      result = thread.resume<ActionReturn>(m_lastDt).value(ActionReturn(NodeStatus::Invalid, LuaNil));
    } catch (LuaException e) {
      throw StarException(strf("Lua Exception caught resuming action node %s in behavior %s: %s", node.name, m_tree->name, outputException(e, false)));
    }

    auto status = get<0>(result);
    if (status == NodeStatus::Success || status == NodeStatus::Failure)
      m_threads.append(thread);
  }

  if (auto table = get<1>(result).maybe<LuaTable>())
    board()->setOutput(node, *table);

  return get<0>(result);
}

NodeStatus BehaviorState::runDecorator(DecoratorNode const& node, NodeState& state) {
  uint64_t id = (uint64_t)&node;
  NodeStatus status = NodeStatus::Running;
  if (state.isNothing()) {
    auto parameters = board()->parameters(node.parameters, id);

    LuaThread thread = nodeLuaThread(node.name);
    try {
      status = thread.resume<NodeStatus>(parameters, blackboardPtr(), id).value(NodeStatus::Invalid);
    } catch (LuaException e) {
      throw StarException(strf("Lua Exception caught initializing decorator node %s in behavior %s: %s", node.name, m_tree->name, outputException(e, false)));
    }
    if (status == NodeStatus::Success || status == NodeStatus::Failure)
      return status;

    state.set(DecoratorState(thread));
  }

  DecoratorState& decorator = state->get<DecoratorState>();
  // decorator runs its child on yield and is resumed with the child's status on success or failure
  while (status == NodeStatus::Running) {
    auto childStatus = runNode(*node.child, *decorator.child);
    if (childStatus == NodeStatus::Success || childStatus == NodeStatus::Failure) {
      try {
        status = decorator.thread.resume<NodeStatus>(childStatus).value(NodeStatus::Invalid);
      } catch (LuaException e) {
        throw StarException(strf("Lua Exception caught resuming decorator node %s in behavior %s: %s", node.name, m_tree->name, outputException(e, false)));
      }
    } else {
      return NodeStatus::Running;
    }
  }

  m_threads.append(decorator.thread);

  return status;
}

NodeStatus BehaviorState::runComposite(CompositeNode const& node, NodeState& state) {
  NodeStatus status;
  if (node.is<SequenceNode>())
    status =  runSequence(node.get<SequenceNode>(), state);
  else if (node.is<SelectorNode>())
    status = runSelector(node.get<SelectorNode>(), state);
  else if (node.is<ParallelNode>())
    status = runParallel(node.get<ParallelNode>(), state);
  else if (node.is<DynamicNode>())
    status = runDynamic(node.get<DynamicNode>(), state);
  else if (node.is<RandomizeNode>())
    status = runRandomize(node.get<RandomizeNode>(), state);
  else
    throw StarException(strf("Unable to run composite node type with variant type index %s", node.typeIndex()));

  return status;
}

NodeStatus BehaviorState::runSequence(SequenceNode const& node, NodeState& state) {
  if (state.isNothing())
    state.set(CompositeState(node.children.size()));

  CompositeState& composite = state->get<CompositeState>();
  while (composite.index < node.children.size()) {
    auto child = node.children.get(composite.index);
    NodeStatus childStatus = runNode(*child, *composite.children[composite.index]);

    if (childStatus == NodeStatus::Failure || childStatus == NodeStatus::Running)
      return childStatus;
    else
      composite.index++;
  }

  return NodeStatus::Success;
}

NodeStatus BehaviorState::runSelector(SelectorNode const& node, NodeState& state) {
  if (state.isNothing())
    state.set(CompositeState(node.children.size()));

  CompositeState& composite = state->get<CompositeState>();
  while (composite.index < node.children.size()) {
    NodeStatus childStatus = runNode(*node.children[composite.index], *composite.children[composite.index]);

    if (childStatus == NodeStatus::Success || childStatus == NodeStatus::Running)
      return childStatus;
    else
      composite.index++;
  }

  return NodeStatus::Failure;
}

NodeStatus BehaviorState::runParallel(ParallelNode const& node, NodeState& state) {
  if (state.isNothing())
    state.set(CompositeState(node.children.size()));

  CompositeState& composite = state->get<CompositeState>();

  int failed = 0;
  int succeeded = 0;
  for (size_t i = 0; i < node.children.size(); i++) {
    NodeStatus status = runNode(*node.children[i], *composite.children[i]);
    if (status == NodeStatus::Success)
      succeeded++;
    else if (status == NodeStatus::Failure)
      failed++;

    if (succeeded >= node.succeed || failed >= node.fail) {
      return succeeded >= node.succeed ? NodeStatus::Success : NodeStatus::Failure;
    }
  }

  return NodeStatus::Running;
}

NodeStatus BehaviorState::runDynamic(DynamicNode const& node, NodeState& state) {
  if (state.isNothing())
    state.set(CompositeState(node.children.size()));

  CompositeState& composite = state->get<CompositeState>();
  for (size_t i = 0; i <= composite.index; i++) {
    auto child = node.children.get(i);
    auto status = runNode(*child, *composite.children.get(i));
    if (status == NodeStatus::Failure && i == composite.index)
      composite.index++;

    if (i < composite.index && (status == NodeStatus::Success || status == NodeStatus::Running)) {
      composite.children[composite.index]->reset();
      composite.index = i;
    }

    if (status == NodeStatus::Success || composite.index >= node.children.size()) {
      return status;
    }
  }

  return NodeStatus::Running;
}

NodeStatus BehaviorState::runRandomize(RandomizeNode const& node, NodeState& state) {
  if (state.isNothing())
    state.set(CompositeState(node.children.size(), Random::randUInt(node.children.size() - 1)));

  CompositeState& composite = state->get<CompositeState>();
  auto child = node.children.get(composite.index);
  NodeStatus status = runNode(*child, *composite.children[composite.index]);
  return status;
}

}
