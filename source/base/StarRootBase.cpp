#include "StarRootBase.hpp"

namespace Star {
  atomic<RootBase*> RootBase::s_singleton;

  RootBase* RootBase::singletonPtr() {
    return s_singleton.load();
  }

  RootBase& RootBase::singleton() {
    auto ptr = s_singleton.load();
    if (!ptr)
      throw RootException("RootBase::singleton() called with no Root instance available");
    else
      return *ptr;
  }

  RootBase::RootBase() {
    RootBase* oldRoot = nullptr;
    if (!s_singleton.compare_exchange_strong(oldRoot, this))
      throw RootException("Singleton Root has been constructed twice");
  }
}