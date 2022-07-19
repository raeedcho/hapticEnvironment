#pragma once 
#include "chai3d.h"
#include "core/controller.h"
#include "network/network.h"
#include "haptics/haptics.h"
#include "graphics/graphics.h"
#include "graphics/cGenericMovingObject.h"
#include "haptics/cConstrainToLine.h"
#include "math.h"

using namespace chai3d;
using namespace std;

/**
 * @file cCups.h
 * @class cCups
 * @brief Instance of cups task. See Hasson et al., 2012: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3544966/
 *
 * This class instantiates a Cups task tobject. 
*/

class cCups: public cGenericMovingObject, public cGenericEffect
{
  private:
    double escapeTheta;
    double pendulumLength;
    double ballMass;
    double cupMass;
    double gravity;
    double ballDamping;
    double targetDistance;
    cWorld* world;
    cShapeSphere* ball;
    cShapeBox* start;
    cShapeBox* stop;
    cMesh* cupMesh;
    cVector3d* startTarget;
    cVector3d* stopTarget;
    cConstrainToLine* pathConstraint;
    double ballAngle;
    double ballVelocity;
    double ballForce;
    double interactionForce;
    double cupPos;
    double cupVel;
    double cupAcc;
    double lastUpdateTime;
    bool running;
    bool ballInCup;
    cPrecisionClock* cupsClock;

  public:
    cCups(cWorld* worldPtr, double esc, double l, double bM, double cM);
    bool computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce);
    void startCups();
    void stopCups();
    void destructCups();
    void resetCups();
    void graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel);
    double computeBallAcceleration(double theta, double omega);
    void updateNextballAngle(double dt);
};
