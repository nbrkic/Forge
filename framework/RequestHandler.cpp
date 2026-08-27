#include "RequestHandler.h"
#include "Request.h"
#include "Router.h"
#include "StaticFiles.h"
#include "Middleware.h"
#include "Multipart.h"
#include <map>
#include "Config.h"

using namespace std;

string urlDecode(const string& encoded){
    string decoded;
    for(size_t i = 0; i < encoded.length(); i++){
        if(encoded[i] == '%' && i + 2 < encoded.length()){
            string hex = encoded.substr(i + 1, 2);
            try{
                char decodedChar = (char)stoi(hex, nullptr, 16);
                decoded += decodedChar;
                i += 2;
            }
            catch(...){
                decoded += encoded[i];
            }
        }
        else if(encoded[i] == '+'){
            decoded += ' ';
        }
        else{
            decoded += encoded[i];
        }
    }
    return decoded;
}

string dekodirajChunked(const string& sirovoTelo){
    string rezultat;
    size_t pos = 0;
    while(true){
        size_t krajVelicine = sirovoTelo.find("\r\n", pos);
        if(krajVelicine == string::npos) break;

        string hexVelicina = sirovoTelo.substr(pos, krajVelicine - pos);
        size_t velicina = std::stoul(hexVelicina, nullptr, 16);

        if(velicina == 0){
            break;
        }

        size_t pocetakPodataka = krajVelicine + 2;
        rezultat += sirovoTelo.substr(pocetakPodataka, velicina);

        pos = pocetakPodataka + velicina + 2;
    }
    return rezultat;
}

Response obradiZahtev(const string &request, const string &ipAdresa){
    int prviRazmak = request.find(" ");
    int drugiRazmak = request.find(" ", prviRazmak+1);

    string metoda = request.substr(0, prviRazmak);
    string putanja = request.substr(prviRazmak + 1, drugiRazmak - prviRazmak - 1);

    if(config.corsOmogucen && metoda == "OPTIONS"){
        Response odgovor(200, "OK", "");
        odgovor.setHeader("Access-Control-Allow-Origin", config.corsOrigin);
        odgovor.setHeader("Access-Control-Allow-Credentials", "true");
        odgovor.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        odgovor.setHeader("Access-Control-Allow-Headers", "Content-Type, X-CSRF-Token");
        return odgovor;
    }

    map<string, string> query;
    if (putanja.find("?") != string::npos)
    {
        size_t upitPos = putanja.find("?") + 1;
        while (true)
        {
            size_t krajLinije = putanja.find("&", upitPos);
            string linija = putanja.substr(upitPos, krajLinije - upitPos);

            size_t jednakoPos = linija.find("=");
            string kljuc = linija.substr(0, jednakoPos);
            string vrednost = linija.substr(jednakoPos + 1);

            kljuc = urlDecode(kljuc);
            vrednost = urlDecode(vrednost);
            query[kljuc] = vrednost;

            if (krajLinije == string::npos)
            {
                break;
            }
            upitPos = krajLinije + 1;
        }
    }
    putanja = putanja.substr(0, putanja.find("?"));
    putanja = urlDecode(putanja);

    string verzija = request.substr(drugiRazmak + 1);
    map<string, string> headers;

    size_t pos = request.find("\r\n") + 2;
    while (true)
    {
        size_t krajLinije = request.find("\r\n", pos);
        string linija = request.substr(pos, krajLinije - pos);
        if (linija.empty())
        {
            break;
        }
        size_t dvotackaPos = linija.find(": ");
        string kljuc = linija.substr(0, dvotackaPos);
        string vrednost = linija.substr(dvotackaPos + 2);
        headers[kljuc] = vrednost;

        pos = krajLinije + 2;
    }
    map<string, string> cookies;
    if(headers.find("Cookie") != headers.end()){
        string cookieHeader = headers["Cookie"];
        size_t pos = 0;
        while(true){
            size_t kraj = cookieHeader.find("; ", pos);
            string par = cookieHeader.substr(pos, kraj - pos);

            size_t jednakoPos = par.find("=");
            string kljuc = par.substr(0, jednakoPos);
            string vrednost = par.substr(jednakoPos + 1);
            cookies[kljuc] = vrednost;

            if(kraj == string::npos){
                break;
            }
            pos = kraj + 2;
        }
    }

    string requestBody = request.substr(pos + 2);

    if(headers.count("Transfer-Encoding") && headers["Transfer-Encoding"].find("chunked") != string::npos){
        requestBody = dekodirajChunked(requestBody);
    }

    Request req{metoda, putanja, verzija, headers, query, requestBody, cookies};
    req.ipAdresa = ipAdresa;
    parsirajMultipart(req);

    optional<Response> kratkoSpajanje;
    for(auto& mw : middlewares){
        kratkoSpajanje = mw(req);
        if(kratkoSpajanje.has_value()){
            break;
        }
    }

    Response response = kratkoSpajanje.has_value()
        ? kratkoSpajanje.value()
        : ((putanja.substr(0, 8) == "/static/") ? serveStaticFile("public/" + putanja.substr(8)) : route(req, metoda, putanja));

    if(config.corsOmogucen){
        response.setHeader("Access-Control-Allow-Origin", config.corsOrigin);
        response.setHeader("Access-Control-Allow-Credentials", "true");
        response.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        response.setHeader("Access-Control-Allow-Headers", "Content-Type, X-CSRF-Token");
    }

    return response;
}