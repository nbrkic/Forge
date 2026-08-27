#include "Response.h"

Response::Response(int statusCode, const std::string& statusText, const std::string& body, const std::string& contentType){
    this->statusCode = statusCode;
    this->statusText = statusText;
    this->body = body;
    this->contentType = contentType;
}

std::string Response::toString(){
    int duzina = body.length();
    std::string data = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText
                      + "\r\nContent-Type: " + contentType
                      + "\r\nContent-Length: " + std::to_string(duzina) + "\r\n";

    for(auto& cookie : cookies){
        data += "Set-Cookie: " + cookie + "\r\n";
    }

    for(auto& header : headeri){
        data += header.first + ": " + header.second + "\r\n";
    }

    data += "\r\n" + body;
    return data;
}

void Response::setCookie(const std::string& ime, const std::string& vrednost){
    cookies.push_back(ime + "=" + vrednost + "; HttpOnly; Path=/");
}

void Response::setHeader(const std::string& ime, const std::string& vrednost){
    headeri.push_back({ime, vrednost});
}

Response Response::json(int statusCode, const std::string& statusText, const nlohmann::json& podaci){
    return Response(statusCode, statusText, podaci.dump(), "application/json");
}
