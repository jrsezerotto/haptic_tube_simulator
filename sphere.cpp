// Adiciona vetores de força no renderizador da esfera

// C++ library headers
#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

// Platform specific headers
#define NOMINMAX
#include "windows.h"

// GLU library headers
#include "GL/glu.h"

// Force Dimension SDK library header
#include "dhdc.h"

// GLFW library header
#include <GLFW/glfw3.h>

// Project headers
#include "CMatrixGL.h"
#include "FontGL.h"

// Constants
const Eigen::Vector3d SpherePosition(0.0, 0.0, 0.0);
constexpr double SphereRadius = 0.03;
constexpr double ToolRadius = 0.005;
constexpr double LinearStiffness = 1000.0;
constexpr int SwapInterval = 1;

// Global variables
bool simulationRunning = true;
bool simulationFinished = false;
int toolCount = 0;
Eigen::Vector3d toolPosition;
Eigen::Vector3d forceTool;
GLFWwindow* window = nullptr;
int windowWidth = 0;
int windowHeight = 0;

void drawForceVector(const Eigen::Vector3d& force) {
    if (force.norm() < 1e-6) return;

    Eigen::Vector3d start = Eigen::Vector3d::Zero();
    Eigen::Vector3d end = force / 100.0;
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

    double deviceRotation[3][3] = {};
    if (dhdGetOrientationFrame(deviceRotation) < 0) return -1;

    cMatrixGL matrix;
    matrix.set(SpherePosition);
    matrix.glMatrixPushMultiply();
    GLUquadricObj* sphere = gluNewQuadric();
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.1f, 0.3f, 0.5f);
    gluSphere(sphere, SphereRadius, 32, 32);
    matrix.glMatrixPop();

    matrix.set(toolPosition, deviceRotation);
    matrix.glMatrixPushMultiply();
    sphere = gluNewQuadric();
    glColor3f(0.8f, 0.8f, 0.8f);
    gluSphere(sphere, ToolRadius, 32, 32);
    drawForceVector(forceTool);
    matrix.glMatrixPop();

    GLenum err = glGetError();
    return (err != GL_NO_ERROR) ? -1 : 0;
}

void* hapticsLoop(void*) {
    dhdEnableForce(DHD_ON);
    while (simulationRunning) {
        double px, py, pz;
        dhdGetPosition(&px, &py, &pz);
        toolPosition << px, py, pz;

        Eigen::Vector3d dir = (toolPosition - SpherePosition).normalized();
        double pen = (toolPosition - SpherePosition).norm() - SphereRadius - ToolRadius;
        if (pen < 0.0) forceTool = -pen * LinearStiffness * dir;
        else forceTool.setZero();

        Eigen::Vector3d f = forceTool;
        static bool safe = false;
        if (!safe) {
            if (f.norm() == 0.0) safe = true;
            else f.setZero();
        }
        dhdSetForce(f(0), f(1), f(2));
    }
    simulationFinished = true;
    return nullptr;
}

void onExit()
{
    // Wait for the haptic loop to finish.
    simulationRunning = false;
    while (!simulationFinished)
    {
        dhdSleep(0.1);
    }

    // Close the connection to the haptic device.
    if (dhdClose() < 0)
    {
        std::cout << "error: failed to close the connection (" << dhdErrorGetLastStr() << ")" << std::endl;
        return;
    }

    // Report success.
    std::cout << "connection closed" << std::endl;
    return;
}

