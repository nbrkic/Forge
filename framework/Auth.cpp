#include "Auth.h"
#include <windows.h>
#include <bcrypt.h>
#include <sstream>
#include <iomanip>
#include "Database.h"
#include "json.hpp"
#include "Session.h"

using json = nlohmann::json;

Response registracijaHandler(const Request& req){
    try{
        json parsed = json::parse(req.body);
        std::string korisnickoIme = parsed["korisnickoIme"];
        std::string lozinka = parsed["lozinka"];

        if(!registrujKorisnika(korisnickoIme, lozinka)){
            return Response(409, "Conflict", "Korisnicko ime vec postoji");
        }
        return Response(201, "Created", "Registracija uspesna");
    }
    catch(const std::exception&){
        return Response(400, "Bad Request", "Neispravni podaci");
    }
}

Response prijavaHandler(const Request& req){
    try{
        json parsed = json::parse(req.body);
        std::string korisnickoIme = parsed["korisnickoIme"];
        std::string lozinka = parsed["lozinka"];

        if(!prijaviKorisnika(korisnickoIme, lozinka)){
            return Response(401, "Unauthorized", "Pogresno korisnicko ime ili lozinka");
        }

        std::string sessionId = kreirajSesiju(korisnickoIme);
        std::string csrfToken = preuzmiCsrfToken(sessionId);

        Response response(200, "OK", "{\"csrfToken\":\"" + csrfToken + "\"}", "application/json");
        response.setCookie("session_id", sessionId);
        return response;
    }
    catch(const std::exception&){
        return Response(400, "Bad Request", "Neispravni podaci");
    }
}

Response odjavaHandler(const Request& req){
    auto it = req.cookies.find("session_id");
    if(it != req.cookies.end()){
        unistiSesiju(it->second);
    }
    return Response(200, "OK", "Odjavljen");
}

std::string generisiSalt(){
    unsigned char buffer[16];
    BCryptGenRandom(NULL, buffer, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    std::stringstream ss;
    for(int i = 0; i < 16; i++){
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i];
    }
    return ss.str();
}

std::string heshirajLozinku(const std::string& lozinka, const std::string& salt){
    BCRYPT_ALG_HANDLE algoritam;
    BCryptOpenAlgorithmProvider(&algoritam, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);

    unsigned char rezultat[32];
    BCryptDeriveKeyPBKDF2(
        algoritam,
        (PUCHAR)lozinka.c_str(), (ULONG)lozinka.length(),
        (PUCHAR)salt.c_str(), (ULONG)salt.length(),
        100000,
        rezultat, sizeof(rezultat),
        0
    );

    BCryptCloseAlgorithmProvider(algoritam, 0);

    std::stringstream ss;
    for(int i = 0; i < 32; i++){
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)rezultat[i];
    }
    return ss.str();
}

bool proveriLozinku(const std::string& lozinka, const std::string& salt, const std::string& hesiranaLozinka){
    return bezbednoUporedi(heshirajLozinku(lozinka, salt), hesiranaLozinka);
}

void initAuthTabela(){
    izvrsiUpit("CREATE TABLE IF NOT EXISTS korisnici (id INTEGER PRIMARY KEY AUTOINCREMENT, korisnickoIme TEXT UNIQUE NOT NULL, salt TEXT NOT NULL, hesiranaLozinka TEXT NOT NULL)");
}

bool registrujKorisnika(const std::string& korisnickoIme, const std::string& lozinka){
    auto postojeci = izvrsiSelect("SELECT id FROM korisnici WHERE korisnickoIme = ?", {korisnickoIme});
    if(!postojeci.empty()){
        return false;
    }

    std::string salt = generisiSalt();
    std::string hash = heshirajLozinku(lozinka, salt);

    return izvrsiUpit("INSERT INTO korisnici (korisnickoIme, salt, hesiranaLozinka) VALUES (?, ?, ?)", {korisnickoIme, salt, hash});
}

bool prijaviKorisnika(const std::string& korisnickoIme, const std::string& lozinka){
    auto rezultati = izvrsiSelect("SELECT salt, hesiranaLozinka FROM korisnici WHERE korisnickoIme = ?", {korisnickoIme});
    if(rezultati.empty()){
        return false;
    }

    std::string salt = rezultati[0]["salt"];
    std::string hesiranaLozinka = rezultati[0]["hesiranaLozinka"];

    return proveriLozinku(lozinka, salt, hesiranaLozinka);
}

bool bezbednoUporedi(const std::string& a, const std::string& b){
    if(a.length() != b.length()){
        return false;
    }

    unsigned char rezultat = 0;
    for(size_t i = 0; i < a.length(); i++){
        rezultat |= (unsigned char)a[i] ^ (unsigned char)b[i];
    }
    return rezultat == 0;
}
