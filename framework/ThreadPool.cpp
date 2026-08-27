#include "ThreadPool.h"
#include <string>
#include "Response.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include "StaticFiles.h"
#include <sstream>
#include <vector>
#include "Logger.h"
#include <map>
#include <mutex>
#include <ctime>
#include <chrono>
#include "Config.h"

enum class OperacijaTip{ RECV, SEND };

struct IoOperacija{
    OVERLAPPED overlapped;
    char buffer[1024];
    WSABUF wsabuf;
    SOCKET clientSocket;
    OperacijaTip tip;
    std::string primljeno;
    std::string zaSlanje;
    std::ifstream* streamFajl = nullptr;
    int chunkBroj = -1;
    bool keepAlive = true;
    std::string ipAdresa;
};

HANDLE iocp;

std::map<SOCKET, time_t> aktivnost;
std::mutex aktivnostMutex;
const int TIMEOUT_SEKUNDI = config.timeoutSekundi;

void azurirajAktivnost(SOCKET s){
    std::lock_guard<std::mutex> lock(aktivnostMutex);
    aktivnost[s] = time(nullptr);
}

void ukloniAktivnost(SOCKET s){
    std::lock_guard<std::mutex> lock(aktivnostMutex);
    aktivnost.erase(s);
}

void watchdog(){
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(10));

        time_t sada = time(nullptr);
        std::lock_guard<std::mutex> lock(aktivnostMutex);
        for(auto it = aktivnost.begin(); it != aktivnost.end(); ){
            if(sada - it->second > TIMEOUT_SEKUNDI){
                log(NivoLoga::INFO, "Zatvaram neaktivnu konekciju (timeout).");
                closesocket(it->first);
                it = aktivnost.erase(it);
            }
            else{
                ++it;
            }
        }
    }
}

Response obradiZahtev(const std::string &request, const std::string &ipAdresa);

