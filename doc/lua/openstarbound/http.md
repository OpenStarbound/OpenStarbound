# http

The `http` table provides asynchronous HTTP request functionality for Lua scripts. All HTTP requests are executed
asynchronously and return promises that can be checked for completion.

For security, domains must be explicitly trusted before requests can be made to them. Requests to untrusted domains will
trigger a trust dialog (if configured) before execution.

---

#### `bool` http.available()

Returns `true` if HTTP functionality is enabled in the configuration, `false` otherwise.

---

#### `HttpPromise` http.get(`String` url, [`Table` options])

Creates and executes an asynchronous HTTP GET request to the specified URL.

**Options table fields:**

* `headers` - `Table<String, String>` - Optional. HTTP headers to include in the request
* `timeout` - `int` - Optional. Request timeout in milliseconds
* `body` - `String` - Optional. Request body (not typically used with GET)

Returns an `HttpPromise` object that can be checked for completion.

**Example:**

```lua
local promise = http.get("https://example.com/api/data", {
  headers = { ["User-Agent"] = "MyScript/1.0" },
  timeout = 5000
})
```

---

#### `HttpPromise` http.post(`String` url, [`Table` options])

Creates and executes an asynchronous HTTP POST request to the specified URL.

Returns an `HttpPromise` object that can be checked for completion.

---

#### `HttpPromise` http.put(`String` url, [`Table` options])

Creates and executes an asynchronous HTTP PUT request to the specified URL.

Returns an `HttpPromise` object that can be checked for completion.

---

#### `HttpPromise` http.delete(`String` url, [`Table` options])

Creates and executes an asynchronous HTTP DELETE request to the specified URL.


Returns an `HttpPromise` object that can be checked for completion.

---

#### `HttpPromise` http.patch(`String` url, [`Table` options])

Creates and executes an asynchronous HTTP PATCH request to the specified URL.

Returns an `HttpPromise` object that can be checked for completion.

---

#### `HttpPromise` http.createRequest(`String` method, `String` url, [`Table` options])

Creates and executes an asynchronous HTTP request with a custom method to the specified URL.

Returns an `HttpPromise` object that can be checked for completion.

**Example:**

```lua
local promise = http.createRequest("PUT", "https://example.com/resource")
```

---

#### `bool` http.isTrusted(`String` domain)

Returns `true` if the specified domain is in the trusted domains list, `false` otherwise.

**Example:**

```lua
if http.isTrusted("example.com") then
  sb.logInfo("example.com is trusted")
end
```

---

## Security and Trust

HTTP requests require domain trust for security. When a script attempts to make a request to an untrusted domain:

1. The request is queued and not immediately executed
2. A trust dialog is shown to the user (if configured)
3. If the user approves, the domain is added to the trusted list and the request executes
4. If the user denies, the request fails with an error

Trusted domains are stored in the configuration under `safe.luaHttp.trustedSites`.

---

## Complete Example

### PromiseKeeper Example

```lua
require("/scripts/messageutil.lua")

function init()
    if not http.available() then
        sb.logError("HTTP is not enabled!")
        script.setUpdateDelta(0)
    else
        promises:add(http.get("https://api.example.com/data", {
            headers = {
                ["User-Agent"] = "Starbound-Script/1.0",
                ["Accept"] = "application/json"
            },
            body = '{"query":"example"}',
            timeout = 10 -- seconds
        }),
                handleSuccess,
                handleError
        )
    end
end

function handleSuccess(response)
    if response.status == 200 then
        sb.logInfo("Success! Body: %s", response.body)
        -- Process JSON or other data
    else
        sb.logWarn("Request returned status %s", response.status)
    end
end

function handleError(errorMessage)
    sb.logError("Request failed: %s", sb.print(errorMessage))
end

function update(...)
    promises:update()
end
```

