#pragma once 
#include "chai3d.h"
#include "core/controller.h"
#include "network/network.h"
#include "haptics/haptics.h"
#include "graphics/graphics.h"
#include "graphics/cGenericVisualEffect.h"
#include "haptics/cConstrainToLine.h"
#include "math.h"
#include "cCup.h"
#include "cPendulum.h"

using namespace chai3d;
using namespace std;

/**
 * @file cCupAndBall.h
 * @class cCupAndBall
 * @brief Instance of cup-and-ball task. See Hasson et al., 2012: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3544966/
 *
 * This class instantiates a Cup-and-ball task object. The task object is a really a cGenericObject itself
 * and isn't added to the world. Instead, it is a collection of objects (a cup, a pendulum, targets, 
 * and constraints) and rules of the task.
*/

class cPendulum; //rsr needed to break circular include
class cCup; // todo: fix this issue

class cCupAndBall: public cGenericObject
{
  private:
    double escapeTheta;
    double pendulumLength;
    double ballMass;
    double cupMass;
    double gravity;
    double ballDamping;
    double targetDistance;
    cPendulum* ball; //named ball instead of pendulum for consistency with task's name
    cCup* cup;
    cWorld* world;
    cShapeBox* start;
    cShapeBox* stop;
    cVector3d* startTarget;
    cVector3d* stopTarget;
    cConstrainToLine* pathConstraint;

    string cupName;
    string ballName;

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
    cPrecisionClock* cupTaskClock;

  public:
    cCupAndBall(cWorld* worldPtr, double esc, double l, double bM, double cM);
    void startCupTask();
    void stopCupTask();
    void resetCupTask();
    void destructCupTask();

};
