#pragma once
#include <string>

enum class NivoLoga { DEBUG, INFO, WARNING, LOG_ERROR };

void log(NivoLoga nivo, const std::string& poruka);