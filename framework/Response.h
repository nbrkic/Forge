#pragma once
#include <string>
#include <vector>
#include <utility>
#include "json.hpp"

class Response{
    private:
        int statusCode;
        std::string statusText;
        std::string body;
        std::string contentType;
        std::vector<std::string> cookies;
        std::vector<std::pair<std::string,std::string>> headeri;
    public:
        Response(int statusCode, const std::string& statusText, const std::string& body, const std::string& contentType = "text/plain");
        std::string toString();
        void setCookie(const std::string& ime, const std::string& vrednost);
        void setHeader(const std::string& ime, const std::string& vrednost);
        static Response json(int statusCode, const std::string& statusText, const nlohmann::json& podaci);
};
