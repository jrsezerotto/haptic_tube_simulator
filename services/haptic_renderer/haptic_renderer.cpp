#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <iostream>
#include <winsock2.h>  // Para comunicação UDP
#pragma comment(lib, "ws2_32.lib")

#define NOMINMAX
#include "windows.h"

#include "GL/glu.h"
#include <GLFW/glfw3.h>

#include "CMatrixGL.h"
#include "FontGL.h"

// Constants
const Eigen::Vector3d SpherePosition(0.0, 0.0, 0.0);
constexpr double SphereRadius = 0.03;
constexpr double ToolRadius = 0.005;
constexpr int SwapInterval = 1;

// Global variables
GLFWwindow* window = nullptr;
int windowWidth = 0;
int windowHeight = 0;
Eigen::Vector3d toolPosition;
Eigen::Vector3d forceTool;

void setupUDPListener(SOCKET& sock, sockaddr_in& serverAddr) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Erro ao criar socket UDP." << std::endl;
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(9999);

    if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Erro ao bindar porta 9999." << std::endl;
        exit(1);
    }

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
}

void checkForMessage(SOCKET sock) {
    char buffer[1024] = {};
    sockaddr_in from;
    int fromlen = sizeof(from);
    int bytesReceived = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&from, &fromlen);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';

        double x, y, z, fx, fy, fz;
        int parsed = sscanf(buffer, "%lf %lf %lf %lf %lf %lf", &x, &y, &z, &fx, &fy, &fz);
        if (parsed == 6) {
            toolPosition = Eigen::Vector3d(x, y, z);
            forceTool = Eigen::Vector3d(fx, fy, fz);
            std::cout << "Tool: [" << toolPosition.transpose()
                      << "]  Force: [" << forceTool.transpose() << "]" << std::endl;
        } else {
            std::cerr << "Mensagem mal formatada: " << buffer << std::endl;
        }
    }
}

void drawForceVector(const Eigen::Vector3d& force) {
    if (force.norm() < 1e-6) return;

    Eigen::Vector3d start = Eigen::Vector3d::Zero();
    Eigen::Vector3d end = force * 0.1;
    Eigen::Vector3d direction = (end - start).normalized();

    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3d(start.x(), start.y(), start.z());
    glVertex3d(end.x(), end.y(), end.z());
    glEnd();

    glPushMatrix();
    glTranslated(end.x(), end.y(), end.z());
    Eigen::Vector3d axis = direction.normalized();
    double angle = acos(axis.dot(Eigen::Vector3d(0, 0, 1))) * 180.0 / M_PI;
    Eigen::Vector3d rotAxis = Eigen::Vector3d(0, 0, 1).cross(axis);
    if (rotAxis.norm() > 1e-6) {
        rotAxis.normalize();
        glRotated(angle, rotAxis.x(), rotAxis.y(), rotAxis.z());
    }

    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, 0.001, 0.0, 0.003, 16, 1);
    gluDeleteQuadric(quad);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

int updateGraphics() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cMatrixGL matrix;
    matrix.set(SpherePosition);
    matrix.glMatrixPushMultiply();
    GLUquadricObj* sphere = gluNewQuadric();
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.1f, 0.3f, 0.5f);
    gluSphere(sphere, SphereRadius, 32, 32);
    matrix.glMatrixPop();

    matrix.set(toolPosition, Eigen::Matrix3d::Identity());
    matrix.glMatrixPushMultiply();
    sphere = gluNewQuadric();
    glColor3f(0.8f, 0.8f, 0.8f);
    gluSphere(sphere, ToolRadius, 32, 32);
    drawForceVector(forceTool);
    matrix.glMatrixPop();

    GLenum err = glGetError();
    return (err != GL_NO_ERROR) ? -1 : 0;
}

void onWindowResized(GLFWwindow* a_window, int a_width, int a_height) {
    windowWidth = a_width;
    windowHeight = a_height;
    double glAspect = static_cast<double>(a_width) / static_cast<double>(a_height);

    glViewport(0, 0, a_width, a_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, glAspect, 0.01, 10);
    gluLookAt(0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void onKeyPressed(GLFWwindow* a_window, int a_key, int, int a_action, int) {
    if (a_action != GLFW_PRESS) return;
    if (a_key == GLFW_KEY_ESCAPE || a_key == GLFW_KEY_Q) exit(0);
}

void onError(int, const char* a_description) {
    std::cout << "error: " << a_description << std::endl;
}

int initializeGLFW() {
    if (!glfwInit()) return -1;
    glfwSetErrorCallback(onError);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    windowWidth = static_cast<int>(0.8 * mode->height);
    windowHeight = static_cast<int>(0.5 * mode->height);
    int x = static_cast<int>(0.5 * (mode->width - windowWidth));
    int y = static_cast<int>(0.5 * mode->height);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_STEREO, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Sphere Render Only", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, onKeyPressed);
    glfwSetWindowSizeCallback(window, onWindowResized);
    glfwSetWindowPos(window, x, y);
    glfwSwapInterval(SwapInterval);
    glfwShowWindow(window);
    onWindowResized(window, windowWidth, windowHeight);

    GLfloat mat_ambient[] = { 0.5f, 0.5f, 0.5f };
    GLfloat mat_diffuse[] = { 0.5f, 0.5f, 0.5f };
    GLfloat mat_specular[] = { 0.5f, 0.5f, 0.5f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0);

    GLfloat ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0);
    GLfloat lightPos[] = { 2.0, 0.0, 0.0, 1.0f };
    GLfloat lightDir[] = { -1.0, 0.0, 0.0, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lightDir);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.0);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    return 0;
}

int main() {
    std::cout << "Visualização sem dispositivo háptico" << std::endl;
    if (initializeGLFW() < 0) return -1;

    SOCKET udpSocket;
    sockaddr_in serverAddr;
    setupUDPListener(udpSocket, serverAddr);

    while (!glfwWindowShouldClose(window)) {
        checkForMessage(udpSocket);

        if (updateGraphics() < 0) {
            std::cout << "error: failed to update graphics" << std::endl;
            break;
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    closesocket(udpSocket);
    WSACleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
