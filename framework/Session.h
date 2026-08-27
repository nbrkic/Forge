#pragma once
#include <string>
#include <optional>
#include "Request.h"
#include "Response.h"

std::optional<Response> proveriAuth(const Request& req);
std::optional<Response> proveriCsrf(const Request& req);
std::string kreirajSesiju(const std::string& korisnickoIme);
bool proveriSesiju(const std::string& sessionId, std::string& korisnickoIme);
void unistiSesiju(const std::string& sessionId);
std::string preuzmiCsrfToken(const std::string& sessionId);
bool proveriCsrfToken(const std::string& sessionId, const std::string& token);
void initSesijeTabela();
