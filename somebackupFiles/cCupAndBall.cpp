#include "cCupAndBall.h"
extern ControlData controlData;
extern HapticData hapticsData;
extern GraphicsData graphicsData;

/**
 * @param worldPtr Pointer to the world 
 * @param esc escape angle in degrees
 * @param l pendulum length in mm
 * @param bM ball mass in kg
 * @param cM cart mass in kg
 * 
 */
cCupAndBall::cCupAndBall(cWorld* worldPtr, double esc, double l, double bM, double cM): cGenericObject()
{
    world = worldPtr;
    escapeTheta = (esc/180.0)*M_PI;
    pendulumLength = l;
    ballMass = bM;
    cupMass = cM;
    gravity = 9.8;
    ballDamping = 0;

    cPendulum* ball = new cPendulum(world, esc, pendulumLength, ballMass);
    string ballName = "ball";
    controlData.objectMap[ballName] = ball;
    graphicsData.visualEffectsList.push_back(ball);
    hapticsData.hapticEffectsMap[ballName] = ball;
    world->addEffect(ball);

    cCup* cup = new cCup(world, esc, pendulumLength, cupMass);
    string cupName = "cup";
    controlData.objectMap[cupName] = cup;
    graphicsData.visualEffectsList.push_back(cup);
    hapticsData.hapticEffectsMap[cupName] = cup;
    world->addEffect(cup);

    cVector3d toolPosition;
    toolPosition = hapticsData.tool->getLocalPos();
    cupPos = toolPosition.y();
    cupVel = 0.0;
    cupAcc = 0.0;

    // Background world viscosity
    hapticsData.tool->m_material->setViscosity(0.05*hapticsData.hapticDeviceInfo.m_maxLinearDamping);
    hapticsData.tool->createEffectViscosity();

    // Clock to prevent updates from happening too quickly
    cupTaskClock = new cPrecisionClock();
    lastUpdateTime = 0.0;
    running = false;
    ball->resetPendulum();
}

void cCupAndBall::startCupTask()
{
    running = true;
    cupTaskClock->start();
    lastUpdateTime = cupTaskClock->getCurrentTimeSeconds();
}

void cCupAndBall::stopCupTask()
{
    running = false;
    ballInCup = true;
    ballAngle = 0.0;
    ballVelocity = 0.0;
    ballForce = 0.0;
    // cupPos = -100; //rsr do not reset poisition for visual consistency
    cupVel = 0.0;
    cupAcc = 0.0;
    cupTaskClock->stop();
    cupTaskClock->reset();
}

// for now resetCupTask and stopCupTask do the same thing. TODO: Reset moves the robot back to 
void cCupAndBall::resetCupTask()
{
    running = false;
    ballInCup = true;
    ball->resetPendulum();
    ballAngle = 0.0;
    ballVelocity = 0.0;
    ballForce = 0.0;
    // cupPos = -100; //rsr do not reset poisition for visual consistency
    cupVel = 0.0;
    cupAcc = 0.0;
    cupTaskClock->stop();
    cupTaskClock->reset();
}


void cCupAndBall::destructCupTask()
{
    remove(graphicsData.visualEffectsList.begin(), graphicsData.visualEffectsList.end(), cup);
    hapticsData.hapticEffectsMap.erase(cupName);
    cup->destructCup();
    world->deleteChild(cup);

    remove(graphicsData.visualEffectsList.begin(), graphicsData.visualEffectsList.end(), ball);
    hapticsData.hapticEffectsMap.erase(ballName);
    ball->destructPendulum();
    world->deleteChild(ball);

    hapticsData.tool->deleteAllEffects();
}
