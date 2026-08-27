#include "Middleware.h"
#include <iostream>
#include <optional>
#include "Response.h"
#include "Logger.h"

std::vector<Middleware> middlewares;

std::optional<Response> logRequest(const Request& req){
    log(NivoLoga::INFO, req.metoda + " " + req.putanja);
    return std::nullopt;
}
