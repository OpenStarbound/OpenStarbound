#include "StarJsonRpc.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarLogging.hpp"

namespace Star {

JsonRpcInterface::~JsonRpcInterface() {}

JsonRpc::JsonRpc() {
  m_requestId = 0;
}

void JsonRpc::registerHandler(String const& handler, JsonRpcRemoteFunction func) {
  if (m_handlers.contains(handler))
    throw JsonRpcException(strf("Handler by that name already exists '%s'", handler));
  m_handlers.add(move(handler), move(func));
}

void JsonRpc::registerHandlers(JsonRpcHandlers const& handlers) {
  for (auto const& pair : handlers)
    registerHandler(pair.first, pair.second);
}

void JsonRpc::removeHandler(String const& handler) {
  if (!m_handlers.contains(handler))
    throw JsonRpcException(strf("No such handler by the name '%s'", handler));

  m_handlers.remove(handler);
}

void JsonRpc::clearHandlers() {
  m_handlers.clear();
}

RpcPromise<Json> JsonRpc::invokeRemote(String const& handler, Json const& arguments) {
  uint64_t id = m_requestId++;
  JsonObject request;
  m_pending.append(JsonObject{
      {"command", "request"},
      {"id", id},
      {"handler", handler},
      {"arguments", arguments}
    });

  auto pair = RpcPromise<Json>::createPair();
  m_pendingResponse.add(id, pair.second);

  return pair.first;
}

bool JsonRpc::sendPending() const {
  return !m_pending.empty();
}

ByteArray JsonRpc::send() {
  if (m_pending.empty())
    return {};

  DataStreamBuffer buffer;
  buffer.writeContainer(m_pending);
  m_pending.clear();

  return buffer.takeData();
}

void JsonRpc::receive(ByteArray const& inbuffer) {
  if (inbuffer.empty())
    return;

  DataStreamBuffer buffer(inbuffer);
  List<Json> inbound;
  buffer.readContainer(inbound);

  for (auto request : inbound) {
    if (request.get("command") == "request") {
      try {
        auto handlerName = request.getString("handler");
        if (!m_handlers.contains(handlerName))
          throw JsonRpcException(strf("Unknown handler '%s'", handlerName));
        m_pending.append(JsonObject{
            {"command", "response"},
            {"id", request.get("id")},
            {"result", m_handlers[handlerName](request.get("arguments"))}
          });
      } catch (std::exception& e) {
        Logger::error("Exception while handling variant rpc request handler call. %s", outputException(e, false));
        JsonObject response;
        response["command"] = "fail";
        response["id"] = request.get("id");
        m_pending.append(JsonObject{
            {"command", "fail"},
            {"id", request.get("id")}
          });
      }

    } else if (request.get("command") == "response") {
      try {
        auto responseHandler = m_pendingResponse.take(request.getUInt("id"));
        responseHandler.fulfill(request.get("result"));
      } catch (std::exception& e) {
        Logger::error("Exception while handling variant rpc response handler call. %s", outputException(e, true));
      }

    } else if (request.get("command") == "fail") {
      try {
        auto responseHandler = m_pendingResponse.take(request.getUInt("id"));
        responseHandler.fulfill({});
      } catch (std::exception& e) {
        Logger::error("Exception while handling variant rpc failure handler call. %s", outputException(e, true));
      }
    }
  }
}

}
