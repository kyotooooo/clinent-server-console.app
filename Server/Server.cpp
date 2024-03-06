#include <iostream>
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <thread>
#include <ws2tcpip.h>
#include <mutex>
#include <fstream>
#include <Windows.h>
#include <vector>

std::mutex logMutex;
std::vector<SOCKET> connections;
int connectionsCounter = 0; 
const int port = 1111;
const char* ipAddress = "127.0.0.1";



void sendMessage(int clientNumber) {
    int messageSize;
    while(true) {
        if (recv(connections[clientNumber], (char*)&messageSize, sizeof(int), NULL) != INVALID_SOCKET) {
            std::unique_ptr<char[]> message(new char[messageSize + 1]);
            message[messageSize] = '\0';
            recv(connections[clientNumber], message.get(), messageSize, NULL);          

            std::lock_guard<std::mutex> lock(logMutex);

            for (int i = 0; i < connectionsCounter; ++i) {
                if (i == clientNumber) { 
                    continue;
                }
                send(connections[i], (char*)&messageSize, sizeof(int), NULL);           
                send(connections[i], message.get(), messageSize, NULL);                 
            }
        }
        else {
            ::closesocket(connections[clientNumber]);
            std::cout << "Client " << clientNumber + 1 << " disconnected \n";
            return;
        }
       
    }
}

void errorThrowing(std::string error) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::ofstream logFile("logErrors.txt", std::ios::app);
    if (logFile.is_open()) {
        std::cerr << error << std::endl;
        std::lock_guard<std::mutex> lock(logMutex);
        logFile << error << " " << st.wDay <<  "." << st.wMonth << "." << st.wYear << " "
            << st.wHour << ":" << st.wMinute << ":" << st.wSecond <<  std::endl;
        logFile.close();
    }
    if (!logFile.is_open()) {
        std::cerr << "Error with opening log file\n";
    }
    std::cerr << error;
}

int main() {
    WSAData wsaData;  
    WORD DLLVersion = MAKEWORD(2, 2);                           
    if (WSAStartup(DLLVersion, &wsaData) != 0) {                
        errorThrowing("connecting library error.");
        return 1;
    }
    SOCKADDR_IN addr;                                           
    int sizeofaddr = sizeof(addr);
    if (inet_pton(AF_INET, ipAddress, &addr.sin_addr) != 1) {
        errorThrowing("converting IP address error.");    
        return 1;
    }
    addr.sin_port = htons(port);                                 
    if (addr.sin_port == 0) {
        errorThrowing("setting port error.");
        return 1;
    }
    addr.sin_family = AF_INET;                                  
    if (addr.sin_family != AF_INET) {
        errorThrowing("setting address family error.");
        return 1;
    }

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);       
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));              
    listen(sListen, SOMAXCONN);                                 
                                                                

    SOCKET newConnection;                                      
  
    std::cout << "[SERVER] \n";
    for (int i = 0; ; ++i) {                         
        newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr); 

        if (newConnection == INVALID_SOCKET) {
            ::closesocket(newConnection);
            errorThrowing("server socket error.");
            return 1;
        }
        else {
            std::cout << "Client " << i + 1 << " successfully connected. \n";
            connections.push_back(newConnection);                  
            ++connectionsCounter;                              
            std::thread th(sendMessage, i);                    
            th.detach();
        }
    }
    
    WSACleanup();
    

    return 0;
}