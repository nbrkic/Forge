# Forge

Forge is a C++ backend framework, built from scratch as a learning project to understand how an HTTP server works under the hood — from raw TCP sockets to asynchronous I/O. It uses Windows IOCP (I/O Completion Ports), the same mechanism used by IIS and other production Windows servers.

**Note**: Forge was written for learning purposes, not as a replacement for a battle-tested framework like Express. It's Windows-only, and hasn't been production-tested the way tools with years of real-world use have.

## Features

- Asynchronous server built on IOCP (thread pool + completion-based I/O), with HTTP keep-alive (persistent connections) and automatic cleanup of idle connections
- Routing by path and HTTP method, including dynamic path parameters (`/users/:id`)
- Global middleware and per-route middleware, including the ability to short-circuit a request (e.g. for authentication)
- HTTP request parsing (headers, query strings, cookies, JSON/multipart body) and a `Response::json(...)` helper for building JSON responses
- Static file serving with automatic MIME type detection and streaming for large files
- Template engine (`{{variable}}` substitution in HTML files) with an `escapeHtml` helper against stored XSS
- Authentication building blocks: password hashing (PBKDF2 via Windows CNG), constant-time comparisons, CSRF protection, and ready-made register/login/logout handlers
- Persistent, database-backed sessions — survive server restarts and can be shared across multiple server instances pointed at the same database
- SQLite database (generic SQL queries with prepared statements against SQL injection; WAL mode and a busy timeout are enabled automatically for safe concurrent writes), plus `UpitBuilder` for building safe, dynamic search filters and pagination
- Configuration system (`config.json`, optional, with sensible defaults)
- Optional CORS support, for pairing Forge with a separately-hosted front-end (React, etc.)
- Per-route rate limiting (`ograniciBrzinu` middleware, sliding window by client IP)
- Logging system (levels, console + file)
- Multipart/form-data (file uploads, including multiple files per field), with `sacuvajSliku`/`sacuvajSlike` to safely validate (magic bytes, not the claimed `Content-Type`), recompress (via stb_image), and save uploaded images to disk in one call
- Chunked transfer encoding (both sending and receiving)
- Graceful shutdown (Ctrl+C)
- A small, dependency-free automated test suite
- Basic hardening: a top-level exception safety net (a bug in one request can't take down the whole server), path traversal protection on static files, and a maximum request size limit

## Requirements

- Windows
- Visual Studio Build Tools (workload "Desktop development with C++", with the "C++ CMake tools for Windows" component)
- CMake 3.10+

## Running

```
git clone <repo>
cd Forge
cmake -B build -G "NMake Makefiles"
cmake --build build
build\Forge.exe
```

To run the automated test suite instead of the server:

```
build\ForgeTests.exe
```

## Quick example

```cpp
#include "Forge.h"

using namespace std;

Response homePage(const Request& req){
    return Response(200, "OK", "Hello!");
}

int main(){
    routes[{"GET", "/"}] = RutaInfo(homePage);
    pokreniServer(8081);
    return 0;
}
```

## Project structure

```
main.cpp        <- your code, your routes
framework/      <- framework internals, no need to touch it
tests/          <- automated tests, built as a separate executable (ForgeTests)
public/         <- static files (CSS, images...), served at /static/...
views/          <- template files (.html with {{placeholder}} syntax)
config.json     <- optional, overrides default settings (see Configuration)
```

## Configuration

Forge has two ways to set server options, and they're meant to be combined:

```cpp
pokreniServer(8081); // visible, compiled-in default — works with no config file at all
```

If a `config.json` file exists next to the executable, it overrides the compiled-in default. Anything `config.json` doesn't specify falls back to the value passed to `pokreniServer()`, or to the framework's own defaults if it was called with no argument at all (`pokreniServer();`):

```json
{
    "port": 3000,
    "maxZahtevMB": 10,
    "trajanjeSesijeSekundi": 3600,
    "timeoutSekundi": 60,
    "corsOmogucen": false,
    "corsOrigin": "*",
    "rateLimitMaxZahteva": 10,
    "rateLimitProzorSekundi": 60,
    "kvalitetSlike": 80
}
```

| Setting | Default | Description |
|---|---|---|
| `port` | 8081 | TCP port the server listens on |
| `maxZahtevMB` | 10 | Maximum combined size (headers + body) of a single request, in megabytes |
| `trajanjeSesijeSekundi` | 3600 | How long a login session stays valid, in seconds |
| `timeoutSekundi` | 60 | How long an idle keep-alive connection stays open before being closed |
| `corsOmogucen` | `false` | Enables CORS headers on every response and answers `OPTIONS` preflight requests with `200 OK` — see [CORS](#cors) |
| `corsOrigin` | `"*"` | Value sent as `Access-Control-Allow-Origin` when CORS is enabled |
| `rateLimitMaxZahteva` | 10 | Max requests per client IP within the window, on routes using `ograniciBrzinu` — see [Rate limiting](#rate-limiting) |
| `rateLimitProzorSekundi` | 60 | Length of the rate-limit sliding window, in seconds |
| `kvalitetSlike` | 80 | JPEG quality (1-100) used when `sacuvajSliku`/`sacuvajSlike` recompress an uploaded image — see [File uploads](#file-uploads) |

This makes it easy to run multiple instances of the same executable with different settings (e.g. different ports) just by giving each one its own `config.json` — no recompiling needed.

## CORS

By default, CORS is off — Forge assumes the front-end is served by Forge itself (`public/`), same-origin, so no extra headers are needed. If your front-end runs separately (e.g. a React dev server on `http://localhost:3000`), enable it in `config.json`:

```json
{
    "corsOmogucen": true,
    "corsOrigin": "http://localhost:3000"
}
```

With this enabled, Forge answers every `OPTIONS` request with `200 OK` plus the CORS headers (handling preflight automatically), and adds the same headers to every other response — including ones a middleware short-circuits (e.g. a `401`/`403`).

`corsOrigin` must be a specific origin, not `"*"`, if the front-end needs to send cookies (`fetch(url, { credentials: "include" })`) — which it will, since Forge's session/CSRF system relies on the `session_id` cookie. Browsers reject the combination of a wildcard origin and credentials per the CORS spec.

## Rate limiting

`ograniciBrzinu` is a ready-made middleware that limits each client IP to `rateLimitMaxZahteva` requests per `rateLimitProzorSekundi`-second sliding window (both configurable — see [Configuration](#configuration)), returning `429 Too Many Requests` once exceeded. Apply it per-route, on endpoints where abuse actually matters (login, registration, creating posts) — not globally, since that would also throttle static assets and normal browsing:

```cpp
routes[{"POST", "/login"}] = RutaInfo(prijavaHandler, {ograniciBrzinu});
```

It tracks requests in memory, per server process, keyed by `req.ipAdresa`. If you run multiple server instances behind a load balancer, each instance enforces the limit independently, so the effective limit becomes (number of instances) × `rateLimitMaxZahteva`.

## Routes

```cpp
routes[{"GET", "/path"}] = RutaInfo(myHandler);
routes[{"POST", "/path"}] = RutaInfo(anotherHandler);
```

A handler has the signature `Response name(const Request& req)`.

Routes can also have their own middleware, which runs only for that route (after global middleware, before the handler):

```cpp
routes[{"GET", "/profile"}] = RutaInfo(profileHandler, {proveriAuth});
```

### Dynamic route parameters

A path segment starting with `:` matches anything and ends up in `req.params`:

```cpp
routes[{"GET", "/users/:id"}] = RutaInfo(getUserHandler);

Response getUserHandler(const Request& req){
    string id = req.params.at("id");
    return Response(200, "OK", "User " + id);
}
```

An exact match always wins over a dynamic one — if both `/users/me` and `/users/:id` are registered, a request for `/users/me` always hits the exact route, never `:id` with `id="me"`.

## Request object

| Field | Type | Description |
|---|---|---|
| `metoda` | `string` | HTTP method (GET, POST...) |
| `putanja` | `string` | request path |
| `headers` | `map<string,string>` | HTTP headers |
| `query` | `map<string,string>` | query string parameters |
| `body` | `string` | raw request body |
| `cookies` | `map<string,string>` | cookies sent with the request |
| `multipartPolja` | `map<string,string>` | text fields from a multipart form |
| `multipartFajlovi` | `map<string,vector<UploadFajl>>` | uploaded files, grouped by field name (each field can carry more than one file) |
| `params` | `map<string,string>` | dynamic route parameters, e.g. `req.params.at("id")` for a route registered as `/users/:id` |
| `ipAdresa` | `string` | client's IP address |

(Field names are in Serbian, matching the actual source code — see `framework/Request.h` for the exact names.)

## Response object

```cpp
Response(statusCode, statusText, body, contentType = "text/plain")
```

```cpp
Response response(200, "OK", "Hello!", "text/html");
response.setCookie("name", "value");
return response;
```

For JSON responses, `Response::json(...)` builds the body and sets the content type for you:

```cpp
return Response::json(200, "OK", {{"name", "Nemanja"}, {"age", 25}});
```

On the request side, `parsirajJson(req)` parses `req.body` and returns `std::optional<nlohmann::json>` (`std::nullopt` on invalid JSON, instead of throwing):

```cpp
auto podaci = parsirajJson(req);
if(!podaci){
    return Response(400, "Bad Request", "Invalid JSON");
}
string name = (*podaci)["name"];
```

## Middleware

```cpp
optional<Response> myMiddleware(const Request& req){
    // return a Response to SHORT-CIRCUIT the flow (e.g. 401 Unauthorized)
    // return nullopt to continue on
    return nullopt;
}

middlewares.push_back(myMiddleware);
```

Global middleware (registered with `middlewares.push_back(...)`) runs on every request, before routing happens. Route-specific middleware (passed to `RutaInfo`) runs only for that route, after global middleware, before the handler.

## File uploads

Multipart file uploads land in `req.multipartFajlovi` (`map<string,vector<UploadFajl>>`) — grouped by field name, as a **list**, so sending several files under the same field (e.g. multiple photos for one listing) doesn't overwrite previous ones. Each `UploadFajl` has `imeFajla`, `contentType` (as claimed by the client — not to be trusted) and `sadrzaj` (raw bytes as a string).

`sacuvajSliku(fajl, direktorijum = "public/uploads/")` handles the whole save-to-disk flow safely for one file: it checks the file's actual magic bytes (not the claimed `Content-Type`) via `proveriStvarniTipSlike`, generates a random filename with `generisiSalt()` (never trusts the client-supplied filename — that's a path traversal risk), writes it to disk, and returns the generated filename — or `""` if the content isn't a recognized image (JPEG/PNG/GIF) or the write failed. `sacuvajSlike(fajlovi, direktorijum = "public/uploads/")` does the same for a whole vector at once, silently skipping any invalid files and returning the filenames that were actually saved:

```cpp
Response uploadHandler(const Request& req){
    auto it = req.multipartFajlovi.find("slike");
    if(it == req.multipartFajlovi.end()){
        return Response(400, "Bad Request", "No files uploaded");
    }

    auto imenaFajlova = sacuvajSlike(it->second);
    if(imenaFajlova.empty()){
        return Response(415, "Unsupported Media Type", "No valid images");
    }

    std::vector<std::string> putanje;
    for(auto& ime : imenaFajlova){
        putanje.push_back("/static/uploads/" + ime);
    }

    return Response::json(201, "Created", {{"putanje", putanje}});
}
```

`public/uploads/` already exists in the repo (with a `.gitkeep`), ready to receive saved files — they're served automatically at `/static/uploads/...` like anything else in `public/`.

`sacuvajSliku`/`sacuvajSlike` also **recompress** every image before saving: the file is decoded and re-encoded as a JPEG at `kvalitetSlike` quality (default 80, configurable in `config.json`), which typically cuts file size by 50-90% for real photos with little visible quality loss. Because of this, saved files are always `.jpg` regardless of the original format (PNG/GIF included) — fine for photos, but means transparency and animation aren't preserved. This uses [stb_image](https://github.com/nothings/stb) (`stb_image.h`/`stb_image_write.h`, public domain) — vendored the same way as `sqlite3.c` and `json.hpp` — since decoding/encoding real image formats isn't something worth hand-rolling.

If you need the magic-bytes check on its own (without saving), use `proveriStvarniTipSlike(sadrzaj)` directly — it returns `"image/jpeg"`, `"image/png"`, `"image/gif"`, or `""`.

## Static files

Put files in `public/`, they're served at `/static/filename`. Files over 64KB are automatically streamed instead of being loaded into memory all at once. Requests attempting path traversal (`..`) are rejected with `403 Forbidden`.

## Templates

```cpp
map<string,string> data;
data["ime"] = "Nemanja";
string html = renderTemplate("views/my-template.html", data);
return Response(200, "OK", html, "text/html");
```

`renderTemplate` does plain `{{variable}}` substitution — no escaping, no loops/conditionals. That means two things in practice:

- **Escape untrusted data before it goes in.** Anything that came from a user (a listing description, a username) must go through `escapeHtml(...)` before being placed in the map, or it's a stored-XSS hole — the raw value gets substituted straight into the HTML response. Values you control yourself (numbers, hardcoded text) don't need it:
  ```cpp
  data["opis"] = escapeHtml(red["opis"]); // from the database / user input
  data["cena"] = red["cena"];             // a number, safe as-is
  ```
- **There's no loop syntax for repeated content** (e.g. a grid of listings). Build the repeated part yourself in C++ — render a small "card" template once per row and concatenate the results — then pass that already-built HTML string in as a single placeholder for an outer page template. Since it's already-rendered trusted HTML (not raw user input), it should NOT be passed through `escapeHtml` again:
  ```cpp
  string kartice;
  for(auto& red : izvrsiSelect("SELECT * FROM oglasi")){
      map<string,string> kartaPodaci;
      kartaPodaci["naziv"] = escapeHtml(red["marka"] + " " + red["model"]);
      kartaPodaci["cena"] = red["cena"];
      kartice += renderTemplate("views/oglas-karta.html", kartaPodaci);
  }

  map<string,string> stranicaPodaci;
  stranicaPodaci["oglasi"] = kartice; // pre-rendered HTML, not user input — don't escape
  string html = renderTemplate("views/pretraga.html", stranicaPodaci);
  ```

## Database (SQLite)

```cpp
initDatabase(); // call once, at startup

izvrsiUpit("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT)");
izvrsiUpit("INSERT INTO users (name) VALUES (?)", {"Nemanja"});

auto results = izvrsiSelect("SELECT * FROM users");
for(auto& row : results){
    // row["name"], row["id"], etc.
}
```

`initDatabase()` opens (or creates) `forge.db`, enables WAL mode and a 5-second busy timeout, and creates its own tables. Combined with an internal mutex around every query, this makes concurrent writes from multiple requests — e.g. several users creating records at the same time — safe without any extra work on your part.

### Search filters and pagination

`izvrsiSelect` takes raw SQL — there's no query builder/ORM — but for endpoints with several optional filters (only some of which the client actually sends), manually keeping the `WHERE` clause and its `?` parameters in sync is easy to get wrong. `UpitBuilder` is a small helper that couples each condition with its value, so that can't happen, and it can add `LIMIT`/`OFFSET` pagination on top:

```cpp
Response searchHandler(const Request& req){
    UpitBuilder query("SELECT * FROM oglasi");

    if(req.query.count("marka")){
        query.dodajUslov("marka = ?", req.query.at("marka"));
    }
    if(req.query.count("cenaOd")){
        query.dodajUslov("cena >= ?", req.query.at("cenaOd"));
    }
    if(req.query.count("cenaDo")){
        query.dodajUslov("cena <= ?", req.query.at("cenaDo"));
    }

    int page = req.query.count("strana") ? std::stoi(req.query.at("strana")) : 1;
    query.postaviStranicenje(20, page); // 20 results per page

    auto results = izvrsiSelect(query.izgradiSql(), query.getParametri());
    return Response::json(200, "OK", results);
}
```

`dodajUslov(condition, value)` adds one `AND`-ed condition and its parameter together — there's no way to add one without the other. `postaviStranicenje(perPage, page)` (1-indexed page numbers) appends `LIMIT ? OFFSET ?` with the right values. `izgradiSql()` and `getParametri()` always produce a matching SQL string and parameter list, in the same order.

## Authentication & Sessions

Forge ships both ready-made HTTP handlers for the common register/login/logout flow, and the lower-level building blocks in case you want different field names or response formats. Sessions are stored in SQLite rather than in memory, so they survive server restarts and can be shared by multiple server instances pointed at the same database file.

### Quick start (ready-made handlers)

```cpp
#include "Forge.h"

using namespace std;

int main(){
    initDatabase();
    initAuthTabela();
    initSesijeTabela();

    routes[{"POST", "/register"}] = RutaInfo(registracijaHandler);
    routes[{"POST", "/login"}] = RutaInfo(prijavaHandler);
    routes[{"POST", "/logout"}] = RutaInfo(odjavaHandler);
    routes[{"GET", "/profile"}] = RutaInfo(profileHandler, {proveriAuth});

    middlewares.push_back(proveriCsrf);

    pokreniServer(8081);
    return 0;
}
```

`registracijaHandler` and `prijavaHandler` expect a JSON body: `{"korisnickoIme": "...", "lozinka": "..."}`. On success, `prijavaHandler` sets a `session_id` cookie and returns a `csrfToken` in the JSON response body — clients must echo this back as an `X-CSRF-Token` header on subsequent state-changing requests (see below).

A protected route using `proveriAuth` as route-specific middleware, reading the logged-in user:

```cpp
Response profileHandler(const Request& req){
    string username;
    proveriSesiju(req.cookies.at("session_id"), username);
    return Response(200, "OK", "Hello, " + username + "!");
}
```

### Building blocks

| Function | From | Description |
|---|---|---|
| `initAuthTabela()` | `Auth.h` | Creates the `korisnici` (users) table |
| `initSesijeTabela()` | `Session.h` | Creates the `sesije` (sessions) table |
| `registrujKorisnika(username, password)` | `Auth.h` | Registers a new user; hashes the password (PBKDF2, 100,000 iterations, random salt) |
| `prijaviKorisnika(username, password)` | `Auth.h` | Verifies a username/password pair |
| `registracijaHandler`, `prijavaHandler`, `odjavaHandler` | `Auth.h` | Ready-made HTTP handlers wrapping the two functions above plus session creation/destruction |
| `kreirajSesiju(username)` | `Session.h` | Creates a session (persisted in SQLite), returns a session ID (valid for `trajanjeSesijeSekundi` seconds — see [Configuration](#configuration)) |
| `proveriSesiju(sessionId, username)` | `Session.h` | Checks if a session is valid; fills in `username` if so |
| `unistiSesiju(sessionId)` | `Session.h` | Destroys a session (logout) |
| `preuzmiCsrfToken(sessionId)` | `Session.h` | Returns the CSRF token tied to a session |
| `proveriAuth` | `Session.h` | Ready-made middleware — returns 401 if there's no valid session. Use as route-specific middleware. |
| `proveriCsrf` | `Session.h` | Ready-made middleware — returns 403 on state-changing (non-GET) requests missing a valid `X-CSRF-Token` header. Register globally. |

If you need different field names, a different response shape, or extra fields on registration, write your own handler using `registrujKorisnika`/`prijaviKorisnika`/`kreirajSesiju` directly instead of the ready-made handlers — they're a convenience, not a requirement.

### CSRF protection

If you register `proveriCsrf` as global middleware, clients must send the `csrfToken` (received from `prijavaHandler`) back as an `X-CSRF-Token` header on every state-changing (non-GET) request:

```
curl -b cookies.txt -X POST -H "X-CSRF-Token: <token>" http://localhost:8081/logout
```

Requests without a session cookie (e.g. `/register`, `/login` themselves) are not affected — CSRF only matters once a session exists.

## Logging

```cpp
log(NivoLoga::INFO, "Message");
log(NivoLoga::WARNING, "Warning");
log(NivoLoga::LOG_ERROR, "Error");
```

Printed to both the console and the `server.log` file.

## Testing

Forge ships a small, dependency-free test suite in `tests/test_main.cpp`, built as a separate executable (`ForgeTests`) that links the same framework code as `Forge.exe` but never starts a server:

```
cmake --build build
build\ForgeTests.exe
```

It uses a minimal custom assert helper (`assertTrue`/`assertEqual`, printing `[OK]`/`[FAIL]` per check) instead of an external test framework, and covers things like URL decoding, chunked-body decoding, password hashing and verification, constant-time comparison, MIME type detection, and concurrent database writes. The process exits with code `0` if everything passed and `1` otherwise, so it's safe to wire into a CI script.

## Security notes

- All unhandled exceptions thrown while processing a request are caught at the top level and turned into a `500 Internal Server Error` for that one request — a bug in a handler won't crash the server for everyone else.
- Static file paths are checked for `..` traversal attempts on every code path, including the large-file streaming path.
- Requests (headers + body combined) are capped at 10MB by default to guard against memory-exhaustion attacks; adjust `maxZahtev` in `config.json` (or the default in `framework/Config.h`) if you need a different limit.
- Idle keep-alive connections are automatically closed after `timeoutSekundi` (default 60s) to free up server resources. This is a TCP-connection-level cleanup only — it does not log the user out or affect their session; the browser transparently opens a new connection on the next request.
- CSRF tokens and password hashes are compared in constant time (`bezbednoUporedi`) rather than with `==`, to avoid leaking information through timing differences.
- Uploaded files can be checked against their real magic bytes (`proveriStvarniTipSlike`) rather than trusting the client-supplied `Content-Type` — see [File uploads](#file-uploads).
- Sensitive routes (login, registration, posting content) can be protected with the `ograniciBrzinu` rate-limiting middleware — see [Rate limiting](#rate-limiting). It isn't applied automatically to anything; you opt in per route.
- `renderTemplate` does not escape values — pass any user-supplied data through `escapeHtml(...)` before putting it in the template map, or it's a stored-XSS hole. See [Templates](#templates).
- Passwords are never stored in plaintext — see [Authentication & Sessions](#authentication--sessions).
- None of this makes Forge "production-hardened" on its own — see Known limitations below.

## Known limitations

- Windows-only (Winsock + IOCP), no Linux port
- No built-in HTTPS/TLS — use a reverse proxy (e.g. nginx) in front for production
- Rate limiting exists but is in-memory and per-process — not shared across multiple server instances behind a load balancer
- No role-based access control — sessions are just "logged in as this username," nothing more
- Built for learning — not production-tested to the level of established frameworks
