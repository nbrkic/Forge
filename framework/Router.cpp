#include "Router.h"
#include <optional>
#include <sstream>

Response handleNotFound(const Request& req){
    return Response(404, "Not Found", "Error 404, not found");
}

std::map<std::pair<std::string, std::string>, RutaInfo> routes;

std::vector<std::string> podeliPutanju(const std::string& putanja){
    std::vector<std::string> segmenti;
    std::stringstream ss(putanja);
    std::string segment;
    while(std::getline(ss, segment, '/')){
        if(!segment.empty()){
            segmenti.push_back(segment);
        }
    }
    return segmenti;
}

bool poklapaSeRuta(const std::string& registrovanaPutanja, const std::string& stvarnaPutanja, std::map<std::string,std::string>& params){
    std::vector<std::string> registrovaniSegmenti = podeliPutanju(registrovanaPutanja);
    std::vector<std::string> stvarniSegmenti = podeliPutanju(stvarnaPutanja);

    if(registrovaniSegmenti.size() != stvarniSegmenti.size()){
        return false;
    }

    for(size_t i = 0; i < registrovaniSegmenti.size(); i++){
        if(registrovaniSegmenti[i][0] == ':'){
            params[registrovaniSegmenti[i].substr(1)] = stvarniSegmenti[i];
        }
        else if(registrovaniSegmenti[i] != stvarniSegmenti[i]){
            return false;
        }
    }

    return true;
}

Response route(Request& req, const std::string& metoda, const std::string& putanja){
    auto it = routes.find({metoda, putanja});

    if(it == routes.end()){
        for(auto& par : routes){
            if(par.first.first != metoda){
                continue;
            }
            std::map<std::string,std::string> params;
            if(poklapaSeRuta(par.first.second, putanja, params)){
                req.params = params;
                it = routes.find(par.first);
                break;
            }
        }
    }

    if(it == routes.end()){
        return handleNotFound(req);
    }

    for(auto& mw : it->second.middleware){
        std::optional<Response> rezultat = mw(req);
        if(rezultat.has_value()){
            return rezultat.value();
        }
    }

    return it->second.handler(req);
}
