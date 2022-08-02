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
 * @file cPendulum.h
 * @class cPendulum
 * @brief Instance of a pendulum that hands from the tool
 *
 * This class instantiates a pendulum that hangs from the tool
*/

class cPendulum: public cGenericVisualEffect, public cGenericEffect
{
  private:
    double escapeTheta;
    double pendulumLength;
    double ballMass;
    double gravity;
    double pendulumDamping;
    cWorld* world;
    cShapeSphere* ball;
    // todo add a rod
    double pendulumAngle;
    double pendulumVelocity;
    double hingeVel;
    double hingeAcc;
    double lastUpdateTime;
    bool hapticEnable;
    bool visualEnable;
    bool ballInCup;
    cPrecisionClock* pendulumClock;

  public:
    cPendulum(cWorld* worldPtr, double esc, double l, double bM);
    bool computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce);
    void graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel);
    double computePendulumAcceleration(double theta, double omega);
    void updateNextPendulumAngle(double dt);
    void resetPendulum();
    bool setEnableHaptic(bool state);
    bool setEnableVisual(bool state);
    void destructPendulum();
};
