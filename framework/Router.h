#pragma once
#include <map>
#include <utility>
#include <functional>
#include <string>
#include <vector>
#include "Request.h"
#include "Response.h"
#include "Middleware.h"

struct RutaInfo{
    std::function<Response(const Request&)> handler;
    std::vector<Middleware> middleware;

    RutaInfo() {}
    RutaInfo(std::function<Response(const Request&)> h) : handler(h) {}
    RutaInfo(std::function<Response(const Request&)> h, std::vector<Middleware> m) : handler(h), middleware(m) {}
};

extern std::map<std::pair<std::string, std::string>, RutaInfo> routes;

Response route(Request& req, const std::string& metoda, const std::string& putanja);