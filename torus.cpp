// C++ library headers
#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

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

class Utils {
    public:
    inline static Eigen::Vector3d forceOnTool = Eigen::Vector3d::Zero();

    static void drawForceOnTool() {
        Eigen::Vector3d fixedVector(0.03, 0.0, 0.0);
        Eigen::Vector3d start(0.0, 0.0, 0.0);
        Eigen::Vector3d end = Utils::forceOnTool/1000;
        glDisable(GL_LIGHTING);
        glBegin(GL_LINES);
        glColor3f(1.0f, 0.0f, 0.0f); // Set color to red
        glVertex3d(start.x(), start.y(), start.z());
        glVertex3d(end.x(), end.y(), end.z());
        glEnd();

        // Draw the arrow tip as a cone
        GLUquadric* quad = gluNewQuadric();
        glPushMatrix();

        // Move to the end of the vector
        glTranslated(end.x(), end.y(), end.z());

        // Compute the rotation to align the cone with fixedVector
        Eigen::Vector3d axis = fixedVector.normalized();
        double angle = acos(axis.dot(Eigen::Vector3d(0, 0, 1))) * 180.0 / M_PI;
        Eigen::Vector3d rotationAxis = Eigen::Vector3d(0, 0, 1).cross(axis).normalized();
        glRotated(angle, rotationAxis.x(), rotationAxis.y(), rotationAxis.z());

        // Draw the cone (radius = 0.003, height = 0.005)
        gluCylinder(quad, 0.001, 0.0, 0.003, 16, 1);

        // Do cleanup operations for OpenGL
        glPopMatrix();
        gluDeleteQuadric(quad);
        glEnable(GL_LIGHTING);
    };
};

struct HapticDevice
{
    int deviceId;
    Eigen::Vector3d toolPosition;
    Eigen::Matrix3d rotation;

    HapticDevice()
    : deviceId { -1 }
    {}
};

// Constants
constexpr double Stiffness = 1000.0;
constexpr double Mass = 1000.0;
constexpr double Kv = 1.0;
constexpr float TorusOuterRadius = 0.05f;
constexpr float TorusInnerRadius = 0.027f;
constexpr double ToolRadius = 0.005;
constexpr int GlfwSwapInterval = 1;

// Global variables
bool simulationRunning = true;
bool simulationFinished = false;
GLFWwindow* window = nullptr;
int windowWidth = 0;
int windowHeight = 0;
std::vector<HapticDevice> devicesList;
Eigen::Vector3d torusPosition;
Eigen::Matrix3d torusRotation;

int initializeHaptics()
{
    int deviceId = dhdOpenID(0);
    if (deviceId >= 0)
    {
        devicesList.push_back(HapticDevice {});
        HapticDevice& currentDevice = devicesList.back();
        currentDevice.deviceId = deviceId;
        std::cout << dhdGetSystemName() << " device detected" << std::endl;
    }
    return 0;
}

