#include "Logger.h"
#include <iostream>
#include <fstream>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>

std::mutex logMutex;
std::ofstream logFajl("server.log", std::ios::app);

std::string nivoUString(NivoLoga nivo){
    switch (nivo){
        case NivoLoga::DEBUG: return "DEBUG";
        case NivoLoga::LOG_ERROR: return "LOG_ERROR";
        case NivoLoga::INFO: return "INFO";
        case NivoLoga::WARNING: return "WARNING";
    }
    return "?";
}

std::string trenutnoVreme(){
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);
    std::stringstream ss;
    ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void log(NivoLoga nivo, const std::string& poruka){
    std::lock_guard<std::mutex> lock(logMutex);
    std::string linija = "[" + trenutnoVreme() + "] [" + nivoUString(nivo) + "] " + poruka;
    std::cout << linija << std::endl;
    logFajl << linija << std::endl;
    logFajl.flush();
}