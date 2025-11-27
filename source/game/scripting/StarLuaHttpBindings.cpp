#include "StarLuaHttpBindings.hpp"

#include "StarConfiguration.hpp"
#include "StarException.hpp"
#include "StarFormat.hpp"
#include "StarHttpClient.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarRoot.hpp"
#include "StarRpcPromise.hpp"
namespace Star {

struct LuaHttpResponse {
  int statusCode = 0;
  String body;
};

}// namespace Star

namespace Star {

namespace {

// Global callback for requesting trust
Mutex s_trustCallbackMutex;
LuaBindings::HttpTrustRequestCallback s_trustRequestCallback;

//  struct
struct PendingHttpRequest {
  HttpRequest httpRequest;
  RpcPromiseKeeper<LuaHttpResponse> rpcKeeper;
  String domain;
};

// reequest queue
Mutex s_pendingRequestsMutex;
List<shared_ptr<PendingHttpRequest>> s_pendingRequests;

// Structure to hold async HTTP request state
struct AsyncHttpRequest {
  shared_ptr<WorkerPoolPromise<HttpResponse>> workerPromise;
  RpcPromiseKeeper<LuaHttpResponse> rpcKeeper;
  bool checked = false;
};

// Global map to track pending async requests
// We use a simple incremental ID system
Mutex s_asyncRequestsMutex;
uint64_t s_nextRequestId = 1;
HashMap<uint64_t, shared_ptr<AsyncHttpRequest>> s_asyncRequests;

// Check if a domain is in the trusted list
bool isTrustedDomain(String const& domain) {
  auto& root = Root::singleton();
  const auto config = root.configuration();
  if (auto trustedSites = config->getPath("safe.luaHttp.trustedSites").optArray()) {
    for (auto const& site : *trustedSites) {
      if (site.toString() == domain)
        return true;
    }
  }
  return false;
}

// Add domain to trusted list
void addTrustedDomain(String const& domain) {
  auto& root = Root::singleton();
  auto config = root.configuration();

  JsonArray trustedSites;
  if (auto existing = config->getPath("safe.luaHttp.trustedSites").optArray())
    trustedSites = *existing;

  // Check if already exists
  for (auto const& site : trustedSites) {
    if (site.toString() == domain)
      return;
  }

  trustedSites.append(domain);
  config->setPath("safe.luaHttp.trustedSites", trustedSites);
}

String extractDomain(String const& url) {
  const size_t end = url.find("://");
  if (end == NPos)
    return url;

  const size_t domainStart = end + 3;
  const size_t pathStart = url.find('/', domainStart);
  const size_t portStart = url.find(':', domainStart);

  size_t domainEnd = NPos;
  if (pathStart != NPos && portStart != NPos)
    domainEnd = std::min(pathStart, portStart);
  else if (pathStart != NPos)
    domainEnd = pathStart;
  else if (portStart != NPos)
    domainEnd = portStart;

  if (domainEnd == NPos)
    return url.substr(domainStart);
  return url.substr(domainStart, domainEnd - domainStart);
}

void pollAsyncRequests() {
  MutexLocker locker(s_asyncRequestsMutex);

  List<uint64_t> completedIds;

  for (auto& [fst, snd] : s_asyncRequests) {

    if (const auto& asyncReq = snd; !asyncReq->checked && asyncReq->workerPromise->poll()) {
      asyncReq->checked = true;

      try {

        if (const auto httpResp = asyncReq->workerPromise->get(); !httpResp.error.empty()) {
          asyncReq->rpcKeeper.fail(strf("HTTP request failed: {}", httpResp.error));
        } else {
          LuaHttpResponse luaResp;
          luaResp.statusCode = httpResp.statusCode;
          luaResp.body = httpResp.body;
          asyncReq->rpcKeeper.fulfill(std::move(luaResp));
        }
      } catch (std::exception const& e) {
        asyncReq->rpcKeeper.fail(strf("HTTP request exception: {}", e.what()));
      }

      completedIds.append(fst);
    }
  }

  for (auto id : completedIds) {
    s_asyncRequests.remove(id);
  }
}

void executeHttpRequest(HttpRequest const& httpReq, RpcPromiseKeeper<LuaHttpResponse> rpcKeeper) {
  try {
    const auto workerPromise = make_shared<WorkerPoolPromise<HttpResponse>>(HttpClient::requestAsync(httpReq));

    const auto asyncReq = make_shared<AsyncHttpRequest>();
    asyncReq->workerPromise = workerPromise;
    asyncReq->rpcKeeper = std::move(rpcKeeper);

    {
      MutexLocker locker(s_asyncRequestsMutex);
      const uint64_t requestId = s_nextRequestId++;
      s_asyncRequests[requestId] = asyncReq;
    }
  } catch (std::exception const& e) {
    rpcKeeper.fail(strf("HTTP request failed: {}", e.what()));
  }
}

// Handle usrs reply to trust dialog
void handleTrustReply(String const& domain, bool allowed) {
  MutexLocker locker(s_pendingRequestsMutex);

  List<shared_ptr<PendingHttpRequest>> remainingRequests;

  for (auto& pendingReq : s_pendingRequests) {
    if (pendingReq->domain == domain) {
      if (allowed) {
        //  request
        executeHttpRequest(pendingReq->httpRequest, std::move(pendingReq->rpcKeeper));
      } else {
        // Deny
        pendingReq->rpcKeeper.fail(strf("HTTP request to domain '{}' denied by user", domain));
      }
    } else {
      // Keep
      remainingRequests.append(pendingReq);
    }
  }

  s_pendingRequests = std::move(remainingRequests);
}

}

//  RpcPromise<LuaHttpResponse> for PromiseKeeper
template <>
struct LuaUserDataMethods<RpcPromise<LuaHttpResponse>> {
  static LuaMethods<RpcPromise<LuaHttpResponse>> make() {
    LuaMethods<RpcPromise<LuaHttpResponse>> methods;

    methods.registerMethod("finished", [](RpcPromise<LuaHttpResponse> const& promise) -> bool {
      pollAsyncRequests();
      return promise.finished();
    });

    methods.registerMethod("succeeded", [](RpcPromise<LuaHttpResponse> const& promise) -> bool {
      pollAsyncRequests();
      return promise.succeeded();
    });

    methods.registerMethod("failed", [](RpcPromise<LuaHttpResponse> const& promise) -> bool {
      pollAsyncRequests();
      return promise.failed();
    });

    methods.registerMethod("result", [](RpcPromise<LuaHttpResponse> const& promise) -> Maybe<LuaHttpResponse> {
      pollAsyncRequests();
      return promise.result();
    });

    methods.registerMethod("error", [](RpcPromise<LuaHttpResponse> const& promise) -> Maybe<String> {
      pollAsyncRequests();
      return promise.error();
    });

    return methods;
  }
};

template <>
struct LuaConverter<LuaHttpResponse> {
  static LuaValue from(LuaEngine& engine, LuaHttpResponse const& response) {
    auto table = engine.createTable();
    table.set("status", response.statusCode);
    table.set("body", response.body);
    return table;
  }

