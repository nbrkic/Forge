#include "Server.h"
#include <WinSock2.h>
#include <Windows.h>
#include <atomic>
#include "Logger.h"
#include "ThreadPool.h"
#include "Config.h"

std::atomic<bool> radi(true);
SOCKET globalniListenSocket;

BOOL WINAPI ConsoleHandler(DWORD signal){
    if(signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT){
        log(NivoLoga::INFO, "Gasim server...");
        radi = false;
        closesocket(globalniListenSocket);
        return TRUE;
    }
    return FALSE;
}

void pokreniServer(int port){
    if(port != 0){
        config.port = port;
    }
    ucitajConfig();
    WSADATA wsaData;
    int uspeh = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(uspeh != 0){
        log(NivoLoga::LOG_ERROR, "WSAStartup failed");
        return;
    }
    log(NivoLoga::INFO, "WSAStartup succeeded");

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    globalniListenSocket = s;
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    if(s == INVALID_SOCKET){
        log(NivoLoga::LOG_ERROR, "Socket creation failed");
        return;
    }
    log(NivoLoga::INFO, "Socket creation succeeded");

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(config.port);

    int bindResult = ::bind(s, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if(bindResult == SOCKET_ERROR){
        log(NivoLoga::LOG_ERROR, "Bind failed");
        return;
    }
    log(NivoLoga::INFO, "Bind succeeded");

    int listenResult = listen(s, SOMAXCONN);
    if(listenResult == SOCKET_ERROR){
        log(NivoLoga::LOG_ERROR, "Listen failed");
        return;
    }
    log(NivoLoga::INFO, "Listen succeeded");

    initIOCP();

    while(radi){
        SOCKET acceptResult = accept(s, NULL, NULL);
        if(acceptResult == INVALID_SOCKET){
            if(!radi){
                break;
            }
            log(NivoLoga::WARNING, "Accept failed");
            continue;
        }
        log(NivoLoga::INFO, "Klijent povezan");
        registerConnection(acceptResult);
    }

    log(NivoLoga::INFO, "Server ugasen.");
    WSACleanup();
}