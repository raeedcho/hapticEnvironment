#include "cCups.h"
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
cCups::cCups(cWorld* worldPtr, double esc, double l, double bM, double cM):cGenericMovingObject(), cGenericEffect(worldPtr)
{
  world = worldPtr;
  escapeTheta = (esc/180.0)*M_PI;
  pendulumLength = l;
  ballMass = bM;
  cupMass = cM;
  gravity = 9.8;
  ballDamping = 0;


  ballAngle = 0.0;
  ballVelocity = 0.0;
  ballForce = 0.0;

  interactionForce = 0;

  // Gepmetrical dimentions
  double ballRadius = 0.1*pendulumLength;
  double boxDepth = 0.01;
  double boxWidth = 1.3 * pendulumLength*cSinRad(escapeTheta)*2;
  double boxHeight = pendulumLength*(1-cCosRad(escapeTheta)) + 3*ballRadius;

  targetDistance = 0.3;
  startTarget = new cVector3d(-0.1, -targetDistance/2, boxHeight/2 - pendulumLength - 3*ballRadius);
  stopTarget  = new cVector3d(-0.1,  targetDistance/2, boxHeight/2 - pendulumLength - 3*ballRadius);
  
  //Start and stop boxes
  start = new cShapeBox(boxDepth, boxWidth, boxHeight);
  start->setLocalPos(*startTarget);
  start->m_material->setColorf(0.0, 1.0, 0.0);
  world->addChild(start);

  stop = new cShapeBox(boxDepth, boxWidth, boxHeight);
  stop->setLocalPos(*stopTarget);
  stop->m_material->setColorf(0.0, 1.0, 0.0);
  world->addChild(stop);


  cVector3d toolPosition;
  toolPosition = hapticsData.tool->getLocalPos();
  cupPos = toolPosition.y();
  cupVel = 0.0;
  cupAcc = 0.0;

  //Cup 
  cupMesh = new cMesh();
  cMatrix3d* rotationY = new cMatrix3d(0,0,-1 , 0,1,0 , 1,0,0);
  cMatrix3d* rotationZ = new cMatrix3d(cCosDeg(90-esc), -cSinDeg(90-esc), 0.0, cSinDeg(90-esc), cCosDeg(90-esc), 0.0, 0, 0, 1);
  cMatrix3d rotation = cMul(*rotationY, *rotationZ);
  cVector3d cupCenterPosition = cVector3d(0.0, cupPos, 0.0);
  cCreateRingSection(cupMesh, ballRadius, ballRadius, pendulumLength+2*ballRadius, esc*2, true, 10, 10, cupCenterPosition, rotation, cColorf(1.0, 1.0, 0.0, 1.0));
  cupMesh->setEnabled(true);
  cupMesh->createEffectSurface();
  cupMesh->m_material->setStiffness(0.0); //rsr changed it to zero stiffness
  world->addChild(cupMesh);

  // Ball
  ball = new cShapeSphere(ballRadius);
  ball->m_material->setColorf(0.0, 0.75, 1.0);
  ball->m_material->setStiffness(0.); //rsr changed it to zero stiffness
  ball->createEffectSurface();
  ball->setLocalPos(cVector3d(0.0, cupPos, -pendulumLength));
  ball->setEnabled(true);
  world->addChild(ball);
  ballInCup = true;



  // Background world viscosity
  hapticsData.tool->m_material->setViscosity(0.05*hapticsData.hapticDeviceInfo.m_maxLinearDamping);
  hapticsData.tool->createEffectViscosity();

  // Linear path constraint 
  // double constraintStiffness = 500; 
  // cVector3d* point1 = new cVector3d(0,-1,0);
  // cVector3d* point2 = new cVector3d(0,1,0);
  // pathConstraint = new cConstrainToLine(world, point1, point2, constraintStiffness);
  // pathConstraint->setEnabled(true);
  // controlData.worldEffects["pathConstraint"] = pathConstraint;

  // Clock to prevent updates from happening too quickly
  cupsClock = new cPrecisionClock();
  lastUpdateTime = 0.0;
  running = false;
}

bool cCups::computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel,
                      const unsigned int& a_toolID, cVector3d& a_reactionForce)
{
  if (running == true) {
    double cupsTime = cupsClock->getCurrentTimeSeconds();
    double dt = cupsTime - lastUpdateTime;
    // cupPos = a_toolPos.y();
    // cupAcc = ((a_toolVel.y() - cupVel)/dt + cupAcc)/2; 
    // cupVel = a_toolVel.y();  
    if (dt >= 0.002){ 
      cupPos = a_toolPos.y();
      cupAcc = (a_toolVel.y() - cupVel)/dt; 
      cupVel = a_toolVel.y();  
      graphicsData.debuggerContent["cup sim"] = 1/dt;
      lastUpdateTime = cupsTime;
      updateNextballAngle(dt);
    }
    double fBall = ballMass * pendulumLength * (computeBallAcceleration(ballAngle, ballVelocity) * cCosRad(ballAngle) - (ballVelocity * ballVelocity) * cSinRad(ballAngle));
    double fInertia = (cupMass + ballMass) * cupAcc;
    double totalForce = 1*(fInertia*0 - fBall); // this is in the opposite direction of the reaction force. Newton's third law

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


    // the path constraint
    double stiffness = 100;
    double damping = 10;
    cVector3d projectedPont = cProjectPointOnLine(a_toolPos, cVector3d(-0.1,0,0), cVector3d(0,1,0)); //distance to line 
    cVector3d dalta = cSub(projectedPont, a_toolPos);
    cVector3d orthogonalVel = a_toolVel;
    orthogonalVel.y(0);
    cVector3d constraingForce = cAdd(cMul(-damping,orthogonalVel), cMul(stiffness,dalta));
    a_reactionForce.add(constraingForce);

    graphicsData.debuggerContent["fBall"] = fBall;
    graphicsData.debuggerContent["fInertia"] = fInertia;
    return true;
  }
  else {
    a_reactionForce.zero();
    graphicsData.debuggerContent.erase("fBall");
    graphicsData.debuggerContent.erase("fInertia");
  }
  return true;
}

