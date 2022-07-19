#include "controller.h"

/**
 * @file controller.h
 * @file controller.cpp 
 * @brief This file contains the main function that runs the haptic, graphics, and message listening
 * loops.
 *
 * This file is in the global namespace. It accesses haptic data and graphics data through external
 * structs, and keeps track of all the messaging sockets and Chai3D threads through the ControlData
 * extern, which is accessible in other files. 
 */


extern HapticData hapticsData; // rsr. extern tells the compiler it is defined in another module. Linker finds this variable points to it. It's like a global variable. HaptidData is defined in haptics.h
extern GraphicsData graphicsData;
ControlData controlData; // rsr. control data is defined in controller.h, included above


/////////////////////////////////////////////////////
////////// BEGINING OF THE MAIN PROGRAM /////////////
/////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  cout << endl;
  cout << "-----------------------------------" << endl;
  cout << "CHAI3D" << endl;
  cout << "-----------------------------------" << endl << endl << endl;
  cout << "Keyboard Options:" << endl << endl;
  cout << "[f] - Enable/Disable full screen mode" << endl;
  cout << "[d] - Enable/Disable debugging info" << endl;
  cout << "[q] - Exit application" << endl;
  cout << endl << endl;
  
  atexit(close); //rsr. close() is the "exit" function that is called when the program ends.
  
  const char* MODULE_IP;
  int MODULE_PORT;
  const char* MH_IP;
  int MH_PORT;


  // rsr. controlData is a structure containing high-level information about the flow of the program
  controlData.simulationRunning = false;
  controlData.simulationFinished = true;
  controlData.hapticsUp = false;
  controlData.listenerUp = false;
  controlData.streamerUp = false;
  controlData.loggingData = false;

  // TODO: Set these IP addresses from a config file
  //controlData.LISTENER_IP = "127.0.0.1";
  //controlData.LISTENER_PORT = 7000;
  controlData.MODULE_NUM = 1;
  if (argc < 2) {
    controlData.IPADDR = "127.0.0.1";
    controlData.PORT = 7000;
  }
  else {
    controlData.IPADDR = argv[1];
    controlData.PORT = atoi(argv[2]);
  }
  if (argc <= 3) {
    controlData.MH_IP = "127.0.0.1";
    controlData.MH_PORT = 8080;
  }
  else {
    controlData.MH_IP = argv[3];
    controlData.MH_PORT = atoi(argv[4]);
  }

  // rsr. this is a clinet for the RPC server (being the messageHandler app). The client sends function call requests to the server to run and waits for the return value
  controlData.client = new rpc::client(controlData.MH_IP, controlData.MH_PORT);
  //controlData.SENDER_IPS.push_back("127.0.0.1");
  //controlData.SENDER_IPS.push_back("127.0.0.1");
  //controlData.SENDER_PORTS.push_back(9000);
  //controlData.SENDER_PORTS.push_back(10000);
  

  
  controlData.hapticsOnly = false;
  if (controlData.hapticsOnly == false) {
    initDisplay(); //rsr. initializes glfw window. construct the global variable graphicsData. 
    initScene(); //rsr. initializes the chai3d world, camera, light. puts them in graphicsData
    // resizeWindowCallback(graphicsData.window, graphicsData.width, graphicsData.height); //rsr. didn't seem necessary
  }
  
  initHaptics(); //rsr. initializes the haptic device. contructs the global variable hapticsData. 
  startHapticsThread(); //rsr. creates a new thread to run the haptics loop. simulation starts here
  usleep(2e5);

  openMessagingSocket(); //rsr. from network.h. The socker is used to listen to incomming UDP messages sent by messageHandler. The listenerThread listens to incoming messages
  int addSuccess = addMessageHandlerModule(); //rsr. adds this program as module#1 to the messageHandler
  if (addSuccess == 0) {
    cout << "Module addition failed" << endl;
    close();
    exit(1);
  }
  usleep(1e5);
  int subscribeSuccess = subscribeToTrialControl(); //rsr. subscribing to the "trialController", so that the messages requested by trialController are sent to module#1's socket
  if (subscribeSuccess == 0) {
    cout << "Subcribe to Trial Control failed" << endl;
    close();
    exit(1);
  }

  //rsr added to subscribe to my own message requests
  bool subscribed = false;
  while (subscribed == false) {
    auto subscribe = controlData.client->async_call("subscribeTo", controlData.MODULE_NUM, controlData.MODULE_NUM);
    subscribe.wait();
    if (subscribe.get().as<int>() == 1) {
      subscribed = true;
      cout << "subscribed to self" << endl;
    }
  }
      
  usleep(1e5);
  startStreamerThread(); // rsr. starts a new thread to read pos/force/etc from the robot. for the streamer pointed by controlData.streamerThread
  startListenerThread(); // rsr. starts a new thread to listen to messageHandler?! there is a difference between streamer and listener. 
  cout << "streamer and listener started" << endl;

  startGraphicsLoop(); //rsr. starts the main thread: a loop that updates graphics. code stucks here until the end of program

  return(0);
}

