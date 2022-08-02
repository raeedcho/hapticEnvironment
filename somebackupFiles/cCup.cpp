#include "cCup.h"
extern ControlData controlData;
extern HapticData hapticsData;
extern GraphicsData graphicsData;

/**
 * @param worldPtr Pointer to the world 
 * @param rimAngle angle to the edge of the cup, in degrees
 * @param cupRadius cup inner radius, in mm
 * @param cM cup mass in kg
 * @param color Color of the cup
 * 
 */
cCup::cCup(cWorld* worldPtr, double rimAngle, double cupRadius, double cM):cGenericVisualEffect(), cGenericEffect(worldPtr)
{
  world = worldPtr;
  innerRadius = cupRadius;
  double torusRadius = 0.1*innerRadius;
  cupMass = cM;

  cVector3d toolPosition;
  toolPosition = hapticsData.tool->getLocalPos();
  cupPos = toolPosition.y();
  cupVel = 0.0;
  cupAcc = 0.0;
  cColorf cupColor = cColorf(1,1,0,1);

  //Cup 
  cupMesh = new cMesh();
  cMatrix3d* rotationY = new cMatrix3d(0,0,-1 , 0,1,0 , 1,0,0);
  cMatrix3d* rotationZ = new cMatrix3d(cCosDeg(90-rimAngle), -cSinDeg(90-rimAngle), 0.0, cSinDeg(90-rimAngle), cCosDeg(90-rimAngle), 0.0, 0, 0, 1);
  cMatrix3d rotation = cMul(*rotationY, *rotationZ);
  cVector3d cupCenterPosition = cVector3d(0.0, cupPos, 0.0);
  cCreateRingSection(cupMesh, torusRadius, torusRadius, innerRadius+2*torusRadius, rimAngle*2, true, 10, 10, cupCenterPosition, rotation, cupColor);
  cupMesh->setEnabled(true);
  cupMesh->createEffectSurface();
  cupMesh->m_material->setStiffness(0.0); //rsr changed it to zero stiffness
  world->addChild(cupMesh);

  enable = true;
}

bool cCup::setEnable(bool state)
{
    enable = state;
    return enable;
}

bool cCup::computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce)
{
    a_reactionForce.zero();
    return true;
}

void cCup::graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel)
{
    if (enable) {
        double localCupY = toolPos.y(); 
        cupMesh->setLocalPos(0.0, localCupY, 0.0);
    }
}

void cCup::destructCup()
{
    setEnable(false);
    usleep(500);
    world->deleteChild(cupMesh);
}
