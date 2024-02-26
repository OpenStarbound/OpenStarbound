#pragma once

#include "StarBehaviorDatabase.hpp"
#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Blackboard);
STAR_CLASS(BehaviorState);
STAR_STRUCT(ActionState);
STAR_STRUCT(DecoratorState);
STAR_STRUCT(CompositeState);

STAR_EXCEPTION(BehaviorException, StarException);

extern List<NodeParameterType> BlackboardTypes;

class Blackboard {
public:
  Blackboard(LuaTable luaContext);

  LuaValue get(NodeParameterType type, String const& key) const;
  void set(NodeParameterType type, String const& key, LuaValue value);

  LuaTable parameters(StringMap<NodeParameter> const& nodeParameters, uint64_t nodeId);
  void setOutput(ActionNode const& node, LuaTable const& output);

  // takes the set of currently held ephemeral values
  Set<pair<NodeParameterType, String>> takeEphemerals();

  // clears any provided ephemerals that are not currently held
  void clearEphemerals(Set<pair<NodeParameterType, String>> ephemerals);
private:
  LuaTable m_luaContext;

  HashMap<uint64_t, LuaTable> m_parameters;
  HashMap<NodeParameterType, StringMap<LuaValue>> m_board;

  HashMap<NodeParameterType, StringMap<List<pair<uint64_t, String>>>> m_input;
  StringMap<List<pair<uint64_t, LuaTable>>> m_vectorNumberInput;

  Set<pair<NodeParameterType, String>> m_ephemeral;
};

typedef Maybe<Variant<ActionState,DecoratorState,CompositeState>> NodeState;
typedef shared_ptr<NodeState> NodeStatePtr;

typedef pair<LuaFunction, LuaThread> Coroutine;

enum class NodeStatus {
  Invalid,
  Success,
  Failure,
  Running
};

typedef LuaTupleReturn<NodeStatus, LuaValue> ActionReturn;
struct ActionState {
  LuaThread thread;
};

struct DecoratorState {
  DecoratorState(LuaThread thread);
  LuaThread thread;
  NodeStatePtr child;
};

struct CompositeState {
  CompositeState(size_t children);
  CompositeState(size_t children, size_t index);

  size_t index;
  List<NodeStatePtr> children;
};

class BehaviorState {
public:
  BehaviorState(BehaviorTreeConstPtr tree, LuaTable context, Maybe<BlackboardWeakPtr> blackboard = {});

  NodeStatus run(float dt);
  void clear();

  BlackboardWeakPtr blackboardPtr();
private:
  BlackboardPtr board();

  LuaThread nodeLuaThread(String const& funcName);

  NodeStatus runNode(BehaviorNode const& node, NodeState& state);

  NodeStatus runAction(ActionNode const& node, NodeState& state);
  NodeStatus runDecorator(DecoratorNode const& node, NodeState& state);

  NodeStatus runComposite(CompositeNode const& node, NodeState& state);
  NodeStatus runSequence(SequenceNode const& node, NodeState& state);
  NodeStatus runSelector(SelectorNode const& node, NodeState& state);
  NodeStatus runParallel(ParallelNode const& node, NodeState& state);
  NodeStatus runDynamic(DynamicNode const& node, NodeState& state);
  NodeStatus runRandomize(RandomizeNode const& node, NodeState& state);

  BehaviorTreeConstPtr m_tree;
  NodeState m_rootState;

  LuaTable m_luaContext;

  // The blackboard can either be created and owned by this behavior,
  // or a blackboard from another behavior can be used
  Variant<BlackboardPtr, BlackboardWeakPtr> m_board;

  // Keep threads here for recycling
  List<LuaThread> m_threads;
  StringMap<LuaFunction> m_functions;

  float m_lastDt;
};

}
