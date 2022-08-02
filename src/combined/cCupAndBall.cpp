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
cCupAndBall::cCupAndBall(cWorld* worldPtr, double esc, double l, double bM, double cM, double bDamping, double cDamping):cGenericVisualEffect(), cGenericEffect(worldPtr), cGenericStreamerObject()
{
  world = worldPtr;
  escapeTheta = (esc/180.0)*M_PI;
  pendulumLength = l;
  ballMass = bM;
  cupMass = cM;
  gravity = 9.8;
  ballDamping = bDamping;
  cupDamping = cDamping;


  ballAngle = 0.0;
  ballVelocity = 0.0;
  ballForce = 0.0;

  interactionForce = 0;

  // Geometrical dimentions. everything is scaled based on pendulum length and ball radius
  double ballRadius = 0.1*pendulumLength;

  cVector3d toolPosition = hapticsData.tool->getDeviceLocalPos();
  cupPos = toolPosition.y();
  cupVel = 0.0;
  cupAcc = 0.0;


  //Cup 
  cupMesh = new cMesh();
  cMatrix3d* rotationY = new cMatrix3d(0,0,-1 , 0,1,0 , 1,0,0);
  cMatrix3d* rotationZ = new cMatrix3d(cCosDeg(90-esc), -cSinDeg(90-esc), 0.0, cSinDeg(90-esc), cCosDeg(90-esc), 0.0, 0, 0, 1);
  cMatrix3d rotation = cMul(*rotationY, *rotationZ);
  cCreateRingSection(cupMesh, ballRadius, ballRadius, pendulumLength+2*ballRadius, esc*2, true, 10, 10, cVector3d(0,0,0), rotation, cColorf(1.0, 1.0, 0.0, 1.0));
  cupMesh->setLocalPos(0.0, cupPos, 0.0);
  cupMesh->setEnabled(true);
  world->addChild(cupMesh);
  setColorCup(1.0, 1.0, 1.0 ,0.5);

  // Ball
  ball = new cShapeSphere(ballRadius);
  ball->m_material->setColorf(0.0, 0.75, 1.0);
  ball->setLocalPos(cVector3d(0.0, cupPos, -pendulumLength));
  ball->setEnabled(true);
  world->addChild(ball);
  setColorBall(1., 1., 1., 0.5);
  ballInCup = true;


  // default settings
  enabledPathConstraint = false;
  enabledBallHaptics = true;
  enabledBallVisuals = true;
  enabledCupVisuals = true;


  // Clock for timing events
  cupTaskClock = new cPrecisionClock();
  lastModelUpdateTime = 0;
  running = false;
}

/**
 * @param a_toolPos Position of the haptic tool 
 * @param a_toolVel Velocity of the haptic tool 
 * @param a_toolID ID number of the haptic tool 
 * @param a_reactionForce Vector for storing forces to be applied the haptic tool 
 *
 * This function computes the reaction force from the ball that acts on the cup
 */
bool cCupAndBall::computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce)
{
  a_reactionForce.zero();

  if (running == true) 
  {
    // integrate the ball dynamics
    double currentTime = cupTaskClock->getCurrentTimeSeconds();
    double dt = currentTime - lastModelUpdateTime;
    if (dt >= 0.002) // integration time step is 2ms
    { 
      cupPos = a_toolPos.y();
      cupAcc = (a_toolVel.y() - cupVel)/dt; 
      cupVel = a_toolVel.y();  
      graphicsData.debuggerContent["cup sim"] = 1/dt;
      updateNextballAngle(dt);
      lastModelUpdateTime = currentTime;
    }

    // Ball Force effect
    ballForce = 0;
    if (enabledBallHaptics)
    {
      if (ballInCup)
        ballForce = -ballMass * cupAcc 
              + ballMass * pendulumLength * ballVelocity * ballVelocity * cSinRad(ballAngle)
              - ballMass * pendulumLength * computeBallAcceleration(ballAngle, ballVelocity) * cCosRad(ballAngle);
      
      graphicsData.debuggerContent["ballForce"] = ballForce;

      double cupInertiaForce = -cupMass * cupAcc; // NOTE: Robot's controller cannot simulate large cup inertia
      double totalForce = cupInertiaForce + ballForce;

      if (totalForce > hapticsData.maxForce)
        a_reactionForce.y(hapticsData.maxForce);
      else if (totalForce < -hapticsData.maxForce)
        a_reactionForce.y(-hapticsData.maxForce);
      else
        a_reactionForce.y(totalForce);
    }

    // global damping effect
    cVector3d dampingForce = cMul(-cupDamping,a_toolVel);
    a_reactionForce.add(dampingForce);

    // the path constraint effect
    if (enabledPathConstraint)
    {
      double stiffness = 100;
      double damping = 10;
      cVector3d projectedPont = cProjectPointOnLine(a_toolPos, cVector3d(-0.1,0,0), cVector3d(0,1,0)); //distance to line 
      cVector3d dalta = cSub(projectedPont, a_toolPos);
      cVector3d orthogonalVel = a_toolVel;
      orthogonalVel.y(0);
      cVector3d constraintForce = cAdd(cMul(-damping,orthogonalVel), cMul(stiffness,dalta));
      a_reactionForce.add(constraintForce);
    }
    interactionForce = a_reactionForce.y();

  }

  else 
  {
    graphicsData.debuggerContent.erase("ballForce");
  }
  return true;
}

