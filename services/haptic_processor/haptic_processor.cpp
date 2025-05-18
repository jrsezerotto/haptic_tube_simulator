#include <iostream>
#include <Eigen/Dense>
#include <cmath>
#include <thread>
#include <chrono>
#include <conio.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// Constantes da simulação
const Eigen::Vector3d SpherePosition(0.0, 0.0, 0.0);
constexpr double SphereRadius = 0.03;
constexpr double ToolRadius = 0.005;
constexpr double LinearStiffness = 1000.0;
constexpr double PixelToMeter = 0.0005;
constexpr double KeyStep = 0.001;

// Estado atual
Eigen::Vector3d toolPosition(0.05, 0.0, 0.0);
Eigen::Vector3d forceTool;

void updateToolPosition() {
    POINT cursor;
    GetCursorPos(&cursor);
    int screenCenterX = GetSystemMetrics(SM_CXSCREEN) / 2;
    int screenCenterY = GetSystemMetrics(SM_CYSCREEN) / 2;

    double x = (cursor.x - screenCenterX) * PixelToMeter;
    double y = -(cursor.y - screenCenterY) * PixelToMeter;
    toolPosition.x() = x;
    toolPosition.y() = y;

    if (_kbhit()) {
        int key = _getch();
        if (key == 'w') toolPosition.z() += KeyStep;
        if (key == 's') toolPosition.z() -= KeyStep;
    }
}

void updateForce() {
    Eigen::Vector3d delta = toolPosition - SpherePosition;
    double distance = delta.norm();
    double penetration = distance - SphereRadius - ToolRadius;

    if (penetration < 0.0 && distance > 1e-6)
        forceTool = -penetration * LinearStiffness * delta.normalized();
    else
        forceTool.setZero();
}

int main() {
    std::cout << "Use o mouse para mover em X/Y e teclas W/S para Z.\n";

    // Socket setup
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(12345);  // Porta do renderer
    destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Localhost

    SetCursorPos(GetSystemMetrics(SM_CXSCREEN)/2, GetSystemMetrics(SM_CYSCREEN)/2);

    while (true) {
        updateToolPosition();
        updateForce();

        std::cout << "\rPos: [" << toolPosition.transpose()
                  << "]  Força: [" << forceTool.transpose() << "]       " << std::flush;

        const char* message = "Hello there";
        sendto(sock, message, strlen(message), 0, (sockaddr*)&destAddr, sizeof(destAddr));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
