---@meta

--- HTTP API for asynchronous HTTP requests
---@class http
http = {}

---@class HttpOptions
---@field headers table<string, string>|nil Optional. HTTP headers to include in the request
---@field body string|nil Optional. Request body content
---@field timeout number|nil Optional. Request timeout in seconds

---@class HttpResponse
---@field status number HTTP status code (e.g., 200, 404, 500)
---@field body string Response body content

---@class HttpPromise
---@field finished fun(self: HttpPromise): boolean Returns `true` if the HTTP request has completed
---@field succeeded fun(self: HttpPromise): boolean Returns `true` if the HTTP request completed successfully
---@field failed fun(self: HttpPromise): boolean Returns `true` if the HTTP request failed or was denied
---@field result fun(self: HttpPromise): HttpResponse|nil Returns the HTTP response table or `nil` if not finished or failed
---@field error fun(self: HttpPromise): string|nil Returns the error message if the request failed

--- Returns `true` if HTTP functionality is enabled in the configuration, `false` otherwise. ---
---@return boolean
function http.available() end

--- Creates and executes an asynchronous HTTP GET request to the specified URL. Returns an `HttpPromise` object that can be checked for completion. ---
---@param url string Target URL
---@param options HttpOptions|nil Optional request options (headers, body, timeout)
---@return HttpPromise
function http.get(url, options) end

--- Creates and executes an asynchronous HTTP POST request to the specified URL. Returns an `HttpPromise` object that can be checked for completion. ---
---@param url string Target URL
---@param options HttpOptions|nil Optional request options (headers, body, timeout)
---@return HttpPromise
function http.post(url, options) end

--- Creates and executes an asynchronous HTTP PUT request to the specified URL. Returns an `HttpPromise` object that can be checked for completion. ---
---@param url string Target URL
---@param options HttpOptions|nil Optional request options (headers, body, timeout)
---@return HttpPromise
function http.put(url, options) end

--- Creates and executes an asynchronous HTTP DELETE request to the specified URL. Returns an `HttpPromise` object that can be checked for completion. ---
---@param url string Target URL
---@param options HttpOptions|nil Optional request options (headers, body, timeout)
---@return HttpPromise
function http.delete(url, options) end

--- Creates and executes an asynchronous HTTP PATCH request to the specified URL. Returns an `HttpPromise` object that can be checked for completion. ---
---@param url string Target URL
---@param options HttpOptions|nil Optional request options (headers, body, timeout)
---@return HttpPromise
function http.patch(url, options) end

--- Creates and executes an asynchronous HTTP request with a custom method to the specified URL. Returns an `HttpPromise` object that can be checked for completion. ---
---@param method string HTTP method (e.g., "GET", "POST", "HEAD", "OPTIONS")
---@param url string Target URL
---@param options HttpOptions|nil Optional request options (headers, body, timeout)
---@return HttpPromise
function http.createRequest(method, url, options) end

--- Returns `true` if the specified domain is in the trusted domains list, `false` otherwise. ---
---@param domain string Domain to check (e.g., "example.com")
---@return boolean
function http.isTrusted(domain) end

