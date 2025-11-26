#pragma once

#include "StarString.hpp"
#include "StarMap.hpp"
#include "StarMaybe.hpp"
#include "StarWorkerPool.hpp"

namespace Star {

struct HttpRequest {
  String method;
  String url;
  StringMap<String> headers;
  String body;

  int timeout = 30; // 0 - no timeout. NOT RECOMENDED YOU HEAR ME??
};

struct HttpResponse {
  int statusCode = 0;
  StringMap<String> headers;
  String body;
  String error;
};

class HttpClient {
public:
  HttpClient();
  ~HttpClient();

  static WorkerPoolPromise<HttpResponse> requestAsync(HttpRequest const& request);

  static WorkerPoolPromise<HttpResponse> getAsync(String const& url, StringMap<String> const& headers = {});
  static WorkerPoolPromise<HttpResponse> postAsync(String const& url, String const& body, StringMap<String> const& headers = {});
  static WorkerPoolPromise<HttpResponse> putAsync(String const& url, String const& body, StringMap<String> const& headers = {});
  static WorkerPoolPromise<HttpResponse> deleteAsync(String const& url, StringMap<String> const& headers = {});
  static WorkerPoolPromise<HttpResponse> patchAsync(String const& url, String const& body, StringMap<String> const& headers = {});

private:
  static WorkerPool& workerPool();
};

}

