#pragma once
#include "Request.h"
#include <vector>

void parsirajMultipart(Request& req);
std::string sacuvajSliku(const UploadFajl& fajl, const std::string& direktorijum = "public/uploads/");
std::vector<std::string> sacuvajSlike(const std::vector<UploadFajl>& fajlovi, const std::string& direktorijum = "public/uploads/");
