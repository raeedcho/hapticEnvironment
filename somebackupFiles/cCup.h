#pragma once 
#include "chai3d.h"
#include "core/controller.h"
#include "network/network.h"
#include "haptics/haptics.h"
#include "graphics/graphics.h"
#include "graphics/cGenericVisualEffect.h"
#include "math.h"

using namespace chai3d;
using namespace std;

/**
 * @file cCup.h
 * @class cCup
 * @brief An instance of an arc represenging a cup
 *
 * This class instantiates a Cup object. 
*/

class cCup: public cGenericVisualEffect, public cGenericEffect
{
  private:
    double innerRadius;
    double cupMass;
    cWorld* world;
    cMesh* cupMesh;
    double cupPos;
    double cupVel;
    double cupAcc;
    bool enable;

  public:
    cCup(cWorld* worldPtr, double rimAngle, double cupRadius, double cM);
    bool computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce);
    void graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel);
    double computeBallAcceleration(double theta, double omega);
    bool setEnable(bool state);
    void destructCup();
};