  static Maybe<LuaHttpResponse> to(LuaEngine& /*engine*/, LuaValue const& v) {
    if (const auto table = v.ptr<LuaTable>()) {
      LuaHttpResponse response;
      response.statusCode = table->get<Maybe<int>>("status").value(0);
      if (table->contains("body"))
        response.body = table->get<String>("body");
      return response;
    }
    return {};
  }
};

LuaCallbacks LuaBindings::makeHttpCallbacks(bool enabled) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("available", [enabled]() { return enabled; });

  auto makeFailurePromise = [](String message) -> RpcPromise<LuaHttpResponse> {
    return RpcPromise<LuaHttpResponse>::createFailed(std::move(message));
  };

  auto requestForMethod = [enabled](LuaEngine& engine, String const& method, String const& url, Maybe<LuaTable> const& options) -> RpcPromise<LuaHttpResponse> {
    if (!enabled)
      return RpcPromise<LuaHttpResponse>::createFailed("luaHttp disabled by configuration");

    const String domain = extractDomain(url);

    try {
      HttpRequest httpReq;
      httpReq.method = method;
      httpReq.url = url;

      // Parse options if provided
      if (options) {
        const auto& optTable = *options;

        // Extract headers
        if (optTable.contains("headers")) {
          auto headersValue = optTable.get("headers");
          if (const auto headersTable = headersValue.ptr<LuaTable>()) {
            headersTable->iterate([&](LuaValue key, LuaValue value) {
              if (const auto keyStr = key.ptr<LuaString>()) {
                if (const auto valStr = value.ptr<LuaString>()) {
                  httpReq.headers[keyStr->toString()] = valStr->toString();
                } else {
                  httpReq.headers[keyStr->toString()] = engine.luaTo<String>(value);
                }
              }
            });
          }
        }

        if (optTable.contains("body")) {
          httpReq.body = optTable.get<String>("body");
        }

        if (optTable.contains("timeout")) {
          httpReq.timeout = optTable.get<int>("timeout");
        }
      }

      // Create RpcPromise pair
      auto promisePair = RpcPromise<LuaHttpResponse>::createPair();

      // if domain is trusted
      if (isTrustedDomain(domain)) {
        //  is trasted execute
        executeHttpRequest(httpReq, std::move(promisePair.second));
      } else {
        // NOT trastedadded to pending queue
        auto pendingReq = make_shared<PendingHttpRequest>();
        pendingReq->httpRequest = httpReq;
        pendingReq->rpcKeeper = std::move(promisePair.second);
        pendingReq->domain = domain;

        {
          MutexLocker locker(s_pendingRequestsMutex);
          s_pendingRequests.append(pendingReq);
        }

        MutexLocker locker(s_trustCallbackMutex);
        if (s_trustRequestCallback) {
          s_trustRequestCallback(domain);
        } else {
          Logger::warn("HTTP request to untrusted domain '{}' but no trust dialog handler available", domain);
        }
      }

      return promisePair.first;
    } catch (std::exception const& e) {
      return RpcPromise<LuaHttpResponse>::createFailed(strf("HTTP request failed: {}", e.what()));
    }
  };

