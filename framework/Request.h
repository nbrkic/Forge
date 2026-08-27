#pragma once
#include <string>
#include <map>
#include <vector>
#include <optional>
#include "json.hpp"

struct UploadFajl{
    std::string imeFajla;
    std::string contentType;
    std::string sadrzaj;
};

struct Request{
    std::string metoda;
    std::string putanja;
    std::string verzija;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query;
    std::string body;
    std::map<std::string, std::string> cookies;
    std::map<std::string, std::string> multipartPolja;
    std::map<std::string, std::vector<UploadFajl>> multipartFajlovi;
    std::map<std::string, std::string> params;
    std::string ipAdresa;
};

std::optional<nlohmann::json> parsirajJson(const Request& req);