/**
 * Checks if the haptics and messaging threads have exited yet. The graphics loop is in main, and
 * exits when all other threads are down, so it is not checked here.
 */
bool allThreadsDown()
{
  return (controlData.hapticsUp && controlData.listenerUp && controlData.streamerUp);  
}

/**
 * Ends the program. This method does so by setting the "simulationRunning" boolean to false. When
 * false, other threads will exit. To exit gracefully, this method waits until all threads have
 * returned before stopping the haptic tool and exiting the graphic interface.
 */
void close()
{
  controlData.simulationRunning = false; //rsr. this stops haptic/listener/streamer loops. they all depend on this
  while (!controlData.simulationFinished) {
    controlData.simulationFinished = allThreadsDown();
    cSleepMs(100);
  }
  hapticsData.tool->stop();
  cout << "Haptic tool stopped" << endl;
  delete hapticsData.hapticsThread;
  cout << "Deleted haptics thread" << endl;
  delete graphicsData.world;
  cout << "Deleted world" << endl;
  delete hapticsData.handler;
  cout << "Deleted handler" << endl;
  closeMessagingSocket();
  graphicsData.world->deleteAllChildren();
  
  glfwDestroyWindow(graphicsData.window); // rsr. moved from main function to here
  glfwTerminate();

}

/**
 * This function receives packets from the listener threads and updates the haptic environment
 * variables accordingly.
 * @param packet is a pointer to a char array of bytes
 */
