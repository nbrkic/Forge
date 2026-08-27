#pragma once
#include <string>

struct ForgeConfig{
    int port = 8081;
    size_t maxZahtev = 10 * 1024 * 1024;
    int trajanjeSesijeSekundi = 3600;
    int timeoutSekundi = 60;
    bool corsOmogucen = false;
    std::string corsOrigin = "*";
    int rateLimitMaxZahteva = 10;
    int rateLimitProzorSekundi = 60;
    int kvalitetSlike = 80;
};

extern ForgeConfig config;

void ucitajConfig(const std::string& putanja = "config.json");