void* hapticsLoop(void* a_userData)
{
    // Allocate and initialize haptic loop variables.
    double timePrevious = dhdGetTime();
    double px = 0.0;
    double py = 0.0;
    double pz = 0.0;
    double rot[3][3] = {};
    Eigen::Vector3d toolPosition;
    Eigen::Vector3d toolLocalPosition;
    Eigen::Vector3d forceLocal;
    Eigen::Vector3d force;
    Eigen::Vector3d forceTool[2];
    forceTool[0].setZero();
    forceTool[1].setZero();
    Eigen::Vector3d torusAngularVelocity;
    torusAngularVelocity.setZero();

    // Enable force on all devices.
    size_t devicesCount = devicesList.size();
    dhdEnableForce(DHD_ON, devicesList[0].deviceId);

    // Run the haptic loop.
    while (simulationRunning)
    {
        // Compute time step.
        double time = dhdGetTime();
        double timeStep = time - timePrevious;
        timePrevious = time;

        // Process each device in turn.
        // Shortcut to the current device.
        HapticDevice& currentDevice = devicesList[0];

        // Select the active device to receive all subsequent DHD commands.
        dhdSetDevice(currentDevice.deviceId);

        // Retrieve the device orientation frame (identity for 3-dof devices).
        if (dhdGetOrientationFrame(rot) < 0)
        {
            std::cout << std::endl << "error: failed to read rotation (" << dhdErrorGetLastStr() << ")" << std::endl;
            break;
        }
        currentDevice.rotation << rot[0][0], rot[0][1], rot[0][2],
                                    rot[1][0], rot[1][1], rot[1][2],
                                    rot[2][0], rot[2][1], rot[2][2];

        // Retrieve the position of all tools attached to devices.
        // Devices equipped with grippers provide 2 tools, while others only provide 1.
        if (dhdGetPosition(&px, &py, &pz) < 0)
        {
            std::cout << std::endl << "error: failed to read rotation (" << dhdErrorGetLastStr() << ")" << std::endl;
            break;
        }
        toolPosition << px, py, pz;

        // Compute the position of the tool in the local coordinates of the torus.
        toolLocalPosition = torusRotation.transpose() * (toolPosition - torusPosition);

        // Project the tool position onto the torus plane (z = 0).
        Eigen::Vector3d toolProjection = toolLocalPosition;
        toolProjection(2) = 0.0;

        // Search for the nearest point on the torus medial axis.
        forceLocal.setZero();
        if (toolLocalPosition.squaredNorm() > 1e-10)
        {
            Eigen::Vector3d pointAxisTorus = TorusOuterRadius * toolProjection.normalized();

            // Compute eventual penetration of the tool inside the torus.
            Eigen::Vector3d torusToolDirection = toolLocalPosition - pointAxisTorus;

            // If the tool is inside the torus, compute the force which is proportional to the tool penetration.
            double distance = torusToolDirection.norm();
            if ((distance < (TorusInnerRadius + ToolRadius)) && (distance > 0.001))
            {
                forceLocal = ((TorusInnerRadius + ToolRadius) - distance) * Stiffness * torusToolDirection.normalized();
                toolLocalPosition = pointAxisTorus + (TorusInnerRadius + ToolRadius) * torusToolDirection.normalized();
            }

            // Otherwise, the tool is outside the torus and we have a null force.
            else
            {
                forceLocal.setZero();
            }
        }

        // TODO(now): retrieve reactionForce as the negative of forceTool
        // Convert the tool reaction force and position to world coordinates.
        forceTool[0] = torusRotation * forceLocal;
        currentDevice.toolPosition = torusRotation * toolLocalPosition;

        Utils::forceOnTool = forceTool[0];

        // Update the torus angular velocity.
        torusAngularVelocity += -1.0 / Mass * timeStep * (currentDevice.toolPosition - torusPosition).cross(forceTool[0]);

        // Compute the force to render on the haptic device.
        Eigen::Vector3d force;
        double gripperForceMagnitude = 0.0;
        force = forceTool[0];

        // Only enable haptic rendering once the device is in free space.
        static bool safeToRenderHaptics = false;
        if (!safeToRenderHaptics)
        {
            if (force.norm() == 0.0 && gripperForceMagnitude == 0.0)
            {
                safeToRenderHaptics = true;
            }
            else
            {
                force.setZero();
                gripperForceMagnitude = 0.0;
            }
        }

        // Apply all forces at once.
        dhdSetForceAndGripperForce(force(0), force(1), force(2), gripperForceMagnitude);

        // Stop the torus rotation if any of the devices button is pressed.
        for (size_t deviceIndex = 0; deviceIndex < devicesCount; deviceIndex++)
        {
            if (dhdGetButton(0, devicesList[deviceIndex].deviceId) != DHD_OFF)
            {
                torusAngularVelocity.setZero();
            }
        }

        // Add damping to slow down the torus rotation over time.
        torusAngularVelocity *= (1.0 - Kv * timeStep);

        // Compute the next pose of the torus.
        if (torusAngularVelocity.norm() > 1e-10)
        {
            Eigen::Matrix3d torusRotationIncrement;
            torusRotationIncrement = Eigen::AngleAxisd(torusAngularVelocity.norm(), torusAngularVelocity.normalized());
            torusRotation = torusRotationIncrement * torusRotation;
        }
    }

    // Flag the simulation as having finished.
    simulationRunning = false;
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

    // Close the connection to all the haptic devices.
    size_t devicesCount = devicesList.size();
    if (dhdClose() < 0)
    {
        std::cout << "error: failed to close the connection to device ID " << devicesList[0].deviceId << " (" << dhdErrorGetLastStr() << ")" << std::endl;
        return;
    }

    // Report success.
    std::cout << "connection closed" << std::endl;
    return;
}

