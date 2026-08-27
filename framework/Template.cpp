#include "Template.h"
#include <fstream>
#include <sstream>

std::string renderTemplate(const std::string& putanjaFajla, const std::map<std::string, std::string>& podaci){
    std::ifstream file(putanjaFajla, std::ios::binary);
    if(!file.is_open()){
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sadrzaj = buffer.str();

    for(auto& par : podaci){
        std::string placeholder = "{{" + par.first + "}}";
        size_t pos = 0;
        while((pos = sadrzaj.find(placeholder, pos)) != std::string::npos){
            sadrzaj.replace(pos, placeholder.length(), par.second);
            pos += par.second.length();
        }
    }
    return sadrzaj;
}

std::string escapeHtml(const std::string& sirov){
    std::string rezultat;
    for(char c : sirov){
        switch(c){
            case '&': rezultat += "&amp;"; break;
            case '<': rezultat += "&lt;"; break;
            case '>': rezultat += "&gt;"; break;
            case '"': rezultat += "&quot;"; break;
            case '\'': rezultat += "&#39;"; break;
            default: rezultat += c;
        }
    }
    return rezultat;
}
