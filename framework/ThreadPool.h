#pragma once
#include <WinSock2.h>

void initIOCP();
void registerConnection(SOCKET clientSocket);