void DrawTorus(float a_outerRadius,
               float a_innerRadius,
               int a_majorNumSegments,
               int a_minorNumSegments)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    double majorStep = 2.0 * M_PI / a_majorNumSegments;
    for (int majorIndex = 0; majorIndex < a_majorNumSegments; ++majorIndex)
    {
        double a0 = majorIndex * majorStep;
        double a1 = a0 + majorStep;
        GLdouble x0 = cos(a0);
        GLdouble y0 = sin(a0);
        GLdouble x1 = cos(a1);
        GLdouble y1 = sin(a1);

        glBegin(GL_TRIANGLE_STRIP);

        double minorStep = 2.0 * M_PI / a_minorNumSegments;
        for (int minorIndex = 0; minorIndex <= a_minorNumSegments; ++minorIndex)
        {
            double b = minorIndex * minorStep;
            GLdouble c = cos(b);
            GLdouble r = a_innerRadius * c + a_outerRadius;
            GLdouble z = a_innerRadius * sin(b);

            glNormal3d(x0 * c, y0 * c, z / a_innerRadius);
            glTexCoord2d(majorIndex / static_cast<GLdouble>(a_majorNumSegments), minorIndex / static_cast<GLdouble>(a_minorNumSegments));
            glVertex3d(x0 * r, y0 * r, z);

            glNormal3d(x1 * c, y1 * c, z / a_innerRadius);
            glTexCoord2d((majorIndex + 1) / static_cast<GLdouble>(a_majorNumSegments), minorIndex / static_cast<GLdouble>(a_minorNumSegments));
            glVertex3d(x1 * r, y1 * r, z);
        }

        glEnd();
    }
}

int updateGraphics()
{
    // Clean up.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the torus.
    static const GLfloat mat_ambient0[] = { 0.1f, 0.1f, 0.3f };
    static const GLfloat mat_diffuse0[] = { 0.1f, 0.3f, 0.5f };
    static const GLfloat mat_specular0[] = { 0.5f, 0.5f, 0.5f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular0);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0);

    // Render the torus at its current position and rotation.
    cMatrixGL matrix;
    matrix.set(torusPosition, torusRotation);
    matrix.glMatrixPushMultiply();
    DrawTorus(TorusOuterRadius, TorusInnerRadius, 64, 64);
    matrix.glMatrixPop();

    // Configure OpenGL for tool rendering.
    static const GLfloat mat_ambient1[] = { 0.4f, 0.4f, 0.4f };
    static const GLfloat mat_diffuse1[] = { 0.8f, 0.8f, 0.8f };
    static const GLfloat mat_specular1[] = { 0.5f, 0.5f, 0.5f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular1);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0);

    // Render all tools for all devices.
    HapticDevice& currentDevice = devicesList[0];
    matrix.set(currentDevice.toolPosition, currentDevice.rotation);
    matrix.glMatrixPushMultiply();
    GLUquadricObj* sphere = gluNewQuadric();
    gluSphere(sphere, ToolRadius, 32, 32);

    // Drawing force acting over the tool
    Utils::drawForceOnTool();

    matrix.glMatrixPop();

    // Report any issue.
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        return -1;
    }

    // Report success.
    return 0;
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
    window = glfwCreateWindow(windowWidth, windowHeight, "Force Dimension - OpenGL Torus Example", nullptr, nullptr);
    if (!window)
    {
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, onKeyPressed);
    glfwSetWindowSizeCallback(window, onWindowResized);
    glfwSetWindowPos(window, x, y);
    glfwSwapInterval(GlfwSwapInterval);
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

int initializeSimulation()
{
    size_t devicesCount = devicesList.size();
    devicesList[0].toolPosition.setZero();
    torusPosition.setZero();
    torusRotation.Identity();
    torusRotation = Eigen::AngleAxisd(M_PI * 45.0 / 180.0, Eigen::Vector3d(0.0, 1.0, -1.0));
    return 0;
}

int main(int argc,
         char* argv[])
{
    // Display version information.
    std::cout << "OpenGL Torus Example " << dhdGetSDKVersionStr() << std::endl;
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

    // Display user instructions.
    std::cout << std::endl;
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