void cCupAndBall::graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel)
{
  if (true) {
    double localCupY = toolPos.y(); 
    
    // Update cart graphics
    if (enabledCupVisuals){
      cupMesh->setLocalPos(0, localCupY, 0);
    }
    
    // Update ball graphics
    if (enabledBallVisuals){
      double ballY, ballZ; 
      if (abs(ballAngle)>escapeTheta) {
        ballInCup = false;
      }
      if (ballInCup) {
        ballY = localCupY + pendulumLength * cSinRad(ballAngle); 
        ballZ = -pendulumLength * cCosRad(ballAngle); 
      }
      else {
        ballY = ball->getLocalPos().y(); 
        ballZ = -0.15; 
      }
      cVector3d* ballCenter = new cVector3d(0.0, ballY, ballZ);
      ball->setLocalPos(*ballCenter); 
    }
  }
}

double cCupAndBall::computeBallAcceleration(double theta, double omega) 
{
  double ballAcc = -(cupAcc/pendulumLength)*cCosRad(theta) - (gravity/pendulumLength)*cSinRad(theta) - ballDamping*omega/(ballMass*pendulumLength*pendulumLength);
  return ballAcc;
}

/**
 * Use Runge-Kutta-4 to ingrate the equation of motion to give the current ball angle and angular velocity
 * 
 * @brief  integrate the ball equation to caclulate next ballAngle/ballVelocity
 * @param dt current itegration time step
 */
void cCupAndBall::updateNextballAngle(double dt)
{
  double k1 = computeBallAcceleration(ballAngle, ballVelocity);
  double k2 = computeBallAcceleration(ballAngle + 0.5 * dt * ballVelocity, ballVelocity + 0.5 * dt * k1);
  double k3 = computeBallAcceleration(ballAngle + 0.5 * dt * ballVelocity + 0.25 * dt * dt * k1, ballVelocity + 0.5 * dt * k2);
  double k4 = computeBallAcceleration(ballAngle + dt * ballVelocity + 0.5 * dt * dt * k2, ballVelocity + dt * k3);

  ballAngle = ballAngle + dt * ballVelocity + dt * dt / 6.0 * (k1 + k2 + k3);
  ballVelocity = ballVelocity + dt / 6.0 * (k1 + 2*k2 + 2*k3 +k4);
}

void cCupAndBall::startCupAndBall()
{
  setColorBall(1., 0., 0., 1.);
  setColorCup(0.2, 0.5, 1., 1.);
  running = true;
  ballInCup = true;
  cupTaskClock->start();
  lastModelUpdateTime = 0;
}

void cCupAndBall::stopCupAndBall()
{
  setColorBall(1., 1., 1., 0.5);
  setColorCup(1., 1., 1., 0.5);
  running = false;
  ballInCup = true;
  ballAngle = 0.0;
  ballVelocity = 0.0;
  ballForce = 0.0;
  interactionForce = 0;
  // cupPos = -100; //rsr do not reset poisition for visual consistency
  cupVel = 0.0;
  cupAcc = 0.0;
  cupTaskClock->stop();
  cupTaskClock->reset();
}

// for now resetCupAndBall and stopCupAndBall do the same thing. TODO: Reset moves the robot back to origin?
void cCupAndBall::resetCupAndBall()
{
  running = false;
  ballInCup = true;
  ballAngle = 0.0;
  ballVelocity = 0.0;
  ballForce = 0.0;
  interactionForce = 0;
  // cupPos = -100; //rsr do not reset poisition for visual consistency
  cupVel = 0.0;
  cupAcc = 0.0;
  cupTaskClock->stop();
  cupTaskClock->reset();
  lastModelUpdateTime = 0;
}


void cCupAndBall::destructCupAndBall()
{
  stopCupAndBall();
  world->deleteChild(ball);
  world->deleteChild(cupMesh);
}



bool cCupAndBall::setEnablePathConstraint(bool state)
{
  enabledPathConstraint = state;
  return enabledPathConstraint;
}

bool cCupAndBall::setEnableBallVisuals(bool state)
{
  enabledBallVisuals = state;
  ball->setShowEnabled(enabledBallVisuals);
  return enabledBallVisuals;
}

bool cCupAndBall::setEnableBallHaptics(bool state)
{
  enabledBallHaptics = state;
  ball->setHapticEnabled(enabledBallHaptics);
  return enabledBallHaptics;
}

bool cCupAndBall::setEnableCupVisuals(bool state)
{
  enabledCupVisuals = state;
  cupMesh->setShowEnabled(enabledCupVisuals);
  return enabledCupVisuals;
}



bool cCupAndBall::getEnablePathConstraint()
{
  return enabledPathConstraint;
}

bool cCupAndBall::getEnableBallVisuals()
{
  return enabledBallVisuals;
}

bool cCupAndBall::getEnableBallHaptics()
{
  return enabledBallHaptics;
}

bool cCupAndBall::getEnableCupVisuals()
{
  return enabledCupVisuals;
}


void cCupAndBall::setColorBall(double r, double g, double b, double alpha)
{
  ball->m_material->setColorf(r,g,b,alpha);
}

void cCupAndBall::setColorCup(double r, double g, double b, double alpha)
{
  cupMesh->m_material->setColorf(r,g,b,alpha);
}


void cCupAndBall::sendData()
{
  M_CUPTASK_DATA msg;
  memset(&msg, 0, sizeof(msg));
  msg.header = createMesssageHeader(CUPTASK_DATA);
  msg.time = cupTaskClock->getCurrentTimeSeconds();
  msg.ballAngle = ballAngle;
  msg.ballForce = ballForce;
  msg.interactionForce = interactionForce;
  msg.cupPos = cupPos;
  char packet[sizeof(msg)];
  memcpy(&packet, &msg, sizeof(msg));
  vector<char> packetData(packet, packet+sizeof(packet) / sizeof(char));
  auto results = controlData.client->async_call("sendMessage", packetData, sizeof(msg), controlData.MODULE_NUM);    
  results.wait();  
}