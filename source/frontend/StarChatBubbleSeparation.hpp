#pragma once

#include "StarRect.hpp"
#include "StarList.hpp"

namespace Star {

template <typename T>
struct BubbleState {
  T contents;
  // The destination is the position the bubble is being pulled towards,
  // ignoring the positions of all other bubbles (so it could overlap).
  // This is the position of the entity.
  Vec2F idealDestination;
  // The idealDestination is the position of the entity now, while the
  // currentDestination is only updated once the idealDestination passes a
  // minimum distance. (So that the algorithm is not re-run for sub-pixel
  // position changes.)
  Vec2F currentDestination;
  // The bound box of the nametag if it was at the destination.
  RectF boundBox;
  // The separation offset for the bubble chosen by the algorithm (which it may not
  // have fully moved to yet).
  Vec2F seperatedOffset;
  // The bound box of the bubble around the separatedOffset.
  RectF separatedBox;
  // Where the bubble offset is now, which could be anywhere en route to the
  // seperatedOffset.
  Vec2F currentOffset;

  // The final position.
  Vec2F currentPosition;
};

template <typename T>
class BubbleSeparator {
public:
  using Bubble = BubbleState<T>;

  BubbleSeparator(float tweenFactor = 0.5f, float movementThreshold = 2.0f);

  float tweenFactor() const;
  void setTweenFactor(float tweenFactor);

  float movementThreshold() const;
  void setMovementThreshold(float movementThreshold);

  void addBubble(Vec2F position, RectF boundBox, T contents, unsigned margin = 0);

  void filter(function<bool(Bubble const&, T&)> func);
  List<Bubble> filtered(function<bool(Bubble const&, T const&)> func);
  void forEach(function<void(Bubble&, T&)> func);

  void update(float dt);
  void clear();
  bool empty() const;

private:
  static bool compareBubbleY(Bubble const& a, Bubble const& b);

  float m_tweenFactor;
  float m_movementThreshold;
  List<Bubble> m_bubbles;
  List<RectF> m_sortedLeftEdges;
  List<RectF> m_sortedRightEdges;
};

bool compareLeft(RectF const& a, RectF const& b);
bool compareRight(RectF const& a, RectF const& b);
bool compareOverlapLeft(RectF const& newBox, RectF const& fixedBox);
bool compareOverlapRight(RectF const& newBox, RectF const& fixedBox);

// Shifts box upwards until it is not overlapping any of the boxes in
// sortedLeftEdges
// and sortedRightEdges.
// The resulting box is returned and inserted into sortedLeftEdges and
// sortedRightEdges.
// The two lists contain all the chat bubbles that have been separated, sorted
// by
// the X positions of their left and right edges respectively.
RectF separateBubble(List<RectF> const& sortedLeftEdges, List<RectF> const& sortedRightEdges, List<RectF>& outLeftEdges, List<RectF>& outRightEdges, RectF box);

template <typename T>
BubbleSeparator<T>::BubbleSeparator(float tweenFactor, float movementThreshold)
  : m_tweenFactor(tweenFactor), m_movementThreshold(movementThreshold), m_sortedLeftEdges(), m_sortedRightEdges() {}

template <typename T>
float BubbleSeparator<T>::tweenFactor() const {
  return m_tweenFactor;
}

template <typename T>
void BubbleSeparator<T>::setTweenFactor(float tweenFactor) {
  m_tweenFactor = tweenFactor;
}

template <typename T>
float BubbleSeparator<T>::movementThreshold() const {
  return m_movementThreshold;
}

template <typename T>
void BubbleSeparator<T>::setMovementThreshold(float movementThreshold) {
  m_movementThreshold = movementThreshold;
}

template <typename T>
void BubbleSeparator<T>::addBubble(Vec2F position, RectF boundBox, T contents, unsigned margin) {
  boundBox.setYMax(boundBox.yMax() + margin);
  RectF separated = separateBubble(m_sortedLeftEdges, m_sortedRightEdges, m_sortedLeftEdges, m_sortedRightEdges, boundBox);
  Vec2F separatedOffset = separated.min() - boundBox.min();
  Vec2F separatedPosition = position + separatedOffset;
  Bubble bubble = Bubble{ contents, position, position, boundBox, separatedOffset, separated, separatedOffset, separatedPosition };
  m_bubbles.insertSorted(std::move(bubble), &BubbleSeparator<T>::compareBubbleY);
}

template <typename T>
void BubbleSeparator<T>::filter(function<bool(Bubble const&, T&)> func) {
  m_bubbles.filter([this, func](Bubble& bubble) {
    if (!func(bubble, bubble.contents)) {
      m_sortedLeftEdges.remove(bubble.separatedBox);
      m_sortedRightEdges.remove(bubble.separatedBox);
      return false;
    }
    return true;
  });
}

template <typename T>
List<BubbleState<T>> BubbleSeparator<T>::filtered(function<bool(Bubble const&, T const&)> func) {
  return m_bubbles.filtered([func](Bubble const& bubble) { return func(bubble, bubble.contents); });
}

template <typename T>
void BubbleSeparator<T>::forEach(function<void(Bubble&, T&)> func) {
  bool anyMoved = false;
  
  List<Box<float, 2>> leftEdges = std::move(m_sortedLeftEdges);
  List<Box<float, 2>> rightEdges = std::move(m_sortedRightEdges);

  m_bubbles.exec([this, func, &anyMoved, &leftEdges, &rightEdges](Bubble& bubble) {
    RectF oldBoundBox = bubble.boundBox;

    func(bubble, bubble.contents);

    // Kae: I'm disabling the movement threshold check for now because it also
    // stops bubble sorting on bubbles that haven't moved, which causes problems.

    Vec2F sizeDelta = bubble.boundBox.size() - oldBoundBox.size();
    leftEdges.remove(bubble.separatedBox);
    rightEdges.remove(bubble.separatedBox);
    RectF boundBox = RectF::withCenter(bubble.idealDestination, bubble.boundBox.size());
    RectF separated = separateBubble(leftEdges, rightEdges, m_sortedLeftEdges, m_sortedRightEdges, boundBox);
    leftEdges.insertSorted(bubble.separatedBox, compareLeft);
    rightEdges.insertSorted(bubble.separatedBox, compareRight);

    anyMoved = true;
    bubble.currentDestination = bubble.idealDestination;
    bubble.boundBox = boundBox;
    bubble.seperatedOffset = separated.min() - boundBox.min();
    bubble.separatedBox = separated;
  });

  m_sortedLeftEdges.sort(compareLeft);
  m_sortedRightEdges.sort(compareRight);

  if (anyMoved)
    m_bubbles.sort(&BubbleSeparator<T>::compareBubbleY);
}

template <typename T>
void BubbleSeparator<T>::update(float dt) {
  m_bubbles.exec([this](Bubble& bubble) {
    Vec2F delta = bubble.seperatedOffset - bubble.currentOffset;
    bubble.currentOffset += m_tweenFactor * delta;

    bubble.currentPosition = bubble.currentDestination + bubble.currentOffset;
  });
}

template <typename T>
void BubbleSeparator<T>::clear() {
  m_bubbles.clear();
  m_sortedLeftEdges.clear();
  m_sortedRightEdges.clear();
}

template <typename T>
bool BubbleSeparator<T>::empty() const {
  return m_bubbles.empty();
}

template <typename T>
bool BubbleSeparator<T>::compareBubbleY(Bubble const& a, Bubble const& b) {
  return a.currentDestination[1] < b.currentDestination[1];
}
}
