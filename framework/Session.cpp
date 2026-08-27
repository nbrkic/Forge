#include "Session.h"
#include <random>
#include <ctime>
#include "Auth.h"
#include "Config.h"
#include "Database.h"

void initSesijeTabela(){
    izvrsiUpit("CREATE TABLE IF NOT EXISTS sesije (id TEXT PRIMARY KEY, korisnickoIme TEXT NOT NULL, istice INTEGER NOT NULL, csrfToken TEXT NOT NULL)");
}

std::string kreirajSesiju(const std::string& korisnickoIme){
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, 15);
    const char* hexChars = "0123456789abcdef";

    std::string id;
    for(int i = 0; i < 32; i++){
        id += hexChars[dist(gen)];
    }

    std::string csrfToken = generisiSalt();
    time_t istice = time(nullptr) + config.trajanjeSesijeSekundi;

    izvrsiUpit("INSERT INTO sesije (id, korisnickoIme, istice, csrfToken) VALUES (?, ?, ?, ?)",
        {id, korisnickoIme, std::to_string(istice), csrfToken});

    return id;
}

std::string preuzmiCsrfToken(const std::string& sessionId){
    auto rezultati = izvrsiSelect("SELECT csrfToken FROM sesije WHERE id = ?", {sessionId});
    return rezultati.empty() ? "" : rezultati[0]["csrfToken"];
}

bool proveriCsrfToken(const std::string& sessionId, const std::string& token){
    auto rezultati = izvrsiSelect("SELECT csrfToken FROM sesije WHERE id = ?", {sessionId});
    if(rezultati.empty()){
        return false;
    }
    return bezbednoUporedi(rezultati[0]["csrfToken"], token);
}

bool proveriSesiju(const std::string& sessionId, std::string& korisnickoIme){
    auto rezultati = izvrsiSelect("SELECT korisnickoIme, istice FROM sesije WHERE id = ?", {sessionId});
    if(rezultati.empty()){
        return false;
    }

    time_t istice = (time_t)std::stoll(rezultati[0]["istice"]);
    if(time(nullptr) > istice){
        izvrsiUpit("DELETE FROM sesije WHERE id = ?", {sessionId});
        return false;
    }

    korisnickoIme = rezultati[0]["korisnickoIme"];
    return true;
}

void unistiSesiju(const std::string& sessionId){
    izvrsiUpit("DELETE FROM sesije WHERE id = ?", {sessionId});
}

std::optional<Response> proveriAuth(const Request& req){
    auto it = req.cookies.find("session_id");
    if(it == req.cookies.end()){
        return Response(401, "Unauthorized", "Nisi ulogovan");
    }
    std::string korisnickoIme;
    if(!proveriSesiju(it->second, korisnickoIme)){
        return Response(401, "Unauthorized", "Sesija nije validna");
    }
    return std::nullopt;
}

std::optional<Response> proveriCsrf(const Request& req){
    if(req.metoda == "GET" || req.metoda == "HEAD" || req.metoda == "OPTIONS"){
        return std::nullopt;
    }

    auto it = req.cookies.find("session_id");
    if(it == req.cookies.end()){
        return std::nullopt;
    }

    auto headerIt = req.headers.find("X-CSRF-Token");
    if(headerIt == req.headers.end() || !proveriCsrfToken(it->second, headerIt->second)){
        return Response(403, "Forbidden", "Neispravan CSRF token");
    }

    return std::nullopt;
}