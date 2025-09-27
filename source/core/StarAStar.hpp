#pragma once

#include <queue>

#include "StarList.hpp"
#include "StarMap.hpp"
#include "StarSet.hpp"
#include "StarLexicalCast.hpp"
#include "StarMathCommon.hpp"
#include "StarBlockAllocator.hpp"

namespace Star {
namespace AStar {

  struct Score {
    Score();

    double gScore;
    double hScore;
    double fScore;
  };

  // 'Edge' should be implemented as a class with public fields compatible with
  // these:
  //    double cost;
  //    Node source;
  //    Node target;

  template <class Edge>
  using Path = List<Edge>;

  template <class Edge, class Node>
  class Search {
  public:
    typedef function<double(Node, Node)> HeuristicFunction;
    typedef function<void(Node, List<Edge>& neighbors)> NeighborFunction;
    typedef function<bool(Node)> GoalFunction;
    typedef function<bool(Node, Node)> CompareFunction;
    typedef function<bool(Edge)> ValidateEndFunction;

    Search(HeuristicFunction heuristicCost,
        NeighborFunction getAdjacent,
        GoalFunction goalReached,
        bool returnBestIfFailed = false,
        // In returnBestIfFailed mode, validateEnd checks the end of the path
        // is valid, e.g. not floating in the air.
        Maybe<ValidateEndFunction> validateEnd = {},
        Maybe<double> maxFScore = {},
        Maybe<unsigned> maxNodesToSearch = {});

    // Start a new exploration, resets result if it was found before.
    void start(Node startNode, Node goalNode);
    // Explore the given number of nodes in the search space.  If
    // maxNodesToSearch is reached, or the search space is exhausted, will
    // return
    // false to signal failure.  On success, will return true.  If the given
    // maxExploreNodes is exhausted before success or failure, will return
    // nothing.
    Maybe<bool> explore(Maybe<unsigned> maxExploreNodes = {});
    // Returns the result if it was found.
    Maybe<Path<Edge>> const& result() const;

    // Convenience, equivalent to calling start, then explore({}) and returns
    // result()
    Maybe<Path<Edge>> const& findPath(Node startNode, Node goalNode);

  private:
    struct ScoredNode {
      bool operator<(ScoredNode const& other) const {
        return score.fScore > other.score.fScore;
      }

      Score score;
      Node node;
    };

    struct NodeMeta {
      Score score;
      Maybe<Edge> cameFrom;
    };

    Path<Edge> reconstructPath(Node currentNode);

    HeuristicFunction m_heuristicCost;
    NeighborFunction m_getAdjacent;
    GoalFunction m_goalReached;
    bool m_returnBestIfFailed;
    Maybe<ValidateEndFunction> m_validateEnd;
    Maybe<double> m_maxFScore;
    Maybe<unsigned> m_maxNodesToSearch;

    Node m_goal;
    Map<Node, NodeMeta, std::less<Node>, BlockAllocator<pair<Node const, NodeMeta>, 1024>> m_nodeMeta;
    std::priority_queue<ScoredNode> m_openQueue;
    Set<Node, std::less<Node>, BlockAllocator<Node, 1024>> m_openSet;
    Set<Node, std::less<Node>, BlockAllocator<Node, 1024>> m_closedSet;
    Maybe<ScoredNode> m_earlyExploration;

    bool m_finished;
    Maybe<Path<Edge>> m_result;
  };

  inline Score::Score() : gScore(highest<double>()), hScore(0), fScore(highest<double>()) {}

  template <class Edge, class Node>
  Search<Edge, Node>::Search(HeuristicFunction heuristicCost,
      NeighborFunction getAdjacent,
      GoalFunction goalReached,
      bool returnBestIfFailed,
      Maybe<ValidateEndFunction> validateEnd,
      Maybe<double> maxFScore,
      Maybe<unsigned> maxNodesToSearch)
    : m_heuristicCost(heuristicCost),
      m_getAdjacent(getAdjacent),
      m_goalReached(goalReached),
      m_returnBestIfFailed(returnBestIfFailed),
      m_validateEnd(validateEnd),
      m_maxFScore(maxFScore),
      m_maxNodesToSearch(maxNodesToSearch) {}

  template <class Edge, class Node>
  void Search<Edge, Node>::start(Node startNode, Node goalNode) {
    m_goal = std::move(goalNode);
    m_nodeMeta.clear();
    m_openQueue = std::priority_queue<ScoredNode>();
    m_openSet.clear();
    m_closedSet.clear();
    m_earlyExploration = {};
    m_finished = false;
    m_result.reset();

    Score startScore;
    startScore.gScore = 0;
    startScore.hScore = m_heuristicCost(startNode, m_goal);
    startScore.fScore = startScore.hScore;
    m_nodeMeta[startNode].score = startScore;

    m_openSet.insert(startNode);
    m_openQueue.push(ScoredNode{startScore, std::move(startNode)});
  }

