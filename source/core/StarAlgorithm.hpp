#pragma once

#include "StarException.hpp"

namespace Star {

// Function that does nothing and takes any number of arguments
template <typename... T>
void nothing(T&&...) {}

// Functional constructor call / casting.
template <typename ToType>
struct construct {
  template <typename... FromTypes>
  ToType operator()(FromTypes&&... fromTypes) const {
    return ToType(std::forward<FromTypes>(fromTypes)...);
  }
};

struct identity {
  template <typename U>
  constexpr decltype(auto) operator()(U&& v) const {
    return std::forward<U>(v);
  }
};

template <typename Func>
struct SwallowReturn {
  template <typename... T>
  void operator()(T&&... args) {
    func(std::forward<T>(args)...);
  }

  Func func;
};

template <typename Func>
SwallowReturn<Func> swallow(Func f) {
  return SwallowReturn<Func>{std::move(f)};
}

struct Empty {
  bool operator==(Empty const) const {
    return true;
  }

  bool operator<(Empty const) const {
    return false;
  }
};

// Compose arbitrary functions
template <typename FirstFunction, typename SecondFunction>
struct FunctionComposer {
  FirstFunction f1;
  SecondFunction f2;

  template <typename... T>
  decltype(auto) operator()(T&&... args) {
    return f1(f2(std::forward<T>(args)...));
  }
};

template <typename FirstFunction, typename SecondFunction>
decltype(auto) compose(FirstFunction&& firstFunction, SecondFunction&& secondFunction) {
  return FunctionComposer<FirstFunction, SecondFunction>{std::move(std::forward<FirstFunction>(firstFunction)), std::move(std::forward<SecondFunction>(secondFunction))};
}

template <typename FirstFunction, typename SecondFunction, typename ThirdFunction, typename... RestFunctions>
decltype(auto) compose(FirstFunction firstFunction, SecondFunction secondFunction, ThirdFunction thirdFunction, RestFunctions... restFunctions) {
  return compose(std::forward<FirstFunction>(firstFunction), compose(std::forward<SecondFunction>(secondFunction), compose(std::forward<ThirdFunction>(thirdFunction), std::forward<RestFunctions>(restFunctions)...)));
}

template <typename Container, typename Value, typename Function>
Value fold(Container const& l, Value v, Function f) {
  auto i = l.begin();
  auto e = l.end();
  while (i != e) {
    v = f(v, *i);
    ++i;
  }
  return v;
}

// Like fold, but returns default value when container is empty.
template <typename Container, typename Function>
typename Container::value_type fold1(Container const& l, Function f) {
  typename Container::value_type res = {};
  typename Container::const_iterator i = l.begin();
  typename Container::const_iterator e = l.end();

  if (i == e)
    return res;

  res = *i;
  ++i;
  while (i != e) {
    res = f(res, *i);
    ++i;
  }
  return res;
}

// Return intersection of sorted containers.
template <typename Container>
Container intersect(Container const& a, Container const& b) {
  Container r;
  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(r, r.end()));
  return r;
}

template <typename MapType1, typename MapType2>
bool mapMerge(MapType1& targetMap, MapType2 const& sourceMap, bool overwrite = false) {
  bool noCommonKeys = true;
  for (auto i = sourceMap.begin(); i != sourceMap.end(); ++i) {
    auto res = targetMap.insert(*i);
    if (!res.second) {
      noCommonKeys = false;
      if (overwrite)
        res.first->second = i->second;
    }
  }
  return noCommonKeys;
}

template <typename MapType1, typename MapType2>
bool mapsEqual(MapType1 const& m1, MapType2 const& m2) {
  if (&m1 == &m2)
    return true;

  if (m1.size() != m2.size())
    return false;

  for (auto const& m1pair : m1) {
    auto m2it = m2.find(m1pair.first);
    if (m2it == m2.end() || !(m2it->second == m1pair.second))
      return false;
  }

  return true;
}

