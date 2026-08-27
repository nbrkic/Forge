#include "Database.h"
#include "Logger.h"
#include "json.hpp"
#include <mutex>

std::mutex dbMutex;

using json = nlohmann::json;

sqlite3* db = nullptr;

bool initDatabase(){
    int rc = sqlite3_open("forge.db", &db);
    if(rc != SQLITE_OK){
        log(NivoLoga::LOG_ERROR, "Ne mogu da otvorim bazu: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    log(NivoLoga::INFO, "Baza uspesno otvorena.");

    sqlite3_busy_timeout(db, 5000);

    char* walErr = nullptr;
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &walErr);
    if(walErr){
        log(NivoLoga::WARNING, "Ne mogu da ukljucim WAL mod: " + std::string(walErr));
        sqlite3_free(walErr);
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS oglasi ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "marka TEXT NOT NULL,"
                       "model TEXT NOT NULL,"
                       "godiste INTEGER,"
                       "cena REAL,"
                       "opis TEXT"
                       ");";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if(rc != SQLITE_OK){
        log(NivoLoga::LOG_ERROR, "Greska pri kreiranju tabele: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    log(NivoLoga::INFO, "Tabela 'oglasi' spremna.");

    return true;
}

bool izvrsiUpit(const std::string& sql, const std::vector<std::string>& parametri){
    std::lock_guard<std::mutex> lock(dbMutex);

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        log(NivoLoga::LOG_ERROR, "Greska pri pripremi upita: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    for(size_t i = 0; i < parametri.size(); i++){
        sqlite3_bind_text(stmt, (int)i + 1, parametri[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE){
        log(NivoLoga::LOG_ERROR, "Greska pri izvrsavanju upita: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    return true;
}

std::vector<std::map<std::string, std::string>> izvrsiSelect(const std::string& sql, const std::vector<std::string>& parametri){
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<std::map<std::string, std::string>> rezultati;

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        log(NivoLoga::LOG_ERROR, "Greska pri pripremi SELECT upita: " + std::string(sqlite3_errmsg(db)));
        return rezultati;
    }

    for(size_t i = 0; i < parametri.size(); i++){
        sqlite3_bind_text(stmt, (int)i + 1, parametri[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    int brojKolona = sqlite3_column_count(stmt);
    while(sqlite3_step(stmt) == SQLITE_ROW){
        std::map<std::string, std::string> red;
        for(int i = 0; i < brojKolona; i++){
            std::string imeKolone = sqlite3_column_name(stmt, i);
            const unsigned char* vrednost = sqlite3_column_text(stmt, i);
            red[imeKolone] = vrednost ? reinterpret_cast<const char*>(vrednost) : "";
        }
        rezultati.push_back(red);
    }
    sqlite3_finalize(stmt);

    return rezultati;
}
