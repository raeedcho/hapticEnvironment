#pragma once 
#include "chai3d.h"
#include "core/controller.h"
#include "network/network.h"
#include "haptics/haptics.h"
#include "graphics/graphics.h"
#include "graphics/cGenericVisualEffect.h"
#include "network/cGenericStreamerObject.h"
#include "math.h"

using namespace chai3d;
using namespace std;

/**
 * @file cCupAndBall.h
 * @class cCupAndBall
 * @brief Instance of cup task. See Hasson et al., 2012: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3544966/
 *
 * This class instantiates a ball-in-a-cup object. 
*/

class cCupAndBall: public cGenericVisualEffect, public cGenericEffect, public cGenericStreamerObject
{
  private:
    cWorld* world;
    cShapeSphere* ball;
    cMesh* cupMesh;
    double escapeTheta;
    double pendulumLength;
    double ballMass;
    double cupMass;
    double gravity;
    double ballDamping;
    double cupDamping;
    double ballAngle;
    double ballVelocity;
    double ballForce;
    double interactionForce;
    double cupPos;
    double cupVel;
    double cupAcc;
    double lastModelUpdateTime;
    bool running;
    bool ballInCup;
    bool enabledPathConstraint;
    bool enabledBallHaptics;
    bool enabledBallVisuals;
    bool enabledCupVisuals;

    cPrecisionClock* cupTaskClock;

  public:
    cCupAndBall(cWorld* worldPtr, double esc, double l, double bM, double cM,
                double bDamping, double cDamping);
    bool computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce);
    void startCupAndBall();
    void stopCupAndBall();
    void destructCupAndBall();
    void resetCupAndBall();
    void graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel);
    double computeBallAcceleration(double theta, double omega);
    void updateNextballAngle(double dt);
    
    bool setEnablePathConstraint(bool state);
    bool setEnableBallVisuals(bool state);
    bool setEnableBallHaptics(bool state);
    bool setEnableCupVisuals(bool state);

    bool getEnablePathConstraint();
    bool getEnableBallVisuals();
    bool getEnableBallHaptics();
    bool getEnableCupVisuals();

    void setColorBall(double r, double g, double b, double alpha);
    void setColorCup(double r, double g, double b, double alpha);

    void sendData();
};
