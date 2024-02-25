#pragma once

#include "StarGameTypes.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_CLASS(BehaviorDatabase);
STAR_STRUCT(ActionNode);
STAR_STRUCT(DecoratorNode);
STAR_STRUCT(SequenceNode);
STAR_STRUCT(SelectorNode);
STAR_STRUCT(ParallelNode);
STAR_STRUCT(DynamicNode);
STAR_STRUCT(RandomizeNode);
STAR_STRUCT(BehaviorTree);

typedef Variant<SequenceNode, SelectorNode, ParallelNode, DynamicNode, RandomizeNode> CompositeNode;

typedef Variant<ActionNode, DecoratorNode, CompositeNode, BehaviorTreeConstPtr> BehaviorNode;
typedef std::shared_ptr<const BehaviorNode> BehaviorNodeConstPtr;

enum class NodeParameterType : uint8_t {
  Json,
  Entity,
  Position,
  Vec2,
  Number,
  Bool,
  List,
  Table,
  String
};
extern EnumMap<NodeParameterType> const NodeParameterTypeNames;

typedef Variant<String, Json> NodeParameterValue;
typedef pair<NodeParameterType, NodeParameterValue> NodeParameter;
typedef pair<NodeParameterType, pair<Maybe<String>, bool>> NodeOutput;

NodeParameterValue nodeParameterValueFromJson(Json const& json);

Json jsonFromNodeParameter(NodeParameter const& parameter);
NodeParameter jsonToNodeParameter(Json const& json);

Json jsonFromNodeOutput(NodeOutput const& output);
NodeOutput jsonToNodeOutput(Json const& json);

enum class BehaviorNodeType : uint16_t {
  Action,
  Decorator,
  Composite,
  Module
};
extern EnumMap<BehaviorNodeType> const BehaviorNodeTypeNames;

enum class CompositeType : uint16_t {
  Sequence,
  Selector,
  Parallel,
  Dynamic,
  Randomize
};
extern EnumMap<CompositeType> const CompositeTypeNames;

// replaces global tags in nodeParameters in place
NodeParameterValue replaceBehaviorTag(NodeParameterValue const& parameter, StringMap<NodeParameterValue> const& treeParameters);
Maybe<String> replaceOutputBehaviorTag(Maybe<String> const& output, StringMap<NodeParameterValue> const& treeParameters);
void applyTreeParameters(StringMap<NodeParameter>& nodeParameters, StringMap<NodeParameterValue> const& treeParameters);

struct ActionNode {
  ActionNode(String name, StringMap<NodeParameter> parameters, StringMap<NodeOutput> output);

  String name;
  StringMap<NodeParameter> parameters;
  StringMap<NodeOutput> output;
};

struct DecoratorNode {
  DecoratorNode(String const& name, StringMap<NodeParameter> parameters, BehaviorNodeConstPtr child);

  String name;
  StringMap<NodeParameter> parameters;
  BehaviorNodeConstPtr child;
};

struct SequenceNode {
  SequenceNode(List<BehaviorNodeConstPtr> children);

  List<BehaviorNodeConstPtr> children;
};

struct SelectorNode {
  SelectorNode(List<BehaviorNodeConstPtr> children);

  List<BehaviorNodeConstPtr> children;
};

struct ParallelNode {
  ParallelNode(StringMap<NodeParameter>, List<BehaviorNodeConstPtr> children);

  int succeed;
  int fail;
  List<BehaviorNodeConstPtr> children;
};

struct DynamicNode {
  DynamicNode(List<BehaviorNodeConstPtr> children);

  List<BehaviorNodeConstPtr> children;
};

struct RandomizeNode {
  RandomizeNode(List<BehaviorNodeConstPtr> children);

  List<BehaviorNodeConstPtr> children;
};

struct BehaviorTree {
  BehaviorTree(String const& name, StringSet scripts, JsonObject const& parameters);

  String name;
  StringSet scripts;
  StringSet functions;
  JsonObject parameters;

  BehaviorNodeConstPtr root;
};

typedef std::shared_ptr<const BehaviorNode> BehaviorNodeConstPtr;

class BehaviorDatabase {
public:
  BehaviorDatabase();

  BehaviorTreeConstPtr behaviorTree(String const& name) const;
  BehaviorTreeConstPtr buildTree(Json const& config, StringMap<NodeParameterValue> const& overrides = {}) const;
  Json behaviorConfig(String const& name) const;

private:
  StringMap<Json> m_configs;
  StringMap<BehaviorTreeConstPtr> m_behaviors;
  StringMap<StringMap<NodeParameter>> m_nodeParameters;
  StringMap<StringMap<NodeOutput>> m_nodeOutput;

  void loadTree(String const& name);

  // constructs node variants
  CompositeNode compositeNode(Json const& config, StringMap<NodeParameter> parameters, StringMap<NodeParameterValue> const& treeParameters, BehaviorTree& tree) const;
  BehaviorNodeConstPtr behaviorNode(Json const& json, StringMap<NodeParameterValue> const& treeParameters, BehaviorTree& tree) const;
};

}
