#include "StarListener.hpp"

namespace Star {

Listener::~Listener() {}

CallbackListener::CallbackListener(function<void()> callback)
  : callback(std::move(callback)) {}

void CallbackListener::trigger() {
  if (callback)
    callback();
}

TrackerListener::TrackerListener() : triggered(false) {}

void ListenerGroup::addListener(ListenerWeakPtr listener) {
  MutexLocker locker(m_mutex);
  m_listeners.insert(std::move(listener));
}

void ListenerGroup::removeListener(ListenerWeakPtr listener) {
  MutexLocker locker(m_mutex);
  m_listeners.erase(std::move(listener));
}

void ListenerGroup::clearExpiredListeners() {
  MutexLocker locker(m_mutex);
  eraseWhere(m_listeners, mem_fn(&ListenerWeakPtr::expired));
};

void ListenerGroup::clearAllListeners() {
  MutexLocker locker(m_mutex);
  m_listeners.clear();
}

void ListenerGroup::trigger() {
  MutexLocker locker(m_mutex);
  filter(m_listeners, [](ListenerWeakPtr const& wl) {
      if (auto lock = wl.lock()) {
        lock->trigger();
        return true;
      } else {
        return false;
      }
    });
}

}
