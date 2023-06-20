#ifndef STAR_RPC_PROMISE_HPP
#define STAR_RPC_PROMISE_HPP

#include "StarEither.hpp"
#include "StarString.hpp"

namespace Star {

STAR_EXCEPTION(RpcPromiseException, StarException);

// The other side of an RpcPromise, can be used to either fulfill or fail a
// paired promise.  Call either fulfill or fail function exactly once, any
// further invocations will result in an exception.
template <typename Result, typename Error = String>
class RpcPromiseKeeper {
public:
  void fulfill(Result result);
  void fail(Error error);

private:
  template <typename ResultT, typename ErrorT>
  friend class RpcPromise;

  function<void(Result)> m_fulfill;
  function<void(Error)> m_fail;
};

// A generic promise for the result of a remote procedure call.  It has
// reference semantics and is implicitly shared, but is not thread safe.
template <typename Result, typename Error = String>
class RpcPromise {
public:
  static pair<RpcPromise, RpcPromiseKeeper<Result, Error>> createPair();
  static RpcPromise createFulfilled(Result result);
  static RpcPromise createFailed(Error error);

  // Has the respoonse either failed or succeeded?
  bool finished() const;
  // Has the response finished with success?
  bool succeeded() const;
  // Has the response finished with failure?
  bool failed() const;

  // Returns the result of the rpc call on success, nothing on failure or when
  // not yet finished.
  Maybe<Result> const& result() const;

  // Returns the error of a failed rpc call.  Returns nothing if the call is
  // successful or not yet finished.
  Maybe<Error> const& error() const;

  // Wrap this RpcPromise into another promise which returns instead the result
  // of this function when fulfilled
  template <typename Function>
  decltype(auto) wrap(Function function);

private:
  template <typename ResultT, typename ErrorT>
  friend class RpcPromise;

  struct Value {
    Maybe<Result> result;
    Maybe<Error> error;
  };

  RpcPromise() = default;

  function<Value const*()> m_getValue;
};

template <typename Result, typename Error>
void RpcPromiseKeeper<Result, Error>::fulfill(Result result) {
  m_fulfill(move(result));
}

template <typename Result, typename Error>
void RpcPromiseKeeper<Result, Error>::fail(Error error) {
  m_fail(move(error));
}

template <typename Result, typename Error>
pair<RpcPromise<Result, Error>, RpcPromiseKeeper<Result, Error>> RpcPromise<Result, Error>::createPair() {
  auto valuePtr = make_shared<Value>();

  RpcPromise promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };

  RpcPromiseKeeper<Result, Error> keeper;
  keeper.m_fulfill = [valuePtr](Result result) {
    if (valuePtr->result || valuePtr->error)
      throw RpcPromiseException("fulfill called on already finished RpcPromise");
    valuePtr->result = move(result);
  };
  keeper.m_fail = [valuePtr](Error error) {
    if (valuePtr->result || valuePtr->error)
      throw RpcPromiseException("fail called on already finished RpcPromise");
    valuePtr->error = move(error);
  };

  return {move(promise), move(keeper)};
}

template <typename Result, typename Error>
RpcPromise<Result, Error> RpcPromise<Result, Error>::createFulfilled(Result result) {
  auto valuePtr = make_shared<Value>();
  valuePtr->result = move(result);

  RpcPromise<Result, Error> promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };
  return promise;
}

template <typename Result, typename Error>
RpcPromise<Result, Error> RpcPromise<Result, Error>::createFailed(Error error) {
  auto valuePtr = make_shared<Value>();
  valuePtr->error = move(error);

  RpcPromise<Result, Error> promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };
  return promise;
}

template <typename Result, typename Error>
bool RpcPromise<Result, Error>::finished() const {
  auto val = m_getValue();
  return val->result || val->error;
}

template <typename Result, typename Error>
bool RpcPromise<Result, Error>::succeeded() const {
  return m_getValue()->result.isValid();
}

template <typename Result, typename Error>
bool RpcPromise<Result, Error>::failed() const {
  return m_getValue()->error.isValid();
}

template <typename Result, typename Error>
Maybe<Result> const& RpcPromise<Result, Error>::result() const {
  return m_getValue()->result;
}

template <typename Result, typename Error>
Maybe<Error> const& RpcPromise<Result, Error>::error() const {
  return m_getValue()->error;
}

template <typename Result, typename Error>
template <typename Function>
decltype(auto) RpcPromise<Result, Error>::wrap(Function function) {
  typedef RpcPromise<typename std::decay<decltype(function(std::declval<Result>()))>::type, Error> WrappedPromise;
  WrappedPromise wrappedPromise;
  wrappedPromise.m_getValue = [wrapper = move(function), valuePtr = make_shared<typename WrappedPromise::Value>(), otherGetValue = m_getValue]() {
    if (!valuePtr->result && !valuePtr->error) {
      auto otherValue = otherGetValue();
      if (otherValue->result)
        valuePtr->result.set(wrapper(*otherValue->result));
      else if (otherValue->error)
        valuePtr->error.set(*otherValue->error);
    }
    return valuePtr.get();
  };
  return wrappedPromise;
}

}

#endif
