#include <chrono>
#include <GLFW/glfw3.h>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/keysym.h>

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

   // Use a static variable to persist the x-position across calls.
   static double keyboardZ = 0.0;

   // Open the X display once for this call.
   Display* display = XOpenDisplay(nullptr);
   if (!display) {
       std::cerr << "Cannot open X display.\n";
       *px = *py = *pz = 0.0;
       return DHD_ERROR;
   }

   // --- Update currentPx using keyboard state ---
   auto getKeyboardPosition = [display]() -> double {
      double keyboardPosition = 0.0; // Initialize to zero

      // --- Check current key state using XQueryKeymap ---
      char keys[32];
      XQueryKeymap(display, keys);
      KeyCode wKey = XKeysymToKeycode(display, XK_w);
      KeyCode sKey = XKeysymToKeycode(display, XK_s);

      // Check if the bit corresponding to XK_w is set.
      double displacementIncrement = 0.00006;
      if (keys[wKey / 8] & (1 << (wKey % 8))) {
         keyboardPosition += displacementIncrement;  // Increase when W is pressed.
      }
      // Check if the bit corresponding to XK_s is set.
      if (keys[sKey / 8] & (1 << (sKey % 8))) {
         keyboardPosition -= displacementIncrement;  // Decrease when S is pressed.
      }

      return keyboardPosition;
   };

   // --- Retrieve mouse position using a lambda ---
   auto getMousePosition = [&]() -> std::pair<double, double> {
       Window root = DefaultRootWindow(display);
       Window returnedRoot, returnedChild;
       int rootX, rootY, winX, winY;
       unsigned int mask;
       XQueryPointer(display, root, &returnedRoot, &returnedChild,
                     &rootX, &rootY, &winX, &winY, &mask);
       // Transform coordinates as per your original calculations.
       double mouseX = -static_cast<double>(rootY);
       double mouseY = static_cast<double>(rootX);
       mouseX = 2.9 * (mouseX / 10000.0 + 0.0825);
       mouseY = 2.9 * (mouseY / 10000.0 - 0.1285);
       return {mouseX, mouseY};
   };

   auto [mouseX, mouseY] = getMousePosition();
   keyboardZ += getKeyboardPosition();

   *px = keyboardZ;
   *py = mouseY;
   *pz = mouseX;

   XCloseDisplay(display);
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

bool __SDK dhdHasWrist (char ID) {
   return false;
};

bool __SDK dhdHasGripper (char ID) {
   return false;
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

bool __SDK dhdHasActiveGripper (char ID) {
   return false;
};