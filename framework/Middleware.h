#pragma once

#include <functional>
#include <vector>
#include "Request.h"
#include <optional>
#include "Response.h"

using Middleware = std::function<std::optional<Response>(const Request&)>;

extern std::vector<Middleware> middlewares;

std::optional<Response> logRequest(const Request& req);