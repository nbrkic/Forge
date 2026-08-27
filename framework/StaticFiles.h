#pragma once

#include <string>
#include "Response.h"

std::string mimeType(const std::string& putanja);
Response serveStaticFile(const std::string& fajlPutanja);
std::string proveriStvarniTipSlike(const std::string& sadrzaj);