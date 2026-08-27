#pragma once
#include "sqlite3.h"
#include <string>
#include <vector>
#include <map>

extern sqlite3* db;

bool initDatabase();
bool izvrsiUpit(const std::string& sql, const std::vector<std::string>& parametri = {});
std::vector<std::map<std::string, std::string>> izvrsiSelect(const std::string& sql, const std::vector<std::string>& parametri = {});