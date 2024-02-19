#ifndef STAR_EITHER_HPP
#define STAR_EITHER_HPP

#include "StarVariant.hpp"

namespace Star {

STAR_EXCEPTION(EitherException, StarException);

template <typename Value>
struct EitherLeftValue {
  Value value;
};

template <typename Value>
struct EitherRightValue {
  Value value;
};

template <typename Value>
EitherLeftValue<Value> makeLeft(Value value);

template <typename Value>
EitherRightValue<Value> makeRight(Value value);

// Container that contains exactly one of either Left or Right.
template <typename Left, typename Right>
class Either {
public:
  // Constructs Either that contains a default constructed Left value
  Either();

  Either(EitherLeftValue<Left> left);
  Either(EitherRightValue<Right> right);

  template <typename T>
  Either(EitherLeftValue<T> left);

  template <typename T>
  Either(EitherRightValue<T> right);

  Either(Either const& rhs);
  Either(Either&& rhs);

  Either& operator=(Either const& rhs);
  Either& operator=(Either&& rhs);

  template <typename T>
  Either& operator=(EitherLeftValue<T> left);

  template <typename T>
  Either& operator=(EitherRightValue<T> right);

  bool isLeft() const;
  bool isRight() const;

  void setLeft(Left left);
  void setRight(Right left);

  // left() and right() throw EitherException on invalid access

  Left const& left() const;
  Right const& right() const;

  Left& left();
  Right& right();

  Maybe<Left> maybeLeft() const;
  Maybe<Right> maybeRight() const;

  // leftPtr() and rightPtr() do not throw on invalid access

  Left const* leftPtr() const;
  Right const* rightPtr() const;

  Left* leftPtr();
  Right* rightPtr();

private:
  typedef EitherLeftValue<Left> LeftType;
  typedef EitherRightValue<Right> RightType;

  Variant<LeftType, RightType> m_value;
};

template <typename Value>
EitherLeftValue<Value> makeLeft(Value value) {
  return {std::move(value)};
}

template <typename Value>
EitherRightValue<Value> makeRight(Value value) {
  return {std::move(value)};
}

template <typename Left, typename Right>
Either<Left, Right>::Either() {}

template <typename Left, typename Right>
Either<Left, Right>::Either(EitherLeftValue<Left> left)
  : m_value(std::move(left)) {}

template <typename Left, typename Right>
Either<Left, Right>::Either(EitherRightValue<Right> right)
  : m_value(std::move(right)) {}

template <typename Left, typename Right>
template <typename T>
Either<Left, Right>::Either(EitherLeftValue<T> left)
  : Either(LeftType{std::move(left.value)}) {}

template <typename Left, typename Right>
template <typename T>
Either<Left, Right>::Either(EitherRightValue<T> right)
  : Either(RightType{std::move(right.value)}) {}

template <typename Left, typename Right>
Either<Left, Right>::Either(Either const& rhs)
  : m_value(rhs.m_value) {}

template <typename Left, typename Right>
Either<Left, Right>::Either(Either&& rhs)
  : m_value(std::move(rhs.m_value)) {}

template <typename Left, typename Right>
Either<Left, Right>& Either<Left, Right>::operator=(Either const& rhs) {
  m_value = rhs.m_value;
  return *this;
}

template <typename Left, typename Right>
Either<Left, Right>& Either<Left, Right>::operator=(Either&& rhs) {
  m_value = std::move(rhs.m_value);
  return *this;
}

template <typename Left, typename Right>
template <typename T>
Either<Left, Right>& Either<Left, Right>::operator=(EitherLeftValue<T> left) {
  m_value = LeftType{std::move(left.value)};
  return *this;
}

template <typename Left, typename Right>
template <typename T>
Either<Left, Right>& Either<Left, Right>::operator=(EitherRightValue<T> right) {
  m_value = RightType{std::move(right.value)};
  return *this;
}

template <typename Left, typename Right>
bool Either<Left, Right>::isLeft() const {
  return m_value.template is<LeftType>();
}

template <typename Left, typename Right>
bool Either<Left, Right>::isRight() const {
  return m_value.template is<RightType>();
}

template <typename Left, typename Right>
void Either<Left, Right>::setLeft(Left left) {
  m_value = LeftType{std::move(left)};
}

template <typename Left, typename Right>
void Either<Left, Right>::setRight(Right right) {
  m_value = RightType{std::move(right)};
}

template <typename Left, typename Right>
Left const& Either<Left, Right>::left() const {
  if (auto l = leftPtr())
    return *l;
  throw EitherException("Improper access of left side of Either");
}

template <typename Left, typename Right>
Right const& Either<Left, Right>::right() const {
  if (auto r = rightPtr())
    return *r;
  throw EitherException("Improper access of right side of Either");
}

template <typename Left, typename Right>
Left& Either<Left, Right>::left() {
  if (auto l = leftPtr())
    return *l;
  throw EitherException("Improper access of left side of Either");
}

template <typename Left, typename Right>
Right& Either<Left, Right>::right() {
  if (auto r = rightPtr())
    return *r;
  throw EitherException("Improper access of right side of Either");
}

template <typename Left, typename Right>
Maybe<Left> Either<Left, Right>::maybeLeft() const {
  if (auto l = leftPtr())
    return *l;
  return {};
}

template <typename Left, typename Right>
Maybe<Right> Either<Left, Right>::maybeRight() const {
  if (auto r = rightPtr())
    return *r;
  return {};
}

template <typename Left, typename Right>
Left const* Either<Left, Right>::leftPtr() const {
  if (auto l = m_value.template ptr<LeftType>())
    return &l->value;
  return nullptr;
}

template <typename Left, typename Right>
Right const* Either<Left, Right>::rightPtr() const {
  if (auto r = m_value.template ptr<RightType>())
    return &r->value;
  return nullptr;
}

template <typename Left, typename Right>
Left* Either<Left, Right>::leftPtr() {
  if (auto l = m_value.template ptr<LeftType>())
    return &l->value;
  return nullptr;
}

template <typename Left, typename Right>
Right* Either<Left, Right>::rightPtr() {
  if (auto r = m_value.template ptr<RightType>())
    return &r->value;
  return nullptr;
}

}

#endif
