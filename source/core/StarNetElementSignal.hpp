#pragma once

#include "StarNetElement.hpp"

namespace Star {

// NetElement that sends signals during delta writes that can be received by
// slaves.  It has no 'state', and nothing is sent during a store / load, and
// it only keeps past signals for a maximum number of versions.  Thus, it is
// not appropriate to use to send updates to long term states, only for event
// like things that are not harmful if missed.
template <typename Signal>
class NetElementSignal : public NetElement {
public:
  NetElementSignal(size_t maxSignalQueue = 32);

  void initNetVersion(NetElementVersion const* version = nullptr) override;

  void netStore(DataStream& ds) const override;
  void netLoad(DataStream& ds) override;

  void enableNetInterpolation(float extrapolationHint = 0.0f) override;
  void disableNetInterpolation() override;
  void tickNetInterpolation(float dt) override;

  bool writeNetDelta(DataStream& ds, uint64_t fromVersion) const override;
  void readNetDelta(DataStream& ds, float interpolationTime = 0.0) override;

  void send(Signal signal);
  List<Signal> receive();

private:
  struct SignalEntry {
    uint64_t version;
    Signal signal;
    bool received;
  };

  size_t m_maxSignalQueue;
  NetElementVersion const* m_netVersion = nullptr;
  bool m_netInterpolationEnabled = false;
  Deque<SignalEntry> m_signals;
  Deque<pair<float, Signal>> m_pendingSignals;
};

template <typename Signal>
NetElementSignal<Signal>::NetElementSignal(size_t maxSignalQueue) {
  m_maxSignalQueue = maxSignalQueue;
}

template <typename Signal>
void NetElementSignal<Signal>::initNetVersion(NetElementVersion const* version) {
  m_netVersion = version;
  m_signals.clear();
}

template <typename Signal>
void NetElementSignal<Signal>::netStore(DataStream&) const {}

template <typename Signal>
void NetElementSignal<Signal>::netLoad(DataStream&) {
}

template <typename Signal>
void NetElementSignal<Signal>::enableNetInterpolation(float) {
  m_netInterpolationEnabled = true;
}

template <typename Signal>
void NetElementSignal<Signal>::disableNetInterpolation() {
  m_netInterpolationEnabled = false;
  for (auto& p : take(m_pendingSignals))
    send(std::move(p.second));
}

template <typename Signal>
void NetElementSignal<Signal>::tickNetInterpolation(float dt) {
  for (auto& p : m_pendingSignals)
    p.first -= dt;

  while (!m_pendingSignals.empty() && m_pendingSignals.first().first <= 0.0f)
    send(m_pendingSignals.takeFirst().second);
}

template <typename Signal>
bool NetElementSignal<Signal>::writeNetDelta(DataStream& ds, uint64_t fromVersion) const {
  size_t numToWrite = 0;
  for (auto const& p : m_signals) {
    if (p.version >= fromVersion)
      ++numToWrite;
  }
  if (numToWrite == 0)
    return false;

  ds.writeVlqU(numToWrite);

  for (auto const& p : m_signals) {
    if (p.version >= fromVersion)
      ds.write(p.signal);
  }

  return true;
}

template <typename Signal>
void NetElementSignal<Signal>::readNetDelta(DataStream& ds, float interpolationTime) {
  size_t numToRead = ds.readVlqU();
  for (size_t i = 0; i < numToRead; ++i) {
    Signal s;
    ds.read(s);
    if (m_netInterpolationEnabled && interpolationTime > 0.0f) {
      if (!m_pendingSignals.empty() && m_pendingSignals.last().first > interpolationTime) {
        for (auto& p : take(m_pendingSignals))
          send(std::move(p.second));
      }
      m_pendingSignals.append({interpolationTime, std::move(s)});
    } else {
      send(std::move(s));
    }
  }
}

template <typename Signal>
void NetElementSignal<Signal>::send(Signal signal) {
  m_signals.append({m_netVersion ? m_netVersion->current() : 0, signal, false});
  while (m_signals.size() > m_maxSignalQueue)
    m_signals.removeFirst();
}

template <typename Signal>
List<Signal> NetElementSignal<Signal>::receive() {
  List<Signal> received;
  for (auto& p : m_signals) {
    if (!p.received) {
      received.append(p.signal);
      p.received = true;
    }
  }
  return received;
}

}
