

#include <iostream>
#pragma comment (lib,"ws2_32.lib")
#include <winSock2.h>
#include <string>
#include <thread>
#include <ws2tcpip.h>
#include <fstream>
#include <windows.h>
#include <mutex>

SOCKET connection;
const int port = 1111;
const char* ipAddress = "127.0.0.1";
std::mutex logMutex;

void acceptMessage() {
    int messageSize;
    while (true) {
        recv(connection, (char*)&messageSize, sizeof(int), NULL);
        std::unique_ptr<char[]> message(new char[messageSize + 1]);
        message[messageSize] = '\0';
        recv(connection, message.get(), messageSize, NULL);
        if (std::string(message.get()) != "exit") {
            std::cout << message.get() << std::endl;
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
        logFile << error << " " << st.wDay << "." << st.wMonth << "." << st.wYear << " "
            << st.wHour << ":" << st.wMinute << ":" << st.wSecond << std::endl;
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

    connection = socket(AF_INET, SOCK_STREAM, NULL);

    std::cout << "[CLIENT] \n";

    if (connect(connection, (SOCKADDR*)&addr, sizeof(addr)) == INVALID_SOCKET) {
        errorThrowing("client socket error.");
        return 1;
    }
    std::cout << "Successfully connected. \n";
    std::cout << "Write \"exit\" if you want to exit\n";


    std::thread th(acceptMessage);
    th.detach();


    std::string message;
    while (true) {
        std::getline(std::cin, message);
        int messageSize = message.size();
        send(connection, (char*)&messageSize, sizeof(int), NULL);
        send(connection, message.c_str(), messageSize, NULL);
        if (message == "exit") {
            break;
        }
    }
    ::closesocket(connection);
    WSACleanup();
    return 0;
}

