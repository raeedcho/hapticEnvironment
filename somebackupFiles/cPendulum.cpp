#include "cPendulum.h"
// extern ControlData controlData;
extern HapticData hapticsData;
extern GraphicsData graphicsData;

/**
 * @param worldPtr Pointer to the world 
 * @param esc escape angle in degrees
 * @param l pendulum length in meters
 * @param bM ball mass in kg
 * 
 */
cPendulum::cPendulum(cWorld* worldPtr, double esc, double l, double bM): cGenericVisualEffect(), cGenericEffect(worldPtr)
{
  world = worldPtr;
  escapeTheta = (esc/180.0)*M_PI;
  pendulumLength = l;
  ballMass = bM;
  gravity = 9.8;
  pendulumDamping = 0;
  ballInCup = true;
  pendulumAngle = 0.0;
  pendulumVelocity = 0.0;

  double ballRadius = 0.1*pendulumLength;

  // Ball
  ball = new cShapeSphere(ballRadius);
  ball->m_material->setColorf(0.0, 0.75, 1.0);
  ball->m_material->setStiffness(0.); //rsr changed it to zero stiffness
  ball->createEffectSurface();
  ball->setLocalPos(cVector3d(0.0, hapticsData.tool->getLocalPos().y(), -pendulumLength));
  ball->setEnabled(true);
  world->addChild(ball);

  // Clock to prevent updates from happening too quickly
  pendulumClock = new cPrecisionClock();
  pendulumClock->start();
  lastUpdateTime = pendulumClock->getCurrentTimeSeconds();

  setEnableHaptic(true);
  setEnableVisual(true);
}

bool cPendulum::computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce)
{
    double currentTime = pendulumClock->getCurrentTimeSeconds();
    double dt = currentTime - lastUpdateTime;
    if (dt >= 0.002){ 
        // double cupPos = a_toolPos.y();
        hingeAcc = (a_toolVel.y() - hingeVel)/dt; 
        hingeVel = a_toolVel.y();  
        graphicsData.debuggerContent["cup sim"] = 1/dt;
        lastUpdateTime = currentTime;
        updateNextPendulumAngle(dt);
        cout<<lastUpdateTime<<endl;
    }

    if (hapticEnable == true) {
        double fBall = ballMass * pendulumLength * (computePendulumAcceleration(pendulumAngle, pendulumVelocity) * cCosRad(pendulumAngle) - (pendulumVelocity * pendulumVelocity) * cSinRad(pendulumAngle));
        double totalForce = -fBall; // this is in the opposite direction of the reaction force. Newton's third law

        if (!ballInCup){
        totalForce = 0.;
        }

        a_reactionForce.x(0.0);
        a_reactionForce.z(0.0);
        if (totalForce > hapticsData.maxForce)
            a_reactionForce.y(hapticsData.maxForce);
        else if (totalForce < -hapticsData.maxForce)
            a_reactionForce.y(-hapticsData.maxForce);
        else
            a_reactionForce.y(totalForce);      

        graphicsData.debuggerContent["fBall"] = fBall;
        return true;
    }
    else {
        a_reactionForce.zero();
        graphicsData.debuggerContent.erase("fBall");
    }
  return true;
}

void cPendulum::graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel)
{
  if (visualEnable == true) {
    double hingePosY = toolPos.y(); 
    double ballY, ballZ; 
    if (abs(pendulumAngle)>escapeTheta) {
      ballInCup = false;
    }
    if (ballInCup) {
      ballY = hingePosY + pendulumLength * cSinRad(pendulumAngle); 
      ballZ = -pendulumLength * cCosRad(pendulumAngle); 
    }
    else { //todo: simulate fall
      ballY = ball->getLocalPos().y(); 
      ballZ = 0; 
    }
    cVector3d* ballCenter = new cVector3d(0.0, ballY, ballZ);
    ball->setLocalPos(*ballCenter); 
  }
}

double cPendulum::computePendulumAcceleration(double theta, double omega) 
{
  double pendulumAcc = -(hingeAcc/pendulumLength)*cCosRad(theta) - (gravity/pendulumLength)*cSinRad(theta) - pendulumDamping*omega/(ballMass*pendulumLength*pendulumLength);
  return pendulumAcc;
}

/**
 * Use Runge-Kutta-4 to ingrate the equation of motion to give the current pendulum angle and angular velocity
 * 
 * @brief  integrate the pendulum equation to caclulate next pendulumAngle/pendulumVelocity
 * @param dt current itegration time step
 */
void cPendulum::updateNextPendulumAngle(double dt)
{
  double k1 = computePendulumAcceleration(pendulumAngle, pendulumVelocity);
  double k2 = computePendulumAcceleration(pendulumAngle + 0.5 * dt * pendulumVelocity, pendulumVelocity + 0.5 * dt * k1);
  double k3 = computePendulumAcceleration(pendulumAngle + 0.5 * dt * pendulumVelocity + 0.25 * dt * dt * k1, pendulumVelocity + 0.5 * dt * k2);
  double k4 = computePendulumAcceleration(pendulumAngle + dt * pendulumVelocity + 0.5 * dt * dt * k2, pendulumVelocity + dt * k3);

  pendulumAngle = pendulumAngle + dt * pendulumVelocity + dt * dt / 6.0 * (k1 + k2 + k3);
  pendulumVelocity = pendulumVelocity + dt / 6.0 * (k1 + 2*k2 + 2*k3 +k4);
}

bool cPendulum::setEnableHaptic(bool state)
{
    hapticEnable = state;
    return hapticEnable;
}

bool cPendulum::setEnableVisual(bool state)
{
    visualEnable = state;
    return visualEnable;
}


void cPendulum::resetPendulum()
{
  ballInCup = true;
  pendulumAngle = 0.0;
  pendulumVelocity = 0.0;
  pendulumClock->stop();
  pendulumClock->reset();
  lastUpdateTime = 0.0;
}


void cPendulum::destructPendulum()
{
    setEnableHaptic(false);
    setEnableVisual(false);
    world->deleteChild(ball);
}
