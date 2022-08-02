#pragma once
#include "chai3d.h"

using namespace chai3d;
using namespace std;


/**
 * @file cGenericVisualEffect.h
 *
 * @class cGenericVisualEffect
 *
 * @brief A generic abstract class for any object that is visible on the screen and shown at regular time intervals.
 * 
 * All objects that are visible must inherit this class and override the graphicsLoopFunction. This
 * function is called in the graphics update loop for each moving object in the environment. 
 */
class cGenericVisualEffect : public cGenericObject 
{
  public:
    cGenericVisualEffect();
    ~cGenericVisualEffect();
    virtual void graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel) {};
};

