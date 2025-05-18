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

// GLFW library header
#include <GLFW/glfw3.h>

// Project headers
#include "CMatrixGL.h"
#include "FontGL.h"

// Constants
constexpr float TorusOuterRadius = 0.05f;
constexpr float TorusInnerRadius = 0.027f;
constexpr double ToolRadius = 0.005;
constexpr int GlfwSwapInterval = 1;

GLFWwindow* window = nullptr;
int windowWidth = 0;
int windowHeight = 0;
Eigen::Vector3d torusPosition;
Eigen::Matrix3d torusRotation;
Eigen::Vector3d toolPosition = Eigen::Vector3d::Zero();

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
            glVertex3d(x0 * r, y0 * r, z);

            glNormal3d(x1 * c, y1 * c, z / a_innerRadius);
            glVertex3d(x1 * r, y1 * r, z);
        }

        glEnd();
    }
}

int updateGraphics()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static const GLfloat mat_ambient0[] = { 0.1f, 0.1f, 0.3f };
    static const GLfloat mat_diffuse0[] = { 0.1f, 0.3f, 0.5f };
    static const GLfloat mat_specular0[] = { 0.5f, 0.5f, 0.5f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular0);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0);

    cMatrixGL matrix;
    matrix.set(torusPosition, torusRotation);
    matrix.glMatrixPushMultiply();
    DrawTorus(TorusOuterRadius, TorusInnerRadius, 64, 64);
    matrix.glMatrixPop();

    static const GLfloat mat_ambient1[] = { 0.4f, 0.4f, 0.4f };
    static const GLfloat mat_diffuse1[] = { 0.8f, 0.8f, 0.8f };
    static const GLfloat mat_specular1[] = { 0.5f, 0.5f, 0.5f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular1);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0);

    matrix.set(toolPosition, Eigen::Matrix3d::Identity());
    matrix.glMatrixPushMultiply();
    GLUquadricObj* sphere = gluNewQuadric();
    gluSphere(sphere, ToolRadius, 32, 32);
    gluDeleteQuadric(sphere);
    matrix.glMatrixPop();

    GLenum err = glGetError();
    return (err != GL_NO_ERROR) ? -1 : 0;
}

void onWindowResized(GLFWwindow* a_window, int a_width, int a_height)
{
    windowWidth = a_width;
    windowHeight = a_height;
    double glAspect = static_cast<double>(a_width) / a_height;

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

void onKeyPressed(GLFWwindow* a_window, int a_key, int, int a_action, int)
{
    if (a_action == GLFW_PRESS && (a_key == GLFW_KEY_ESCAPE || a_key == GLFW_KEY_Q)) exit(0);
}

void onError(int, const char* a_description)
{
    std::cout << "error: " << a_description << std::endl;
}

int initializeGLFW()
{
    if (!glfwInit()) return -1;
    glfwSetErrorCallback(onError);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    windowWidth = static_cast<int>(0.8 *  mode->height);
    windowHeight = static_cast<int>(0.5 * mode->height);
    int x = static_cast<int>(0.5 * (mode->width - windowWidth));
    int y = static_cast<int>(0.5 * (mode->height - windowHeight));

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_STEREO, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Force Dimension - OpenGL Torus Example", nullptr, nullptr);
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, onKeyPressed);
    glfwSetWindowSizeCallback(window, onWindowResized);
    glfwSetWindowPos(window, x, y);
    glfwSwapInterval(GlfwSwapInterval);
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

int initializeSimulation()
{
    torusPosition.setZero();
    torusRotation = Eigen::AngleAxisd(M_PI * 45.0 / 180.0, Eigen::Vector3d(0.0, 1.0, -1.0));
    return 0;
}

int main()
{
    if (initializeGLFW() < 0) return -1;
    if (initializeSimulation() < 0) return -1;

    while (!glfwWindowShouldClose(window))
    {
        if (updateGraphics() < 0)
        {
            std::cout << "error: failed to update graphics" << std::endl;
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
