#include "RateLimiter.h"
#include <map>
#include <vector>
#include <mutex>
#include <ctime>
#include <algorithm>
#include "Config.h"

std::map<std::string, std::vector<time_t>> zahteviPoIp;
std::mutex rateLimitMutex;

std::optional<Response> ograniciBrzinu(const Request& req){
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    time_t sada = time(nullptr);

    auto& vremena = zahteviPoIp[req.ipAdresa];

    vremena.erase(std::remove_if(vremena.begin(), vremena.end(), [sada](time_t t){
        return sada - t > config.rateLimitProzorSekundi;
    }), vremena.end());

    if((int)vremena.size() >= config.rateLimitMaxZahteva){
        return Response(429, "Too Many Requests", "Previse zahteva, pokusaj ponovo kasnije");
    }

    vremena.push_back(sada);
    return std::nullopt;
}
