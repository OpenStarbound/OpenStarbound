#pragma once

#include "StarEither.hpp"
#include "StarString.hpp"
#include "StarThread.hpp"

// TODO: improve this

namespace Star {

STAR_EXCEPTION(RpcPromiseException, StarException);

template <typename Result, typename Error>
class RpcPromise;

// The other side of an RpcPromise, can be used to either fulfill or fail a
// paired promise.  Call either fulfill or fail function exactly once, any
// further invocations will result in an exception.
template <typename Result, typename Error = String>
class RpcPromiseKeeper {
public:
  void chain(RpcPromise<Result,Error> promise);
  void fulfill(Result result);
  void fail(Error error);

private:
  template <typename ResultT, typename ErrorT>
  friend class RpcPromise;

  function<void(Variant<Result,RpcPromise<Result,Error>>)> m_fulfill;
  function<void(Error)> m_fail;
};

// Wraps an RpcPromiseKeeper, best used as a shared_ptr. Fails the promise on destruction.
template <typename Result, typename Error = String>
class AutoFailRpcPromiseKeeper {
public:
  void chain(RpcPromise<Result,Error> promise);
  void fulfill(Result result);
  void fail(Error error);

  AutoFailRpcPromiseKeeper(RpcPromiseKeeper<Result,Error> keeper);
  ~AutoFailRpcPromiseKeeper();
private:
  RpcPromiseKeeper<Result,Error> m_keeper;
  bool m_done = false;
};

template <typename Result, typename Error = String>
using AutoFailRpcPromiseKeeperPtr = shared_ptr<AutoFailRpcPromiseKeeper<Result,Error>>;

// A generic promise for the result of a remote procedure call.  It has
// reference semantics and is implicitly shared. Thread safe via mutexes. Can be chained.
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
  Maybe<Result> result() const;

  // Returns the error of a failed rpc call.  Returns nothing if the call is
  // successful or not yet finished.
  Maybe<Error> error() const;

  // Wrap this RpcPromise into another promise which returns instead the result
  // of this function when fulfilled
  template <typename Function>
  decltype(auto) wrap(Function function);

private:
  template <typename ResultT, typename ErrorT>
  friend class RpcPromise;

  struct Value {
    Mutex mutex;
    
    MVariant<Result,RpcPromise<Result,Error>> result;
    Maybe<Error> error;
  };

  function<Value*()> m_getValue;
};

template <typename Result, typename Error>
void RpcPromiseKeeper<Result, Error>::chain(RpcPromise<Result,Error> promise) {
  m_fulfill(std::move(promise));
}

template <typename Result, typename Error>
void RpcPromiseKeeper<Result, Error>::fulfill(Result result) {
  m_fulfill(std::move(result));
}

template <typename Result, typename Error>
void RpcPromiseKeeper<Result, Error>::fail(Error error) {
  m_fail(std::move(error));
}

template <typename Result, typename Error>
void AutoFailRpcPromiseKeeper<Result, Error>::chain(RpcPromise<Result,Error> promise) {
  m_done = true;
  m_keeper.chain(std::move(promise));
}

template <typename Result, typename Error>
void AutoFailRpcPromiseKeeper<Result, Error>::fulfill(Result result) {
  m_done = true;
  m_keeper.fulfill(std::move(result));
}

template <typename Result, typename Error>
void AutoFailRpcPromiseKeeper<Result, Error>::fail(Error error) {
  m_done = true;
  m_keeper.fail(std::move(error));
}

template <typename Result, typename Error>
AutoFailRpcPromiseKeeper<Result, Error>::AutoFailRpcPromiseKeeper(RpcPromiseKeeper<Result,Error> keeper) : m_keeper(keeper) {}

template <typename Result, typename Error>
AutoFailRpcPromiseKeeper<Result, Error>::~AutoFailRpcPromiseKeeper() {
  if (!m_done) {
    m_keeper.fail("Keeper destroyed");
  }
}

template <typename Result, typename Error>
pair<RpcPromise<Result, Error>, RpcPromiseKeeper<Result, Error>> RpcPromise<Result, Error>::createPair() {
  auto valuePtr = make_shared<Value>();

  RpcPromise promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };

  RpcPromiseKeeper<Result, Error> keeper;
  keeper.m_fulfill = [valuePtr](Variant<Result,RpcPromise<Result,Error>> result) {
    MutexLocker lock(valuePtr->mutex);
    if (valuePtr->result || valuePtr->error)
      throw RpcPromiseException("fulfill called on already finished RpcPromise");
    valuePtr->result = std::move(result);
  };
  keeper.m_fail = [valuePtr](Error error) {
    MutexLocker lock(valuePtr->mutex);
    if (valuePtr->result || valuePtr->error)
      throw RpcPromiseException("fail called on already finished RpcPromise");
    valuePtr->error = std::move(error);
  };

  return {std::move(promise), std::move(keeper)};
}

template <typename Result, typename Error>
RpcPromise<Result, Error> RpcPromise<Result, Error>::createFulfilled(Result result) {
  auto valuePtr = std::make_shared<Value>();
  valuePtr->result = std::move(result);

  RpcPromise<Result, Error> promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };
  return promise;
}

template <typename Result, typename Error>
RpcPromise<Result, Error> RpcPromise<Result, Error>::createFailed(Error error) {
  auto valuePtr = std::make_shared<Value>();
  valuePtr->error = std::move(error);

  RpcPromise<Result, Error> promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };
  return promise;
}

template <typename Result, typename Error>
bool RpcPromise<Result, Error>::finished() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  if (val->result.template is<RpcPromise<Result,Error>>()) {
    return val->result.template get<RpcPromise<Result,Error>>().finished();
  }
  return val->result || val->error;
}

template <typename Result, typename Error>
bool RpcPromise<Result, Error>::succeeded() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  if (val->result.template is<RpcPromise<Result,Error>>()) {
    return val->result.template get<RpcPromise<Result,Error>>().succeeded();
  }
  return !val->result.empty();
}

template <typename Result, typename Error>
bool RpcPromise<Result, Error>::failed() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  if (val->result.template is<RpcPromise<Result,Error>>()) {
    return val->result.template get<RpcPromise<Result,Error>>().failed();
  }
  return val->error.isValid();
}

template <typename Result, typename Error>
Maybe<Result> RpcPromise<Result, Error>::result() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  if (val->result.template is<RpcPromise<Result,Error>>()) {
    return val->result.template get<RpcPromise<Result,Error>>().result();
  }
  return val->result.template maybe<Result>();
}

template <typename Result, typename Error>
Maybe<Error> RpcPromise<Result, Error>::error() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  if (val->result.template is<RpcPromise<Result,Error>>()) {
    return val->result.template get<RpcPromise<Result,Error>>().error();
  }
  return val->error;
}

template <typename Result, typename Error>
template <typename Function>
decltype(auto) RpcPromise<Result, Error>::wrap(Function function) {
  typedef RpcPromise<typename std::decay<decltype(function(std::declval<Result>()))>::type, Error> WrappedPromise;
  WrappedPromise wrappedPromise;
  wrappedPromise.m_getValue = [wrapper = std::move(function), valuePtr = std::make_shared<typename WrappedPromise::Value>(), other = *this]() {
    MutexLocker lock(valuePtr->mutex);
    if (!valuePtr->result && !valuePtr->error) {
      if (other.succeeded())
        valuePtr->result = wrapper(*other.result());
      else if (other.failed()) {
        valuePtr->error = other.error();
      }
    }
    return valuePtr.get();
  };
  return wrappedPromise;
}

}
