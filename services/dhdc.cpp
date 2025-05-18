#include <chrono>
#include <GLFW/glfw3.h>
#include <iostream>
#include <windows.h>

#include "dhdc.h"

int __SDK dhdGetOrientationFrame (double matrix[3][3], char ID) {
   double local[3][3] = {
      {1.0, 0.0, 0.0},
      {0.0, 1.0, 0.0},
      {0.0, 0.0, 1.0}
   };

   // Copy local into matrix.
   for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
         matrix[i][j] = local[i][j];
      }
   }

   return DHD_NO_ERROR;
};

double __SDK dhdGetTime () {
   using namespace std::chrono;
   static auto startTime = steady_clock::now();
   auto now = steady_clock::now();
   duration<double> elapsed = now - startTime;
   return elapsed.count();
};

double __SDK dhdGetComFreq (char ID) {
   return 1000;
};

const char* __SDK dhdErrorGetLastStr () {
   return "No error";
};

int __SDK dhdEnableForce (uchar val, char ID) {
   return DHD_NO_ERROR;
};

int __SDK dhdGetGripperThumbPos (double *px, double *py, double *pz,  char ID) {
   return DHD_NO_ERROR;
};

int __SDK dhdGetGripperFingerPos (double *px, double *py, double *pz,  char ID) {
   return 0;
};

int __SDK dhdGetPosition(double *px, double *py, double *pz, char ID) {
    if (!px || !py || !pz) return DHD_ERROR_INVALID;

    static double keyboardZ = 0.0;

    // --- Atualiza posição Z com base nas teclas W/S ---
    SHORT wState = GetAsyncKeyState('W');
    SHORT sState = GetAsyncKeyState('S');
    double displacementIncrement = 0.000003;

    if (wState & 0x8000) keyboardZ += displacementIncrement;
    if (sState & 0x8000) keyboardZ -= displacementIncrement;

    // --- Obtém posição do mouse ---
    POINT cursorPos;
    if (!GetCursorPos(&cursorPos)) {
        std::cerr << "Cannot get cursor position.\n";
        *px = *py = *pz = 0.0;
        return DHD_ERROR;
    }

    double mouseX = -static_cast<double>(cursorPos.y);
    double mouseY = static_cast<double>(cursorPos.x);
    mouseX = 2.9 * (mouseX / 10000.0 + 0.053);
    mouseY = 2.9 * (mouseY / 10000.0 - 0.095);

    *px = keyboardZ;
    *py = mouseY;
    *pz = mouseX;

    return DHD_NO_ERROR;
}

bool __SDK dhdIsLeftHanded (char ID) {
   return true;
};

int __SDK dhdSetForceAndGripperForce (double fx, double fy, double fz, double fg, char ID) {
   return 0;
};

void __SDK dhdSleep (double sec) {
   return;
};

int __SDK dhdClose (char ID) {
   return 0;
};

int __SDK dhdOpen () {
   return 0;
};

const char* __SDK dhdGetSDKVersionStr() {
   return "3.17.6";
};

const char* __SDK dhdGetSystemName (char ID) {
   return "SystemName";
};

int __SDK dhdSetDeviceAngleRad (double angle, char ID) {
   return DHD_NO_ERROR;
};

int __SDK dhdGetButton (int index, char ID) {
   return DHD_NO_ERROR;
};

int __SDK dhdGetAvailableCount () {
   return 1;
};

int __SDK dhdSetDevice (char ID) {
   return DHD_NO_ERROR;
};

int __SDK dhdOpenID (char ID) {
   return DHD_NO_ERROR;
};

int __SDK dhdEmulateButton (uchar val, char ID) {
   return DHD_NO_ERROR;
};