void cCups::graphicsLoopFunction(double dt, cVector3d toolPos, cVector3d toolVel)
{
  if (running == true) {
    // Update cart graphics
    double localCupY = toolPos.y(); //rsr. to read current tool position, but at the same time not overwriting the cupPos variable (which is done in the computerForce loop)
    cupMesh->setLocalPos(0.0, localCupY, 0.0);

    
    // Update ball graphics
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
      ballZ = 0; 
    }
    cVector3d* ballCenter = new cVector3d(0.0, ballY, ballZ);
    ball->setLocalPos(*ballCenter); 
  }
}

double cCups::computeBallAcceleration(double theta, double omega) 
{
  // TODO. get ball acc from states, not the cupAcc for less noise
  double ballAcc = -(cupAcc/pendulumLength)*cCosRad(theta) - (gravity/pendulumLength)*cSinRad(theta) - ballDamping*omega/(ballMass*pendulumLength*pendulumLength);
  return ballAcc;
}

/**
 * Use Runge-Kutta-4 to ingrate the equation of motion to give the current ball angle and angular velocity
 * 
 * @brief  integrate the ball equation to caclulate next ballAngle/ballVelocity
 * @param dt current itegration time step
 */
void cCups::updateNextballAngle(double dt)
{
  double k1 = computeBallAcceleration(ballAngle, ballVelocity);
  double k2 = computeBallAcceleration(ballAngle + 0.5 * dt * ballVelocity, ballVelocity + 0.5 * dt * k1);
  double k3 = computeBallAcceleration(ballAngle + 0.5 * dt * ballVelocity + 0.25 * dt * dt * k1, ballVelocity + 0.5 * dt * k2);
  double k4 = computeBallAcceleration(ballAngle + dt * ballVelocity + 0.5 * dt * dt * k2, ballVelocity + dt * k3);

  ballAngle = ballAngle + dt * ballVelocity + dt * dt / 6.0 * (k1 + k2 + k3);
  ballVelocity = ballVelocity + dt / 6.0 * (k1 + 2*k2 + 2*k3 +k4);

  
  // M_CUPS_DATA cupsData;
  // memset(&cupsData, 0, sizeof(cupsData));
  // auto packetIdx = controlData.client->async_call("getMsgNum");
  // auto timestamp = controlData.client->async_call("getTimestamp");
  // packetIdx.wait();
  // timestamp.wait();
  // int packetNum = packetIdx.get().as<int>();
  // double currTime = timestamp.get().as<double>();
  // cupsData.header.serial_no = packetNum;
  // cupsData.header.msg_type = CUPS_DATA;
  // cupsData.header.timestamp = currTime;
  // cupsData.ballAngle = ballAngle;
  // cupsData.cupPos = cupPos;
  // char packet[sizeof(cupsData)];
  // memcpy(&packet, &cupsData, sizeof(cupsData));
  //   vector<char> packetData(packet, packet+sizeof(packet) / sizeof(char));

  // auto cupsInt = controlData.client->async_call("sendMessage", packetData, sizeof(cupsData), controlData.MODULE_NUM);    

}

void cCups::startCups()
{
  running = true;
  ballInCup = true;
  cupsClock->start();
  lastUpdateTime = cupsClock->getCurrentTimeSeconds();
}

void cCups::stopCups()
{
  running = false;
  ballInCup = true;
  ballAngle = 0.0;
  ballVelocity = 0.0;
  ballForce = 0.0;
  // cupPos = -100; //rsr do not reset poisition for visual consistency
  cupVel = 0.0;
  cupAcc = 0.0;
  cupsClock->stop();
  cupsClock->reset();
}

// for now resetCups and stopCups do the same thing. TODO: Reset moves the robot back to 
void cCups::resetCups()
{
  running = false;
  ballInCup = true;
  ballAngle = 0.0;
  ballVelocity = 0.0;
  ballForce = 0.0;
  // cupPos = -100; //rsr do not reset poisition for visual consistency
  cupVel = 0.0;
  cupAcc = 0.0;
  cupsClock->stop();
  cupsClock->reset();
}


void cCups::destructCups()
{
  world->deleteChild(ball);
  world->deleteChild(cupMesh);
  world->deleteChild(start);
  world->deleteChild(stop);
  hapticsData.tool->deleteAllEffects();
}
