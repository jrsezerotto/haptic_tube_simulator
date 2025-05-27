#include <iostream>
#include <Eigen/Dense>
#include <cmath>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include <windows.h>
#include "dhdc.h"

#pragma comment(lib, "ws2_32.lib")

constexpr double SphereRadius = 0.04;
constexpr double ToolRadius = 0.005;
constexpr double LinearStiffness = 3000.0;

Eigen::Vector3d toolPosition(0.05, 0.0, 0.0);
Eigen::Vector3d forceTool;
const Eigen::Vector3d SpherePosition(0.0, 0.0, 0.0);

int main() {
    std::cout << "Sending haptic tool data over UDP...\n";

    // Inicia dispositivo háptico
    if (dhdOpen() < 0) {
        std::cerr << "Erro ao conectar com o dispositivo háptico: " << dhdErrorGetLastStr() << "\n";
        return -1;
    }
    dhdEnableForce(DHD_ON);

    // Setup de socket
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(9999);
    destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (true) {
        // Atualiza posição da ferramenta
        double x, y, z;
        dhdGetPosition(&x, &y, &z);
        toolPosition = Eigen::Vector3d(x, y, z);

        // Calcula força
        Eigen::Vector3d delta = toolPosition - SpherePosition;
        double distance = delta.norm();
        double penetration = distance - SphereRadius - ToolRadius;

        if (penetration < 0.0 && distance > 1e-6) {
            forceTool = -penetration * LinearStiffness * delta.normalized();
            forceTool /= 10.0;
        } else {
            forceTool.setZero();
        }

        // Aplica força no dispositivo
        dhdSetForce(forceTool.x(), forceTool.y(), forceTool.z());

        // Envia via UDP
        char message[128];
        snprintf(message, sizeof(message), "%.10f %.10f %.10f %.10f %.10f %.10f",
                 toolPosition.x(), toolPosition.y(), toolPosition.z(),
                 forceTool.x(), forceTool.y(), forceTool.z());

        sendto(sock, message, strlen(message), 0, (sockaddr*)&destAddr, sizeof(destAddr));

#ifdef PROD_BUILD
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif
    }

    closesocket(sock);
    WSACleanup();
    dhdClose();
    return 0;
}