void iocpWorker()
{
    while (true)
    {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        OVERLAPPED *overlappedPtr;
        GetQueuedCompletionStatus(iocp, &bytesTransferred, &completionKey, &overlappedPtr, INFINITE);
        IoOperacija *op = reinterpret_cast<IoOperacija *>(overlappedPtr);

        if (op->tip == OperacijaTip::RECV)
        {
            if(bytesTransferred == 0){
                closesocket(op->clientSocket);
                ukloniAktivnost(op->clientSocket);
                delete op;
                continue;
            }

            op->primljeno.append(op->buffer, bytesTransferred);

            const size_t MAX_ZAHTEV = config.maxZahtev;
            if(op->primljeno.length() > MAX_ZAHTEV){
                log(NivoLoga::WARNING, "Zahtev prevelik, zatvaram konekciju.");
                closesocket(op->clientSocket);
                ukloniAktivnost(op->clientSocket);
                delete op;
                continue;
            }

            bool kompletno = false;
            size_t krajHeadera = op->primljeno.find("\r\n\r\n");

            if (krajHeadera != std::string::npos)
            {
                size_t clPos = op->primljeno.find("Content-Length:");
                size_t tePos = op->primljeno.find("Transfer-Encoding: chunked");

                if (clPos != std::string::npos && clPos < krajHeadera)
                {
                    size_t brojPocetak = op->primljeno.find(":", clPos) + 1;
                    int contentLength = std::stoi(op->primljeno.substr(brojPocetak));
                    size_t trenutnoTela = op->primljeno.length() - (krajHeadera + 4);
                    kompletno = (trenutnoTela >= (size_t)contentLength);
                }
                else if (tePos != std::string::npos && tePos < krajHeadera)
                {
                    kompletno = (op->primljeno.find("0\r\n\r\n", krajHeadera + 4) != std::string::npos);
                }
                else
                {
                    kompletno = true;
                }
            }

            if (!kompletno)
            {
                ZeroMemory(&(op->overlapped), sizeof(OVERLAPPED));
                op->wsabuf.buf = op->buffer;
                op->wsabuf.len = sizeof(op->buffer);
                DWORD flags = 0;
                WSARecv(op->clientSocket, &(op->wsabuf), 1, NULL, &flags, &(op->overlapped), NULL);
                azurirajAktivnost(op->clientSocket);
            }
            else
            {
                size_t prviRazmak = op->primljeno.find(" ");
                size_t drugiRazmak = op->primljeno.find(" ", prviRazmak + 1);
                std::string putanja = op->primljeno.substr(prviRazmak + 1, drugiRazmak - prviRazmak - 1);
                putanja = putanja.substr(0, putanja.find("?"));

                if(putanja.substr(0, 8) == "/static/"){
                    std::string fajlPutanja = "public/" + putanja.substr(8);

                    if(fajlPutanja.find("..") == std::string::npos){
                        std::ifstream* fajl = new std::ifstream(fajlPutanja, std::ios::binary | std::ios::ate);
                        if(fajl->is_open()){
                            std::streamsize velicina = fajl->tellg();
                            if(velicina > 65536){
                                fajl->seekg(0);

                                std::string mime = mimeType(fajlPutanja);
                                std::string headeri = "HTTP/1.1 200 OK\r\nContent-Type: " + mime + "\r\nContent-Length: " + std::to_string(velicina) + "\r\n\r\n";

                                IoOperacija* sendOp = new IoOperacija();
                                sendOp->tip = OperacijaTip::SEND;
                                sendOp->clientSocket = op->clientSocket;
                                sendOp->streamFajl = fajl;
                                sendOp->zaSlanje = headeri;
                                sendOp->wsabuf.buf = &sendOp->zaSlanje[0];
                                sendOp->wsabuf.len = (ULONG)sendOp->zaSlanje.length();

                                WSASend(op->clientSocket, &(sendOp->wsabuf), 1, NULL, 0, &(sendOp->overlapped), NULL);

                                delete op;
                                continue;
                            }
                            fajl->close();
                            delete fajl;
                        }
                        else{
                            delete fajl;
                        }
                    }
                }
                if(putanja == "/chunked-test"){
                    std::string headeri = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n";

                    IoOperacija* sendOp = new IoOperacija();
                    sendOp->tip = OperacijaTip::SEND;
                    sendOp->clientSocket = op->clientSocket;
                    sendOp->chunkBroj = 0;
                    sendOp->zaSlanje = headeri;
                    sendOp->wsabuf.buf = &sendOp->zaSlanje[0];
                    sendOp->wsabuf.len = (ULONG)sendOp->zaSlanje.length();

                    WSASend(op->clientSocket, &(sendOp->wsabuf), 1, NULL, 0, &(sendOp->overlapped), NULL);

                    delete op;
                    continue;
                }

                bool keepAlive = (op->primljeno.find("Connection: close") == std::string::npos);

                std::string data;
                try{
                    Response response = obradiZahtev(op->primljeno, op->ipAdresa);
                    data = response.toString();
                }
                catch(const std::exception& e){
                    log(NivoLoga::LOG_ERROR, std::string("Neuhvacen izuzetak u obradi zahteva: ") + e.what());
                    Response greska(500, "Internal Server Error", "Doslo je do greske na serveru.");
                    data = greska.toString();
                }
                catch(...){
                    log(NivoLoga::LOG_ERROR, "Nepoznat izuzetak u obradi zahteva.");
                    Response greska(500, "Internal Server Error", "Doslo je do nepoznate greske na serveru.");
                    data = greska.toString();
                }

                IoOperacija *sendOp = new IoOperacija();
                sendOp->tip = OperacijaTip::SEND;
                sendOp->clientSocket = op->clientSocket;
                sendOp->keepAlive = keepAlive;
                sendOp->zaSlanje = data;
                sendOp->wsabuf.buf = &sendOp->zaSlanje[0];
                sendOp->wsabuf.len = (ULONG)sendOp->zaSlanje.length();

                WSASend(op->clientSocket, &(sendOp->wsabuf), 1, NULL, 0, &(sendOp->overlapped), NULL);

                delete op;
            }
        }
        else{ 
            if(op->chunkBroj >= 0){
                static std::vector<std::string> chunkovi = {"Ovo je prvi deo. ", "Ovo je drugi deo. ", "Ovo je poslednji deo."};

                if(op->chunkBroj < (int)chunkovi.size()){
                    std::stringstream ss;
                    ss << std::hex << chunkovi[op->chunkBroj].length() << "\r\n" << chunkovi[op->chunkBroj] << "\r\n";
                    op->zaSlanje = ss.str();
                    op->chunkBroj++;
                }
                else if(op->chunkBroj == (int)chunkovi.size()){
                    op->zaSlanje = "0\r\n\r\n";
                    op->chunkBroj++;
                }
               else{
                    closesocket(op->clientSocket);
                    ukloniAktivnost(op->clientSocket);
                    delete op;
                    continue;
                }
                op->wsabuf.buf = &op->zaSlanje[0];
                op->wsabuf.len = (ULONG)op->zaSlanje.length();
                ZeroMemory(&(op->overlapped), sizeof(OVERLAPPED));
                WSASend(op->clientSocket, &(op->wsabuf), 1, NULL, 0, &(op->overlapped), NULL);
            }
            else if(op->streamFajl != nullptr){
                char chunkBuffer[65536];
                op->streamFajl->read(chunkBuffer, sizeof(chunkBuffer));
                std::streamsize procitano = op->streamFajl->gcount();
                if(procitano > 0){
                    op->zaSlanje.assign(chunkBuffer, (size_t)procitano);
                    op->wsabuf.buf = &op->zaSlanje[0];
                    op->wsabuf.len = (ULONG)procitano;
                    ZeroMemory(&(op->overlapped), sizeof(OVERLAPPED));
                    WSASend(op->clientSocket, &(op->wsabuf), 1, NULL, 0, &(op->overlapped), NULL);
                }
                else{
                    op->streamFajl->close();
                    delete op->streamFajl;
                    closesocket(op->clientSocket);
                    ukloniAktivnost(op->clientSocket);
                    delete op;
                }
            }
            else{
                if(op->keepAlive){
                    op->tip = OperacijaTip::RECV;
                    op->primljeno.clear();
                    op->wsabuf.buf = op->buffer;
                    op->wsabuf.len = sizeof(op->buffer);
                    ZeroMemory(&(op->overlapped), sizeof(OVERLAPPED));
                    DWORD flags = 0;
                    WSARecv(op->clientSocket, &(op->wsabuf), 1, NULL, &flags, &(op->overlapped), NULL);
                    azurirajAktivnost(op->clientSocket);
                }
                else{
                    closesocket(op->clientSocket);
                    ukloniAktivnost(op->clientSocket);
                    delete op;
                }
            }
        }
    }
}

void initIOCP(){
    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    for(int i = 0; i < 4; i++){
        std::thread(iocpWorker).detach();
    }
    std::thread(watchdog).detach();
}

void registerConnection(SOCKET clientSocket)
{
    CreateIoCompletionPort((HANDLE)clientSocket, iocp, 0, 0);

    sockaddr_in adresa;
    int adresaLen = sizeof(adresa);
    std::string ip = "unknown";
    if(getpeername(clientSocket, (sockaddr*)&adresa, &adresaLen) == 0){
        ip = inet_ntoa(adresa.sin_addr);
    }

    IoOperacija *op = new IoOperacija();
    op->tip = OperacijaTip::RECV;
    op->clientSocket = clientSocket;
    op->ipAdresa = ip;
    op->wsabuf.buf = op->buffer;
    op->wsabuf.len = sizeof(op->buffer);

    DWORD flags = 0;
    WSARecv(clientSocket, &(op->wsabuf), 1, NULL, &flags, &(op->overlapped), NULL);
    azurirajAktivnost(clientSocket);
}