template <typename Container, typename Filter>
void filter(Container& container, Filter&& filter) {
  auto p = std::begin(container);
  while (p != std::end(container)) {
    if (!filter(*p))
      p = container.erase(p);
    else
      ++p;
  }
}

template <typename OutContainer, typename InContainer, typename Filter>
OutContainer filtered(InContainer const& input, Filter&& filter) {
  OutContainer out;
  auto p = std::begin(input);
  while (p != std::end(input)) {
    if (filter(*p))
      out.insert(out.end(), *p);
    ++p;
  }
  return out;
}

template <typename Container, typename Cond>
void eraseWhere(Container& container, Cond&& cond) {
  auto p = std::begin(container);
  while (p != std::end(container)) {
    if (cond(*p))
      p = container.erase(p);
    else
      ++p;
  }
}

template <typename Container, typename Compare>
void sort(Container& c, Compare comp) {
  std::sort(c.begin(), c.end(), comp);
}

template <typename Container, typename Compare>
void stableSort(Container& c, Compare comp) {
  std::stable_sort(c.begin(), c.end(), comp);
}

template <typename Container>
void sort(Container& c) {
  std::sort(c.begin(), c.end(), std::less<typename Container::value_type>());
}

template <typename Container>
void stableSort(Container& c) {
  std::stable_sort(c.begin(), c.end(), std::less<typename Container::value_type>());
}

template <typename Container, typename Compare>
Container sorted(Container const& c, Compare comp) {
  auto c2 = c;
  sort(c2, comp);
  return c2;
}

template <typename Container, typename Compare>
Container stableSorted(Container const& c, Compare comp) {
  auto c2 = c;
  sort(c2, comp);
  return c2;
}

template <typename Container>
Container sorted(Container const& c) {
  auto c2 = c;
  sort(c2);
  return c2;
}

template <typename Container>
Container stableSorted(Container const& c) {
  auto c2 = c;
  sort(c2);
  return c2;
}

// Sort a container by the output of a computed value. The computed value is
// only computed *once* per item in the container, which is useful both for
// when the computed value is costly, and to avoid sorting instability with
// floating point values.  Container must have size() and operator[], and also
// must be constructable with Container(size_t).
template <typename Container, typename Getter>
void sortByComputedValue(Container& container, Getter&& valueGetter, bool stable = false) {
  typedef typename Container::value_type ContainerValue;
  typedef decltype(valueGetter(ContainerValue())) ComputedValue;
  typedef std::pair<ComputedValue, size_t> ComputedPair;

  size_t containerSize = container.size();

  if (containerSize <= 1)
    return;

  std::vector<ComputedPair> work(containerSize);
  for (size_t i = 0; i < containerSize; ++i)
    work[i] = {valueGetter(container[i]), i};

  auto compare = [](ComputedPair const& a, ComputedPair const& b) { return a.first < b.first; };

  // Sort the comptued values and the associated indexes
  if (stable)
    stableSort(work, compare);
  else
    sort(work, compare);

  Container result(containerSize);
  for (size_t i = 0; i < containerSize; ++i)
    swap(result[i], container[work[i].second]);

  swap(container, result);
}

template <typename Container, typename Getter>
void stableSortByComputedValue(Container& container, Getter&& valueGetter) {
  return sortByComputedValue(container, std::forward<Getter>(valueGetter), true);
}

template <typename Container>
void reverse(Container& c) {
  std::reverse(c.begin(), c.end());
}

template <typename Container>
Container reverseCopy(Container c) {
  reverse(c);
  return c;
}

template <typename T>
T copy(T c) {
  return c;
}

template <typename Container>
typename Container::value_type sum(Container const& cont) {
  return fold1(cont, std::plus<typename Container::value_type>());
}

template <typename Container>
typename Container::value_type product(Container const& cont) {
  return fold1(cont, std::multiplies<typename Container::value_type>());
}

