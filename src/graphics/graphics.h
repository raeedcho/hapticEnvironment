#pragma once 
#ifndef _GRAPHICS_H_INCLUDED_
#define _GRAPHICS_H_INCLUDED_ 

#include <time.h>
#include "chai3d.h"
#include <GLFW/glfw3.h>
#include "haptics/haptics.h"
#include <vector>

// ------------------------------------------------------
// -------------Custom Graphics Functionality------------
// ------------------------------------------------------
#include "cGenericVisualEffect.h"
#include "cMovingDots.h"
#include "cPipe.h"
#include "cArrow.h"

using namespace chai3d; 
using namespace std; 

struct GraphicsData {
  cStereoMode stereoMode;
  bool fullscreen;
  bool mirroredDisplay;
  cCamera* camera;
  cDirectionalLight* light;
  cDirectionalLight* light2; //A secondary light if needed
  GLFWwindow* window;
  int width;
  int height;
  int xPos;
  int yPos;
  int swapInterval;
  cShapeTorus* object;
  cFrequencyCounter freqCounterGraphics;
  clock_t graphicsClock;
  vector<cGenericVisualEffect*> visualEffectsList;
  cLabel* debuggerLable; //rsr. added for debugging
  unordered_map<string, double> debuggerContent;
  bool debuggerEnabled = true;
};

void initDisplay(void);
void initScene(void);
void errorCallback(int error, const char* errorDescription);
void resizeWindowCallback(GLFWwindow* window, int w, int h);
void keySelectCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void updateGraphics(void);
void startGraphicsLoop(void);

#endif
