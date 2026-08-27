#include "StaticFiles.h"
#include <fstream>
#include <sstream>
#include "Request.h"

Response handleNotFound(const Request& req);

std::string mimeType(const std::string& putanja){
    size_t poslednjaTacka = putanja.rfind(".");
    std::string ekstenzija = (poslednjaTacka != std::string::npos) ? putanja.substr(poslednjaTacka + 1) : "";
    if(ekstenzija == "html" || ekstenzija == "htm"){
        return "text/html";
    }
    else if(ekstenzija == "css"){
        return "text/css";
    }
    else if(ekstenzija == "js"){
        return "application/javascript";
    }
    else if(ekstenzija == "json"){
        return "application/json";
    }
    else if(ekstenzija == "png"){
        return "image/png";
    }
    else if(ekstenzija == "jpg" || ekstenzija == "jpeg"){
        return "image/jpeg";
    }
    else if(ekstenzija == "gif"){
        return "image/gif";
    }
    else{
        return "application/octet-stream";
    }
}

Response serveStaticFile(const std::string& fajlPutanja){
    if(fajlPutanja.find("..") != std::string::npos){
        return Response(403, "Forbidden", "Access denied");
    }

    std::ifstream file(fajlPutanja, std::ios::binary);
    if(!file.is_open()){
        return handleNotFound(Request{});
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sadrzaj = buffer.str();
    std::string contentType = mimeType(fajlPutanja);
    return Response(200, "OK", sadrzaj, contentType);
}

std::string proveriStvarniTipSlike(const std::string& sadrzaj){
    if(sadrzaj.size() >= 3 &&
       (unsigned char)sadrzaj[0] == 0xFF && (unsigned char)sadrzaj[1] == 0xD8 && (unsigned char)sadrzaj[2] == 0xFF){
        return "image/jpeg";
    }
    if(sadrzaj.size() >= 8 &&
       (unsigned char)sadrzaj[0] == 0x89 && sadrzaj[1] == 'P' && sadrzaj[2] == 'N' && sadrzaj[3] == 'G' &&
       (unsigned char)sadrzaj[4] == 0x0D && (unsigned char)sadrzaj[5] == 0x0A && (unsigned char)sadrzaj[6] == 0x1A && (unsigned char)sadrzaj[7] == 0x0A){
        return "image/png";
    }
    if(sadrzaj.size() >= 6 && sadrzaj.substr(0, 3) == "GIF" &&
       (sadrzaj.substr(3, 3) == "87a" || sadrzaj.substr(3, 3) == "89a")){
        return "image/gif";
    }
    return "";
}