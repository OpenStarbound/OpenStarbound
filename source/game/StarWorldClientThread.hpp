#pragma once

#include "StarWorldClient.hpp"
#include "StarThread.hpp"
#include "StarRpcThreadPromise.hpp"

namespace Star {

STAR_CLASS(WorldClientThread);

// Runs a WorldThreadedClient in a separate thread.
class WorldClientThread : public Thread {
public:
  struct Message {
    String message;
    JsonArray args;
    RpcThreadPromiseKeeper<Json> promise;
  };

  typedef function<void(WorldClientThread*, WorldClient*)> WorldClientAction;

  WorldClientThread(ClientSubWorldId subWorldId);
  ~WorldClientThread();

  ClientSubWorldId subWorldId() const;

  void start();
  // Signals the WorldClientThread to stop and then joins it
  void stop();
  void setPause(shared_ptr<const atomic<bool>> pause);

  // An exception occurred from the actual WorldThreadedClient itself and the
  // WorldClientThread has stopped running.
  bool errorOccurred();
  bool shouldExpire();

  // Clients that have caused an error with incoming packets are removed from
  // the world and no further packets are handled from them.  They are still
  // added to this WorldClientThread, and must be removed and the final
  // outgoing packets should be sent to them.

  void pushIncomingPackets(List<PacketPtr> packets);
  List<PacketPtr> pullOutgoingPackets();

  // Executes the given action on the world in a thread safe context.  This
  // does *not* catch exceptions thrown by the action or set the error
  // flag.
  void executeAction(WorldClientAction action);

  // If a callback is set here, then this is called after every world update,
  // also in a thread safe context.
  void setUpdateAction(WorldClientAction updateAction);

  // 
  void passMessage(Message&& message);

protected:
  virtual void run();

private:
  void update();
  void sync();

  mutable RecursiveMutex m_mutex;

  WorldClientPtr m_worldClient;
  ClientSubWorldId m_subWorldId;
  WorldClientAction m_updateAction;

  mutable RecursiveMutex m_queueMutex;
  List<PacketPtr> m_incomingPacketQueue;
  List<PacketPtr> m_outgoingPacketQueue;

  mutable RecursiveMutex m_messageMutex;
  List<Message> m_messages;

  atomic<bool> m_stop;
  shared_ptr<const atomic<bool>> m_pause;
  mutable atomic<bool> m_errorOccurred;
  mutable atomic<bool> m_shouldExpire;
};

}