void parsePacket(char* packet)
{
  MSG_HEADER header;
  memcpy(&header, packet, sizeof(header));
  int msgType = header.msg_type;
  switch (msgType)
  {
    /////////////////////// GENERAL MESSEGES /////////////
    case SESSION_START:
    {
      cout << "Received SESSION_START Message" << endl;
      break;
    }

    case SESSION_END:
    {
      cout << "Received SESSION_END Message" << endl;
      controlData.simulationRunning = false;
      close();
      break;
    }

    case TRIAL_START:
    {
      cout << "Received TRIAL_START Message" << endl;
      break;
    }

    case TRIAL_END:
    {
      cout << "Received TRIAL_END Message" << endl;
      break;
    }

    case START_RECORDING:
    {
      cout << "Received START_RECORDING Message" << endl;
      M_START_RECORDING recInfo;
      memcpy(&recInfo, packet, sizeof(recInfo));
      char* fileName;
      fileName = recInfo.filename;
      controlData.dataFile.open(fileName, ofstream::binary);
      controlData.dataFile.flush();
      controlData.loggingData = true;
      break; 
    }

    case STOP_RECORDING:
    {
      cout << "Received STOP_RECORDING Message" << endl;
      controlData.dataFile.close();
      controlData.loggingData = false;
      break;
    }
    
    case REMOVE_OBJECT:
    {
      cout << "Received REMOVE_OBJECT Message" << endl;
      M_REMOVE_OBJECT rmObj;
      memcpy(&rmObj, packet, sizeof(rmObj));
      if (controlData.objectMap.find(rmObj.objectName) == controlData.objectMap.end()) {
        cout << rmObj.objectName << " not found" << endl;
      }
      else {
        cGenericObject* objPtr = controlData.objectMap[rmObj.objectName];
        graphicsData.world->deleteChild(objPtr);
        controlData.objectMap.erase(rmObj.objectName);
      }
      break;
    }
    
    case RESET_WORLD:
    {
      cout << "Received RESET_WORLD Message" << endl;
      unordered_map<string, cGenericObject*>::iterator objIt = controlData.objectMap.begin();
      while (objIt != controlData.objectMap.end()) {
        bool removedObj = graphicsData.world->deleteChild(objIt->second);
        objIt++;
      }
      unordered_map<string, cGenericEffect*>::iterator effIt = controlData.worldEffects.begin();
      while (effIt != controlData.worldEffects.end()) {
        bool removedEffect = graphicsData.world->removeEffect(effIt->second);
        effIt++;
      }
      controlData.objectMap.clear();
      controlData.objectEffects.clear();
      controlData.worldEffects.clear();
      break;
    }


    /////////////////////// CST MESSEGES /////////////
    case CST_CREATE:
    {
      cout << "Received CST_CREATE Message" << endl;
      M_CST_CREATE cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      cCST* cst = new cCST(graphicsData.world, cstObj.lambdaVal, 
          cstObj.forceMagnitude, cstObj.visionEnabled, cstObj.hapticEnabled);
      char* cstName = cstObj.cstName;
      controlData.objectMap[cstName] = cst;
      graphicsData.movingObjects.push_back(cst);
      graphicsData.world->addEffect(cst);
      controlData.worldEffects[cstName] = cst;
      break;
    }
    case CST_DESTRUCT:
    {
      cout << "Received CST_DESTRUCT Message" << endl;
      M_CST_DESTRUCT cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      if (controlData.objectMap.find(cstObj.cstName) == controlData.objectMap.end()) {
        cout << cstObj.cstName << " not found" << endl;
      }
      else {
        cCST* cst = dynamic_cast<cCST*>(controlData.objectMap[cstObj.cstName]);
        cst->stopCST();
        cst->destructCST();
        remove(graphicsData.movingObjects.begin(), graphicsData.movingObjects.end(), cst);
        controlData.worldEffects.erase(cstObj.cstName);
        bool removedCST = graphicsData.world->removeEffect(cst);
        //rsr. why isn't cst removed from objectMap? it still stays there...
      }
      break;
    }
    case CST_START:
    {
      cout << "Received CST_START Message" << endl;
      M_CST_START cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      cCST* cst = dynamic_cast<cCST*>(controlData.objectMap[cstObj.cstName]);
      hapticsData.tool->setShowEnabled(false); //rsr. tool disapears. cst visual is visible
      cst->startCST();
      break;
    }
    case CST_STOP:
    {
      cout << "Received CST_STOP Message" << endl;
      M_CST_STOP cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      cCST* cst = dynamic_cast<cCST*>(controlData.objectMap[cstObj.cstName]);
      cst->stopCST();
      hapticsData.tool->setShowEnabled(true); //rsr. tool is visible again
      break;
    }
    case CST_SET_VISUAL:
    {
      cout << "Received CST_SET_VISUAL Message" << endl;
      M_CST_SET_VISUAL cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      bool visual = cstObj.visionEnabled;
      cCST* cst = dynamic_cast<cCST*>(controlData.objectMap[cstObj.cstName]);
      cst->setVisionEnabled(visual);
      break;
    }
    case CST_SET_HAPTIC:
    {
      cout << "Received CST_SET_HAPTIC Message" << endl;
      M_CST_SET_HAPTIC cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      bool haptic = cstObj.hapticEnabled;
      cCST* cst = dynamic_cast<cCST*>(controlData.objectMap[cstObj.cstName]);
      cst->setHapticEnabled(haptic);
      break;
    }
    case CST_SET_LAMBDA:
    {
      cout << "Received CST_SET_LAMBDA Message" << endl;
      M_CST_SET_LAMBDA cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      double lambda = cstObj.lambdaVal;
      cCST* cst = dynamic_cast<cCST*>(controlData.objectMap[cstObj.cstName]);
      cst->setLambda(lambda);
      break;
    }



    /////////////////////// CUP TASK MESSEGES /////////////
    case CUPS_CREATE:
    {
      cout << "Received CUPS_CREATE Message" << endl;
      M_CUPS_CREATE createCups;
      memcpy(&createCups, packet, sizeof(createCups));
      cCups* cups = new cCups(graphicsData.world, createCups.escapeAngle, 
          createCups.pendulumLength, createCups.ballMass, createCups.cartMass);
      char* cupsName = createCups.cupsName;
      controlData.objectMap[cupsName] = cups;
      graphicsData.movingObjects.push_back(cups);
      graphicsData.world->addEffect(cups);
      controlData.worldEffects[cupsName] = cups;
      break;
    }
    case CUPS_DESTRUCT:
    {
      cout << "Received CUPS_DESTRUCT Message" << endl;
      M_CUPS_DESTRUCT cupsObj;
      memcpy(&cupsObj, packet, sizeof(cupsObj));
      if (controlData.objectMap.find(cupsObj.cupsName) == controlData.objectMap.end()) {
        cout << cupsObj.cupsName << " not found" << endl;
      }
      else {
        cCups* cups = dynamic_cast<cCups*>(controlData.objectMap[cupsObj.cupsName]);
        cups->stopCups();
        cups->destructCups();
        remove(graphicsData.movingObjects.begin(), graphicsData.movingObjects.end(), cups);
        controlData.worldEffects.erase(cupsObj.cupsName);
        bool removedCups = graphicsData.world->removeEffect(cups);
      }
      break;
    }
    case CUPS_START:
    {
      cout << "Received CUPS_START Message" << endl;
      M_CUPS_START cupsObj;
      memcpy(&cupsObj, packet, sizeof(cupsObj));
      cCups* cups = dynamic_cast<cCups*>(controlData.objectMap[cupsObj.cupsName]);
      hapticsData.tool->setShowEnabled(false);
      cups->startCups();
      break;
    }
    case CUPS_STOP:
    {
      cout << "Received CUPS_STOP Message" << endl;
      M_CUPS_STOP cupsObj;
      memcpy(&cupsObj, packet, sizeof(cupsObj));
      cCups* cups = dynamic_cast<cCups*>(controlData.objectMap[cupsObj.cupsName]);
      cups->stopCups();
      hapticsData.tool->setShowEnabled(true);
      break;
    }

    case CUPS_RESET:
    {
      cout << "Received CUPS_RESET Message" << endl;
      M_CUPS_RESET cupsObj;
      memcpy(&cupsObj, packet, sizeof(cupsObj));
      cCups* cups = dynamic_cast<cCups*>(controlData.objectMap[cupsObj.cupsName]);
      cups->resetCups();
      break;
    }

    // OTHER GENERAL MESSAGES
    case HAPTICS_SET_ENABLED:
    {
      cout << "Received HAPTICS_SET_ENABLED Message" << endl;
      M_HAPTICS_SET_ENABLED hapticsEnabled;
      memcpy(&hapticsEnabled, packet, sizeof(hapticsEnabled));
      char* objectName;
      objectName = hapticsEnabled.objectName;
      if (controlData.objectMap.find(objectName) == controlData.objectMap.end()) {
        cout << objectName << " not found" << endl;
      }
      else {
        if (hapticsEnabled.enabled == 1) {
          controlData.objectMap[objectName]->setHapticEnabled(true);
        }
        else if (hapticsEnabled.enabled == 0) {
          controlData.objectMap[objectName]->setHapticEnabled(false);
        }
      }
      break;
    }

    case HAPTICS_SET_ENABLED_WORLD:
    {
      M_HAPTICS_SET_ENABLED_WORLD worldEnabled;
      memcpy(&worldEnabled, packet, sizeof(worldEnabled));
      char* effectName;
      effectName = worldEnabled.effectName;
      cGenericEffect* fieldEffect = controlData.worldEffects[effectName];
      fieldEffect->setEnabled(worldEnabled.enabled);
      break; 
    }

    case HAPTICS_SET_STIFFNESS:
    {
      cout << "Received HAPTICS_SET_STIFFNESS Message" << endl;
      M_HAPTICS_SET_STIFFNESS stiffness;
      memcpy(&stiffness, packet, sizeof(stiffness));
      char* objectName;
      objectName = stiffness.objectName;
      if (controlData.objectMap.find(objectName) == controlData.objectMap.end()) {
        cout << objectName << " not found" << endl;
      }
      else {
        controlData.objectMap[objectName]->m_material->setStiffness(stiffness.stiffness);
      }
      break;
    }

    case HAPTICS_BOUNDING_PLANE:
    {
      cout << "Received HAPTICS_BOUNDING_PLANE Message" << endl;
      M_HAPTICS_BOUNDING_PLANE bpMsg;
      memcpy(&bpMsg, packet, sizeof(bpMsg));
      double bWidth = bpMsg.bWidth;
      double bHeight = bpMsg.bHeight;
      int stiffness = hapticsData.hapticDeviceInfo.m_maxLinearStiffness;
      double toolRadius = hapticsData.toolRadius;
      cBoundingPlane* bp = new cBoundingPlane(stiffness, toolRadius, bWidth, bHeight);
      graphicsData.world->addChild(bp->getLowerBoundingPlane());
      graphicsData.world->addChild(bp->getUpperBoundingPlane());
      graphicsData.world->addChild(bp->getTopBoundingPlane());
      graphicsData.world->addChild(bp->getBottomBoundingPlane());
      graphicsData.world->addChild(bp->getLeftBoundingPlane());
      graphicsData.world->addChild(bp->getRightBoundingPlane());
      controlData.objectMap["boundingPlane"] = bp;
      break;
    }
    
    case HAPTICS_CONSTANT_FORCE_FIELD:
    {
      cout << "Received HAPTICS_CONSTANT_FORCE_FIELD Message" << endl;
      M_HAPTICS_CONSTANT_FORCE_FIELD cffInfo;
      memcpy(&cffInfo, packet, sizeof(cffInfo));
      double d = cffInfo.direction;
      double m = cffInfo.magnitude;
      cConstantForceFieldEffect* cFF = new cConstantForceFieldEffect(graphicsData.world, d, m);
      graphicsData.world->addEffect(cFF);
      controlData.worldEffects[cffInfo.effectName] = cFF;
      break;
    }

    case HAPTICS_VISCOSITY_FIELD:
    {
      cout << "Received HAPTICS_VISCOSITY_FIELD Message" << endl;
      M_HAPTICS_VISCOSITY_FIELD vF;
      memcpy(&vF, packet, sizeof(vF));
      cMatrix3d* B = new cMatrix3d(vF.viscosityMatrix[0], vF.viscosityMatrix[1], vF.viscosityMatrix[2],
                                   vF.viscosityMatrix[3], vF.viscosityMatrix[4], vF.viscosityMatrix[5],
                                   vF.viscosityMatrix[6], vF.viscosityMatrix[7], vF.viscosityMatrix[8]);
      cViscosityEffect* vFF = new cViscosityEffect(graphicsData.world, B);
      graphicsData.world->addEffect(vFF);
      controlData.worldEffects[vF.effectName] = vFF;
      break;
    }
    
    case HAPTICS_FREEZE_EFFECT:
    {
      cout << "Received HAPTICS_FREEZE_EFFECT Message" << endl;
      M_HAPTICS_FREEZE_EFFECT freeze;
      memcpy(&freeze, packet, sizeof(freeze));
      double workspaceScaleFactor = hapticsData.tool->getWorkspaceScaleFactor();
      double maxStiffness = 1.5*hapticsData.hapticDeviceInfo.m_maxLinearStiffness/workspaceScaleFactor;
      cVector3d currentPos = hapticsData.tool->getDeviceGlobalPos();
      cFreezeEffect* freezeEff = new cFreezeEffect(graphicsData.world, maxStiffness, currentPos);
      graphicsData.world->addEffect(freezeEff);
      controlData.worldEffects[freeze.effectName] = freezeEff;
      break;  
    }

    case HAPTICS_REMOVE_WORLD_EFFECT:
    {
      cout << "Received HAPTICS_REMOVE_FIELD_EFFECT Message" << endl;
      M_HAPTICS_REMOVE_WORLD_EFFECT rmField;
      memcpy(&rmField, packet, sizeof(rmField));
      cGenericEffect* fieldEffect = controlData.worldEffects[rmField.effectName];
      graphicsData.world->removeEffect(fieldEffect);
      controlData.worldEffects.erase(rmField.effectName);
      break;
    }

    case GRAPHICS_SET_ENABLED:
    {
      cout << "Received GRAPHICS_SET_ENABLED Message" << endl;
      M_GRAPHICS_SET_ENABLED graphicsEnabled;
      memcpy(&graphicsEnabled, packet, sizeof(graphicsEnabled));
      char* objectName;
      objectName = graphicsEnabled.objectName;
      int enabled = graphicsEnabled.enabled;
      if (controlData.objectMap.find(objectName) == controlData.objectMap.end()) {
        cout << objectName << " not found" << endl;
      }
      else {
        if (graphicsEnabled.enabled == 1) {
          controlData.objectMap[objectName]->setShowEnabled(true);
        }
        else if (graphicsEnabled.enabled == 0) {
          controlData.objectMap[objectName]->setShowEnabled(false);
        }
      }
      break;
    }
    
    case GRAPHICS_CHANGE_BG_COLOR:
    {
      cout << "Received GRAPHICS_CHANGE_BG_COLOR Message" << endl;
      M_GRAPHICS_CHANGE_BG_COLOR bgColor;
      memcpy(&bgColor, packet, sizeof(bgColor));
      float red = bgColor.color[0]/250.0;
      float green = bgColor.color[1]/250.0;
      float blue = bgColor.color[2]/250.0;
      graphicsData.world->setBackgroundColor(red, green, blue);
      break;
    }
    
    case GRAPHICS_PIPE:
    {
      cout << "Received GRAPHICS_PIPE Message" << endl;
      M_GRAPHICS_PIPE pipe;
      memcpy(&pipe, packet, sizeof(pipe));
      cVector3d* position = new cVector3d(pipe.position[0], pipe.position[1], pipe.position[2]);
      cMatrix3d* rotation = new cMatrix3d(pipe.rotation[0], pipe.rotation[1], pipe.rotation[2],
                                          pipe.rotation[3], pipe.rotation[4], pipe.rotation[5],
                                          pipe.rotation[6], pipe.rotation[7], pipe.rotation[8]);
      cColorf* color = new cColorf(pipe.color[0], pipe.color[1], pipe.color[2], pipe.color[3]);
      cPipe* myPipe = new cPipe(pipe.height, pipe.innerRadius, pipe.outerRadius, pipe.numSides, 
                                pipe.numHeightSegments, position, rotation, color);
      controlData.objectMap[pipe.objectName] = myPipe->getPipeObj();
      graphicsData.world->addChild(myPipe->getPipeObj());
      break;
    }

    case GRAPHICS_ARROW:
    {
      cout << "Received GRAPHICS_ARROW Message" << endl;
      M_GRAPHICS_ARROW arrow;
      memcpy(&arrow, packet, sizeof(arrow));
      cVector3d* direction = new cVector3d(arrow.direction[0], arrow.direction[1], arrow.direction[2]);
      cVector3d* position = new cVector3d(arrow.position[0], arrow.position[1], arrow.position[2]);
      cColorf* color = new cColorf(arrow.color[0], arrow.color[1], arrow.color[2], arrow.color[3]);
      cArrow* myArrow = new cArrow(arrow.aLength, arrow.shaftRadius, arrow.lengthTip, arrow.radiusTip,
                                    arrow.bidirectional, arrow.numSides, direction, position, color);
      controlData.objectMap[arrow.objectName] = myArrow->getArrowObj();
      graphicsData.world->addChild(myArrow->getArrowObj());
      break;
    }
    
    case GRAPHICS_CHANGE_OBJECT_COLOR:
    {
      cout << "Received GRAPHICS_CHANGE_OBJECT_COLOR Message" << endl;
      M_GRAPHICS_CHANGE_OBJECT_COLOR color;
      memcpy(&color, packet, sizeof(color));
      cGenericObject* obj = controlData.objectMap[color.objectName];
      obj->m_material->setColorf(color.color[0], color.color[1], color.color[2], color.color[3]);
      break;
    }
    case GRAPHICS_MOVING_DOTS:
    {
      cout << "Received GRAPHICS_MOVING_DOTS Message" << endl;
      cMultiPoint* test = new cMultiPoint();
      M_GRAPHICS_MOVING_DOTS dots;
      memcpy(&dots, packet, sizeof(dots));
      char* objectName;
      objectName = dots.objectName;
      cMovingDots* md = new cMovingDots(dots.numDots, dots.coherence, dots.direction, dots.magnitude);
      controlData.objectMap[objectName] = md;
      graphicsData.movingObjects.push_back(md);
      graphicsData.world->addChild(md->getMovingPoints());
      graphicsData.world->addChild(md->getRandomPoints());
      break;
    }
    case GRAPHICS_SHAPE_BOX:
    {
      cout << "Received GRAPHICS_SHAPE_BOX Message" << endl;
      M_GRAPHICS_SHAPE_BOX box;
      memcpy(&box, packet, sizeof(box));
      cShapeBox* boxObj = new cShapeBox(box.sizeX, box.sizeY, box.sizeZ);
      boxObj->setLocalPos(box.localPosition[0], box.localPosition[1], box.localPosition[2]);
      boxObj->m_material->setColorf(box.color[0], box.color[1], box.color[2], box.color[3]);
      controlData.objectMap[box.objectName] = boxObj;
      graphicsData.world->addChild(boxObj);
      break;
    }
    case GRAPHICS_SHAPE_SPHERE: 
    {
      cout << "Received GRAPHICS_SHAPE_SPHERE Message" << endl;
      M_GRAPHICS_SHAPE_SPHERE sphere;
      memcpy(&sphere, packet, sizeof(sphere));
      cShapeSphere* sphereObj = new cShapeSphere(sphere.radius);
      sphereObj->setLocalPos(sphere.localPosition[0], sphere.localPosition[1], sphere.localPosition[2]);
      sphereObj->m_material->setColorf(sphere.color[0], sphere.color[1], sphere.color[2], sphere.color[3]);
      controlData.objectMap[sphere.objectName] = sphereObj;
      graphicsData.world->addChild(sphereObj);
      break;
    }
    case GRAPHICS_SHAPE_TORUS:
    {
      cout << "Received GRAPHICS_SHAPE_TORUS Message" << endl;
      M_GRAPHICS_SHAPE_TORUS torus;
      memcpy(&torus, packet, sizeof(torus));
      cShapeTorus* torusObj = new cShapeTorus(torus.innerRadius, torus.outerRadius);
      graphicsData.world->addChild(torusObj);
      torusObj->setLocalPos(0.0, 0.0, 0.0);
      torusObj->m_material->setStiffness(1.0);
      torusObj->m_material->setColorf(255.0, 255.0, 255.0, 1.0);
      cEffectSurface* torusEffect = new cEffectSurface(torusObj);
      torusObj->addEffect(torusEffect);
      controlData.objectMap[torus.objectName] = torusObj;
      break; 
    }
  }
}
