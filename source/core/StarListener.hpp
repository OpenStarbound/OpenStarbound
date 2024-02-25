#pragma once

#include "StarThread.hpp"

namespace Star {

STAR_CLASS(Listener);
STAR_CLASS(CallbackListener);
STAR_CLASS(TrackerListener);
STAR_CLASS(ListenerGroup);

class Listener {
public:
  virtual ~Listener();
  virtual void trigger() = 0;
};

class CallbackListener : public Listener {
public:
  CallbackListener(function<void()> callback);

protected:
  virtual void trigger() override;

private:
  function<void()> callback;
};

class TrackerListener : public Listener {
public:
  TrackerListener();

  bool pullTriggered();

protected:
  virtual void trigger() override;

private:
  atomic<bool> triggered;
};

class ListenerGroup {
public:
  void addListener(ListenerWeakPtr listener);
  void removeListener(ListenerWeakPtr listener);
  void clearExpiredListeners();
  void clearAllListeners();

  void trigger();

private:
  Mutex m_mutex;
  std::set<ListenerWeakPtr, std::owner_less<ListenerWeakPtr>> m_listeners;
};

inline bool TrackerListener::pullTriggered() {
  return triggered.exchange(false);
}

inline void TrackerListener::trigger() {
  triggered = true;
}

}