void onWindowResized(GLFWwindow* a_window,
                     int a_width,
                     int a_height)
{
    windowWidth = a_width;
    windowHeight = a_height;
    double glAspect = (static_cast<double>(a_width) / static_cast<double>(a_height));

    glViewport(0, 0, a_width, a_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, glAspect, 0.01, 10);
    gluLookAt(0.2, 0.0, 0.0,
              0.0, 0.0, 0.0,
              0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void onKeyPressed(GLFWwindow* a_window,
                  int a_key,
                  int a_scanCode,
                  int a_action,
                  int a_modifiers)
{
    // Ignore all keyboard events that are not key presses.
    if (a_action != GLFW_PRESS)
    {
        return;
    }

    // Detect exit requests.
    if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        exit(0);
    }
}

void onError(int a_error,
             const char* a_description)
{
    std::cout << "error: " << a_description << std::endl;
}

int initializeGLFW()
{
    // Initialize the GLFW library.
    if (!glfwInit())
    {
        return -1;
    }

    // Set the error callback.
    glfwSetErrorCallback(onError);

    // Compute the desired window size.
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    windowWidth = static_cast<int>(0.8 *  mode->height);
    windowHeight = static_cast<int>(0.5 * mode->height);
    int x = static_cast<int>(0.5 * (mode->width - windowWidth));
    int y = static_cast<int>(0.5 * (mode->height - windowHeight));

    // Configure OpenGL rendering.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_STEREO, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    // Create the GLFW window and OpenGL display context.
    window = glfwCreateWindow(windowWidth, windowHeight, "Force Dimension - OpenGL Sphere Example", nullptr, nullptr);
    if (!window)
    {
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, onKeyPressed);
    glfwSetWindowSizeCallback(window, onWindowResized);
    glfwSetWindowPos(window, x, y);
    glfwSwapInterval(SwapInterval);
    glfwShowWindow(window);

    // Adjust initial window size
    onWindowResized(window, windowWidth, windowHeight);

    // Define material properties.
    GLfloat mat_ambient[] = { 0.5f, 0.5f, 0.5f };
    GLfloat mat_diffuse[] = { 0.5f, 0.5f, 0.5f };
    GLfloat mat_specular[] = { 0.5f, 0.5f, 0.5f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0);

    // Define light sources.
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

    // Use depth buffering for hidden surface elimination.
    glEnable(GL_DEPTH_TEST);

    // Clear the OpenGL color buffer.
    glClearColor(0.0, 0.0, 0.0, 1.0);

    // Report success.
    return 0;
}

int initializeHaptics()
{
    // Open the first available haptic device.
    if (dhdOpen() < 0)
    {
        return -1;
    }

    // Store the haptic device properties.
    toolCount = 1;

    return 0;
}

int initializeSimulation()
{
    // Initialize all tool positions.
    toolPosition.setZero();
    return 0;
}

int main(int argc,
         char* argv[])
{
    // Display version information.
    std::cout << "OpenGL Sphere Example " << dhdGetSDKVersionStr() << std::endl;
    std::cout << "Copyright (C) 2001-2023 Force Dimension" << std::endl;
    std::cout << "All Rights Reserved." << std::endl << std::endl;

    // Find and open a connection to a haptic device.
    if (initializeHaptics() < 0)
    {
        std::cout << "error: failed to initialize haptics" << std::endl;
        return -1;
    }

    // Open a GLFW window to render the simulation.
    if (initializeGLFW() < 0)
    {
        std::cout << "error: failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Initialize simulation objects.
    if (initializeSimulation() < 0)
    {
        std::cout << "error: failed to initialize the simulation" << std::endl;
        return -1;
    }

    // Create a high priority haptic thread.
    DWORD threadHandle;
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)(hapticsLoop), nullptr, 0x0000, &threadHandle);
    SetThreadPriority(&threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);

    // Register a callback that stops the haptic thread when the application exits.
    atexit(onExit);

    // Display the device type.
    std::cout << dhdGetSystemName() << " device detected" << std::endl << std::endl;

    // Display user instructions.
    std::cout << "press 'r' to toggle display of the haptic rate" << std::endl;
    std::cout << "      'q' to quit" << std::endl << std::endl;

    // Main graphic loop
    while (simulationRunning && !glfwWindowShouldClose(window))
    {
        // Render graphics.
        if (updateGraphics() < 0)
        {
            std::cout << "error: failed to update graphics" << std::endl;
            break;
        }

        // Swap buffers.
        glfwSwapBuffers(window);

        // Process GLFW events.
        glfwPollEvents();
    }

    // Close the GLFW window.
    glfwDestroyWindow(window);

    // Close the GLFW library.
    glfwTerminate();

    // Report success.
    return 0;
}
