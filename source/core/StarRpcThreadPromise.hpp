#ifndef STAR_RPC_THREAD_PROMISE_HPP
#define STAR_RPC_THREAD_PROMISE_HPP

#include "StarEither.hpp"
#include "StarString.hpp"
#include "StarThread.hpp"

// A thread-safe version of RpcPromise.
// This is just a copy-paste with a Mutex tacked on. I don't like that, but it's 11 PM.

namespace Star {

STAR_EXCEPTION(RpcThreadPromiseException, StarException);

template <typename Result, typename Error = String>
class RpcThreadPromiseKeeper {
public:
  void fulfill(Result result);
  void fail(Error error);

private:
  template <typename ResultT, typename ErrorT>
  friend class RpcThreadPromise;

  function<void(Result)> m_fulfill;
  function<void(Error)> m_fail;
};

template <typename Result, typename Error = String>
class RpcThreadPromise {
public:
  static pair<RpcThreadPromise, RpcThreadPromiseKeeper<Result, Error>> createPair();
  static RpcThreadPromise createFulfilled(Result result);
  static RpcThreadPromise createFailed(Error error);

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

private:
  template <typename ResultT, typename ErrorT>
  friend class RpcThreadPromise;

  struct Value {
    Mutex mutex;

    Maybe<Result> result;
    Maybe<Error> error;
  };

  RpcThreadPromise() = default;

  function<Value*()> m_getValue;
};

template <typename Result, typename Error>
void RpcThreadPromiseKeeper<Result, Error>::fulfill(Result result) {
  m_fulfill(move(result));
}

template <typename Result, typename Error>
void RpcThreadPromiseKeeper<Result, Error>::fail(Error error) {
  m_fail(move(error));
}

template <typename Result, typename Error>
pair<RpcThreadPromise<Result, Error>, RpcThreadPromiseKeeper<Result, Error>> RpcThreadPromise<Result, Error>::createPair() {
  auto valuePtr = make_shared<Value>();

  RpcThreadPromise promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };

  RpcThreadPromiseKeeper<Result, Error> keeper;
  keeper.m_fulfill = [valuePtr](Result result) {
    MutexLocker lock(valuePtr->mutex);
    if (valuePtr->result || valuePtr->error)
      throw RpcThreadPromiseException("fulfill called on already finished RpcThreadPromise");
    valuePtr->result = move(result);
  };
  keeper.m_fail = [valuePtr](Error error) {
    MutexLocker lock(valuePtr->mutex);
    if (valuePtr->result || valuePtr->error)
      throw RpcThreadPromiseException("fail called on already finished RpcThreadPromise");
    valuePtr->error = move(error);
  };

  return {move(promise), move(keeper)};
}

template <typename Result, typename Error>
RpcThreadPromise<Result, Error> RpcThreadPromise<Result, Error>::createFulfilled(Result result) {
  auto valuePtr = make_shared<Value>();
  valuePtr->result = move(result);

  RpcThreadPromise<Result, Error> promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };
  return promise;
}

template <typename Result, typename Error>
RpcThreadPromise<Result, Error> RpcThreadPromise<Result, Error>::createFailed(Error error) {
  auto valuePtr = make_shared<Value>();
  valuePtr->error = move(error);

  RpcThreadPromise<Result, Error> promise;
  promise.m_getValue = [valuePtr]() {
    return valuePtr.get();
  };
  return promise;
}

template <typename Result, typename Error>
bool RpcThreadPromise<Result, Error>::finished() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  return val->result || val->error;
}

template <typename Result, typename Error>
bool RpcThreadPromise<Result, Error>::succeeded() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  return val->result.isValid();
}

template <typename Result, typename Error>
bool RpcThreadPromise<Result, Error>::failed() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  return val->error.isValid();
}

template <typename Result, typename Error>
Maybe<Result> RpcThreadPromise<Result, Error>::result() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  return val->result;
}

template <typename Result, typename Error>
Maybe<Error> RpcThreadPromise<Result, Error>::error() const {
  auto val = m_getValue();
  MutexLocker lock(val->mutex);
  return val->error;
}

}

#endif