  template <class Edge, class Node>
  Maybe<bool> Search<Edge, Node>::explore(Maybe<unsigned> maxExploreNodes) {
    if (m_finished)
      return m_result.isValid();

    List<Edge> neighbors;
    while (true) {
      if ((m_maxNodesToSearch && m_closedSet.size() > *m_maxNodesToSearch)
          || (m_openQueue.empty() && !m_earlyExploration)) {
        m_finished = true;
        // Search failed. Either return the path to the closest node to the
        // target,
        // or return nothing.
        if (m_returnBestIfFailed) {
          double bestScore = highest<double>();
          Maybe<Node> bestNode;
          for (Node node : m_closedSet) {
            NodeMeta const& nodeMeta = m_nodeMeta[node];
            if (m_validateEnd && nodeMeta.cameFrom && !(*m_validateEnd)(*nodeMeta.cameFrom))
              continue;
            if (nodeMeta.score.hScore < bestScore) {
              bestScore = nodeMeta.score.hScore;
              bestNode = node;
            }
          }

          if (bestNode)
            m_result = reconstructPath(*bestNode);
        }

        return false;
      }

      if (maxExploreNodes) {
        if (*maxExploreNodes == 0)
          return {};
        --*maxExploreNodes;
      }

      ScoredNode currentScoredNode;
      if (m_earlyExploration) {
        currentScoredNode = m_earlyExploration.take();
      } else {
        currentScoredNode = m_openQueue.top();
        m_openQueue.pop();
        if (!m_openSet.remove(currentScoredNode.node))
          // Duplicate entry in the queue due to this node's score being
          // updated.
          // Just ignore this node; we've already searched it.
          continue;
      }

      Node const& current = currentScoredNode.node;
      Score const& currentScore = currentScoredNode.score;

      if (m_goalReached(current)) {
        m_finished = true;
        m_result = reconstructPath(current);
        return true;
      }

      m_closedSet.insert(current);

      neighbors.clear();
      m_getAdjacent(current, neighbors);

      for (Edge const& edge : neighbors) {
        if (m_closedSet.find(edge.target) != m_closedSet.end() || m_nodeMeta.contains(edge.target))
          // We've already visited this node.
          continue;

        double newGScore = currentScore.gScore + edge.cost;
        NodeMeta& targetMeta = m_nodeMeta[edge.target];
        Score& targetScore = targetMeta.score;
        if (m_openSet.find(edge.target) == m_openSet.end() || newGScore < targetScore.gScore) {
          targetMeta.cameFrom = edge;
          targetScore.gScore = newGScore;
          targetScore.hScore = m_heuristicCost(edge.target, m_goal);
          targetScore.fScore = targetScore.gScore + targetScore.hScore;

          if (m_maxFScore && targetScore.fScore > *m_maxFScore)
            continue;

          // Early exploration optimization - no need to add things to the
          // openQueue/openSet
          // if they're at least as good as the current node.
          if (targetScore.fScore <= currentScore.fScore) {
            if (m_earlyExploration.isNothing()) {
              m_earlyExploration = ScoredNode{targetScore, edge.target};
              continue;
            } else if (m_earlyExploration->score.fScore > targetScore.fScore) {
              m_openSet.insert(m_earlyExploration->node);
              m_openQueue.push(*m_earlyExploration);
              m_earlyExploration = ScoredNode{targetScore, edge.target};
              continue;
            }
          }
          m_openSet.insert(edge.target);
          m_openQueue.push(ScoredNode{targetScore, edge.target});
        }
      }
    }
  }

  template <class Edge, class Node>
  Maybe<Path<Edge>> const& Search<Edge, Node>::result() const {
    return m_result;
  }

  template <class Edge, class Node>
  Maybe<Path<Edge>> const& Search<Edge, Node>::findPath(Node startNode, Node goalNode) {
    start(std::move(startNode), std::move(goalNode));
    explore();
    return result();
  }

  template <class Edge, class Node>
  Path<Edge> Search<Edge, Node>::reconstructPath(Node currentNode) {
    Path<Edge> res; // this will be backwards, we reverse it before returning it.
    while (m_nodeMeta.find(currentNode) != m_nodeMeta.end()) {
      Maybe<Edge> currentEdge = m_nodeMeta[currentNode].cameFrom;
      if (currentEdge.isNothing())
        break;
      res.append(*currentEdge);
      currentNode = currentEdge->source;
    }
    std::reverse(res.begin(), res.end());
    return res;
  }
}

}
