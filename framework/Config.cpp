#include "Config.h"
#include "json.hpp"
#include "Logger.h"
#include <fstream>

using json = nlohmann::json;

ForgeConfig config;

void ucitajConfig(const std::string& putanja){
    std::ifstream file(putanja);
    if(!file.is_open()){
        log(NivoLoga::INFO, "config.json nije pronadjen, koristim podrazumevane vrednosti.");
        return;
    }
    try{
        json parsed;
        file >> parsed;

        config.port = parsed.value("port", config.port);
        config.maxZahtev = (size_t)parsed.value("maxZahtevMB", 10) * 1024 * 1024;
        config.trajanjeSesijeSekundi = parsed.value("trajanjeSesijeSekundi", config.trajanjeSesijeSekundi);
        config.timeoutSekundi = parsed.value("timeoutSekundi", config.timeoutSekundi);
        config.corsOmogucen = parsed.value("corsOmogucen", config.corsOmogucen);
        config.corsOrigin = parsed.value("corsOrigin", config.corsOrigin);
        config.rateLimitMaxZahteva = parsed.value("rateLimitMaxZahteva", config.rateLimitMaxZahteva);
        config.rateLimitProzorSekundi = parsed.value("rateLimitProzorSekundi", config.rateLimitProzorSekundi);
        config.kvalitetSlike = parsed.value("kvalitetSlike", config.kvalitetSlike);

        log(NivoLoga::INFO, "Konfiguracija ucitana iz " + putanja);
    }
    catch(const std::exception& e){
        log(NivoLoga::LOG_ERROR, std::string("Greska pri citanju config.json: ") + e.what());
    }
}
