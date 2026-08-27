#pragma once
#include <string>
#include "Request.h"
#include "Response.h"

Response registracijaHandler(const Request& req);
Response prijavaHandler(const Request& req);
Response odjavaHandler(const Request& req);
std::string generisiSalt();
std::string heshirajLozinku(const std::string& lozinka, const std::string& salt);
bool proveriLozinku(const std::string& lozinka, const std::string& salt, const std::string& hesiranaLozinka);
void initAuthTabela();
bool registrujKorisnika(const std::string& korisnickoIme, const std::string& lozinka);
bool prijaviKorisnika(const std::string& korisnickoIme, const std::string& lozinka);
bool bezbednoUporedi(const std::string& a, const std::string& b);