template <typename OutContainer, typename InContainer, typename Function>
void transformInto(OutContainer& outContainer, InContainer&& inContainer, Function&& function) {
  for (auto&& elem : inContainer) {
    if (std::is_rvalue_reference<InContainer&&>::value)
      outContainer.insert(outContainer.end(), function(std::move(elem)));
    else
      outContainer.insert(outContainer.end(), function(elem));
  }
}

template <typename OutContainer, typename InContainer, typename Function>
OutContainer transform(InContainer&& container, Function&& function) {
  OutContainer res;
  transformInto(res, std::forward<InContainer>(container), std::forward<Function>(function));
  return res;
}

template <typename OutputContainer, typename Function, typename Container1, typename Container2>
OutputContainer zipWith(Function&& function, Container1 const& cont1, Container2 const& cont2) {
  auto it1 = cont1.begin();
  auto it2 = cont2.begin();

  OutputContainer out;
  while (it1 != cont1.end() && it2 != cont2.end()) {
    out.insert(out.end(), function(*it1, *it2));
    ++it1;
    ++it2;
  }

  return out;
}

// Moves the given value and into an rvalue.  Works whether or not the type has
// a valid move constructor or not.  Always leaves the given value in its
// default constructed state.
template <typename T>
T take(T& t) {
  T t2 = std::move(t);
  t = T();
  return t2;
}

template <typename Container1, typename Container2>
bool containersEqual(Container1 const& cont1, Container2 const& cont2) {
  if (cont1.size() != cont2.size())
    return false;
  else
    return std::equal(cont1.begin(), cont1.end(), cont2.begin());
}

// Wraps a unary function to produce an output iterator
template <typename UnaryFunction>
class FunctionOutputIterator {
public:
  typedef std::output_iterator_tag iterator_category;
  typedef void value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;

  class OutputProxy {
  public:
    OutputProxy(UnaryFunction& f)
      : m_function(f) {}

    template <typename T>
    OutputProxy& operator=(T&& value) {
      m_function(std::forward<T>(value));
      return *this;
    }

  private:
    UnaryFunction& m_function;
  };

  explicit FunctionOutputIterator(UnaryFunction f = UnaryFunction())
    : m_function(std::move(f)) {}

  OutputProxy operator*() {
    return OutputProxy(m_function);
  }

  FunctionOutputIterator& operator++() {
    return *this;
  }

  FunctionOutputIterator operator++(int) {
    return *this;
  }

private:
  UnaryFunction m_function;
};

template <typename UnaryFunction>
FunctionOutputIterator<UnaryFunction> makeFunctionOutputIterator(UnaryFunction f) {
  return FunctionOutputIterator<UnaryFunction>(std::move(f));
}

// Wraps a nullary function to produce an input iterator
template <typename NullaryFunction>
class FunctionInputIterator {
public:
  typedef std::output_iterator_tag iterator_category;
  typedef void value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;

  typedef typename std::result_of<NullaryFunction()>::type FunctionReturnType;

  explicit FunctionInputIterator(NullaryFunction f = {})
    : m_function(std::move(f)) {}

  FunctionReturnType operator*() {
    return m_function();
  }

  FunctionInputIterator& operator++() {
    return *this;
  }

  FunctionInputIterator operator++(int) {
    return *this;
  }

private:
  NullaryFunction m_function;
};

template <typename NullaryFunction>
FunctionInputIterator<NullaryFunction> makeFunctionInputIterator(NullaryFunction f) {
  return FunctionInputIterator<NullaryFunction>(std::move(f));
}

template <typename Iterable>
struct ReverseWrapper {
private:
  Iterable& m_iterable;

public:
  ReverseWrapper(Iterable& iterable) : m_iterable(iterable) {}

  decltype(auto) begin() const {
    return std::rbegin(m_iterable);
  }

