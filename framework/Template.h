#pragma once
#include <string>
#include <map>

std::string renderTemplate(const std::string& putanjaFajla, const std::map<std::string, std::string> &podaci);
std::string escapeHtml(const std::string& sirov);