  callbacks.registerCallback("createRequest", [requestForMethod](LuaEngine& engine, String const& method, String const& url, Maybe<LuaTable> const& options) {
    return requestForMethod(engine, method, url, options);
  });

  callbacks.registerCallback("get", [requestForMethod](LuaEngine& engine, String const& url, Maybe<LuaTable> const& options) {
    return requestForMethod(engine, "GET", url, options);
  });

  callbacks.registerCallback("post", [requestForMethod](LuaEngine& engine, String const& url, Maybe<LuaTable> const& options) {
    return requestForMethod(engine, "POST", url, options);
  });
  callbacks.registerCallback("put", [requestForMethod](LuaEngine& engine, String const& url, Maybe<LuaTable> const& options) {
    return requestForMethod(engine, "PUT", url, options);
  });
  callbacks.registerCallback("delete", [requestForMethod](LuaEngine& engine, String const& url, Maybe<LuaTable> const& options) {
    return requestForMethod(engine, "DELETE", url, options);
  });
  callbacks.registerCallback("patch", [requestForMethod](LuaEngine& engine, String const& url, Maybe<LuaTable> const& options) {
    return requestForMethod(engine, "PATCH", url, options);
  });

  callbacks.registerCallback("isTrusted", [](String const& domain) -> bool {
    return isTrustedDomain(domain);
  });

  return callbacks;
}

void LuaBindings::setHttpTrustRequestCallback(HttpTrustRequestCallback callback) {
  MutexLocker locker(s_trustCallbackMutex);
  s_trustRequestCallback = std::move(callback);
}

void LuaBindings::clearHttpTrustRequestCallback() {
  MutexLocker locker(s_trustCallbackMutex);
  s_trustRequestCallback = {};
}

void LuaBindings::handleHttpTrustReply(String const& domain, const bool allowed) {
  handleTrustReply(domain, allowed);
}

}