  decltype(auto) end() const {
    return std::rend(m_iterable);
  }
};

template <typename Iterable>
ReverseWrapper<Iterable> reverseIterate(Iterable& list) {
  return ReverseWrapper<Iterable>(list);
}

template <typename Functor>
class FinallyGuard {
public:
  FinallyGuard(Functor functor) : functor(std::move(functor)), dismiss(false) {}

  FinallyGuard(FinallyGuard&& o) : functor(std::move(o.functor)), dismiss(o.dismiss) {
    o.cancel();
  }

  FinallyGuard& operator=(FinallyGuard&& o) {
    functor = std::move(o.functor);
    dismiss = o.dismiss;
    o.cancel();
    return *this;
  }

  ~FinallyGuard() {
    if (!dismiss)
      functor();
  }

  void cancel() {
    dismiss = true;
  }

private:
  Functor functor;
  bool dismiss;
};

template <typename Functor>
FinallyGuard<typename std::decay<Functor>::type> finally(Functor&& f) {
  return FinallyGuard<Functor>(std::forward<Functor>(f));
}

// Generates compile time sequences of indexes from MinIndex to MaxIndex

template <size_t...>
struct IndexSequence {};

template <size_t Min, size_t N, size_t... S>
struct GenIndexSequence : GenIndexSequence<Min, N - 1, N - 1, S...> {};

template <size_t Min, size_t... S>
struct GenIndexSequence<Min, Min, S...> {
  typedef IndexSequence<S...> type;
};

// Apply a tuple as individual arguments to a function

template <typename Function, typename Tuple, size_t... Indexes>
decltype(auto) tupleUnpackFunctionIndexes(Function&& function, Tuple&& args, IndexSequence<Indexes...> const&) {
  return function(get<Indexes>(std::forward<Tuple>(args))...);
}

template <typename Function, typename Tuple>
decltype(auto) tupleUnpackFunction(Function&& function, Tuple&& args) {
  return tupleUnpackFunctionIndexes<Function, Tuple>(std::forward<Function>(function), std::forward<Tuple>(args),
      typename GenIndexSequence<0, std::tuple_size<typename std::decay<Tuple>::type>::value>::type());
}

// Apply a function to every element of a tuple.  This will NOT happen in a
// predictable order!

template <typename Function, typename Tuple, size_t... Indexes>
decltype(auto) tupleApplyFunctionIndexes(Function&& function, Tuple&& args, IndexSequence<Indexes...> const&) {
  return make_tuple(function(get<Indexes>(std::forward<Tuple>(args)))...);
}

template <typename Function, typename Tuple>
decltype(auto) tupleApplyFunction(Function&& function, Tuple&& args) {
  return tupleApplyFunctionIndexes<Function, Tuple>(std::forward<Function>(function), std::forward<Tuple>(args),
      typename GenIndexSequence<0, std::tuple_size<typename std::decay<Tuple>::type>::value>::type());
}

// Use this version if you do not care about the return value of the function
// or your function returns void.  This version DOES happen in a predictable
// order, first argument first, last argument last.

template <typename Function, typename Tuple>
void tupleCallFunctionCaller(Function&&, Tuple&&) {}

template <typename Tuple, typename Function, typename First, typename... Rest>
void tupleCallFunctionCaller(Tuple&& t, Function&& function) {
  tupleCallFunctionCaller<Tuple, Function, Rest...>(std::forward<Tuple>(t), std::forward<Function>(function));
  function(get<sizeof...(Rest)>(std::forward<Tuple>(t)));
}

template <typename Tuple, typename Function, typename... T>
void tupleCallFunctionExpander(Tuple&& t, Function&& function, tuple<T...> const&) {
  tupleCallFunctionCaller<Tuple, Function, T...>(std::forward<Tuple>(t), std::forward<Function>(function));
}

