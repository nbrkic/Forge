#include "Request.h"

std::optional<nlohmann::json> parsirajJson(const Request& req){
    try{
        return nlohmann::json::parse(req.body);
    }
    catch(...){
        return std::nullopt;
    }
}