template <typename Tuple, typename Function>
void tupleCallFunction(Tuple&& t, Function&& function) {
  tupleCallFunctionExpander<Tuple, Function>(std::forward<Tuple>(t), std::forward<Function>(function), std::forward<Tuple>(t));
}

// Get a subset of a tuple

template <typename Tuple, size_t... Indexes>
decltype(auto) subTupleIndexes(Tuple&& t, IndexSequence<Indexes...> const&) {
  return make_tuple(get<Indexes>(std::forward<Tuple>(t))...);
}

template <size_t Min, size_t Size, typename Tuple>
decltype(auto) subTuple(Tuple&& t) {
  return subTupleIndexes(std::forward<Tuple>(t), GenIndexSequence<Min, Size>::type());
}

template <size_t Trim, typename Tuple>
decltype(auto) trimTuple(Tuple&& t) {
  return subTupleIndexes(std::forward<Tuple>(t), typename GenIndexSequence<Trim, std::tuple_size<typename std::decay<Tuple>::type>::value>::type());
}

// Unpack a parameter expansion into a container

template <typename Container>
void unpackVariadicImpl(Container&) {}

template <typename Container, typename TFirst, typename... TRest>
void unpackVariadicImpl(Container& container, TFirst&& tfirst, TRest&&... trest) {
  container.insert(container.end(), std::forward<TFirst>(tfirst));
  unpackVariadicImpl(container, std::forward<TRest>(trest)...);
}

template <typename Container, typename... T>
Container unpackVariadic(T&&... t) {
  Container c;
  unpackVariadicImpl(c, std::forward<T>(t)...);
  return c;
}

// Call a function on each entry in a variadic parameter set

template <typename Function>
void callFunctionVariadic(Function&&) {}

template <typename Function, typename Arg1, typename... ArgRest>
void callFunctionVariadic(Function&& function, Arg1&& arg1, ArgRest&&... argRest) {
  function(arg1);
  callFunctionVariadic(std::forward<Function>(function), std::forward<ArgRest>(argRest)...);
}

template <typename... Rest>
struct VariadicTypedef;

template <>
struct VariadicTypedef<> {};

template <typename FirstT, typename... RestT>
struct VariadicTypedef<FirstT, RestT...> {
  typedef FirstT First;
  typedef VariadicTypedef<RestT...> Rest;
};

// For generic types, directly use the result of the signature of its
// 'operator()'
template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())> {};

template <typename ReturnType, typename... ArgsTypes>
struct FunctionTraits<ReturnType(ArgsTypes...)> {
  // arity is the number of arguments.
  static constexpr size_t Arity = sizeof...(ArgsTypes);

  typedef ReturnType Return;

  typedef VariadicTypedef<ArgsTypes...> Args;
  typedef tuple<ArgsTypes...> ArgTuple;

  template <size_t i>
  struct Arg {
    // the i-th argument is equivalent to the i-th tuple element of a tuple
    // composed of those arguments.
    typedef typename tuple_element<i, ArgTuple>::type type;
  };
};

template <typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType (*)(Args...)> : public FunctionTraits<ReturnType(Args...)> {};

template <typename FunctionType>
struct FunctionTraits<std::function<FunctionType>> : public FunctionTraits<FunctionType> {};

template <typename ClassType, typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType (ClassType::*)(Args...)> : public FunctionTraits<ReturnType(Args...)> {
  typedef ClassType& OwnerType;
};

template <typename ClassType, typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType (ClassType::*)(Args...) const> : public FunctionTraits<ReturnType(Args...)> {
  typedef const ClassType& OwnerType;
};

template <typename T>
struct FunctionTraits<T&> : public FunctionTraits<T> {};

template <typename T>
struct FunctionTraits<T const&> : public FunctionTraits<T> {};

template <typename T>
struct FunctionTraits<T&&> : public FunctionTraits<T> {};

template <typename T>
struct FunctionTraits<T const&&> : public FunctionTraits<T> {};

}
