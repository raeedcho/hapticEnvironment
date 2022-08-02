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


extern HapticData hapticsData; // HaptidData is defined in haptics.h. Keeps track of objects that have haptic (force) effects
extern GraphicsData graphicsData; // GraphicsData is defined in graphics.h. Keeps track of objects that have special viual effects
ControlData controlData; // ControlData is defined in controller.h. Keeps track of high-level variables


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
  
  atexit(close); 


  // ControlData is a structure containing high-level information about the flow of the program
  controlData.simulationRunning = false;
  controlData.simulationFinished = true;
  controlData.hapticsUp = false;
  controlData.listenerUp = false;
  controlData.streamerUp = false;
  controlData.loggingData = false;


  // TODO: Set these IP addresses from a config file
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

  // This program is a "client" of messageHandler. All communications with other programs is via MH. 
  // This clinet sends "remote prodecure call" requests to the server (MH) to request timestamp or broadcast a message.
  controlData.client = new rpc::client(controlData.MH_IP, controlData.MH_PORT);
  

  // "World" is where all action take place. All objects and haptic effects are added to this world.
  controlData.world = new cWorld(); 


  initDisplay(); // Initializes glfw window. construct the global variable graphicsData. 
  initScene(); // Initializes the camera, lights. puts them in graphicsData

  initHaptics(); // Initializes the haptic device. Contructs the global variable hapticsData. 
  startHapticsThread(); // Creates a new thread to run the haptics loop. Haptic simulation starts here
  
  cSleepMs(200);


  openMessagingSocket(); // Opens a UDP socket to listen to messages broadcasted by messageHandler
  int addSuccess = addMessageHandlerModule(); // Adds this program as module#1 to the messageHandler
  if (addSuccess == 0) {
    cout << "Module addition failed" << endl;
    close();
    exit(1);
  }
  cSleepMs(100);
  int subscribeSuccess = subscribeToTrialControl(); // Subscribing to the "trialController", so that the messages broadcasted by trialController are sent to module#1 as well
  if (subscribeSuccess == 0) {
    cout << "Subcribe to Trial Control failed" << endl;
    close();
    exit(1);
  }

  // To subscribe to my own message requests so when I broadcast a message I receive a copy of it back. For quick development purposes only. These line can be deleted.
  // To be deleted by Raeed.
  bool subscribed = false;
  while (subscribed == false) {
    auto subscribe = controlData.client->async_call("subscribeTo", controlData.MODULE_NUM, controlData.MODULE_NUM);
    subscribe.wait();
    if (subscribe.get().as<int>() == 1) {
      subscribed = true;
      cout << "subscribed to self" << endl;
    }
  }
      
  cSleepMs(100);

  startStreamerThread(); // Starts a new thread to stream out robot data. messages are broadcased via messageHandler to whoever wants to listen
  startListenerThread(); // Starts a new thread to listen to the incoming messages sent by the messageHandler
  cout << "streamer and listener started" << endl;

  startGraphicsLoop(); //Starts the "main thread": a loop that updates graphics. code stucks here until the end of program

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
  controlData.simulationRunning = false; // All threads haptic/listener/streamer/graphics rely on this
  while (!controlData.simulationFinished) {
    controlData.simulationFinished = allThreadsDown();
    cSleepMs(100);
  }
  hapticsData.tool->stop();
  cout << "Haptic tool stopped" << endl;
  delete hapticsData.hapticsThread;
  cout << "Deleted haptics thread" << endl;
  delete controlData.world;
  cout << "Deleted world" << endl;
  delete hapticsData.handler;
  cout << "Deleted handler" << endl;
  closeMessagingSocket();
  controlData.world->deleteAllChildren();
  
  glfwDestroyWindow(graphicsData.window); 
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
  // cout<<msgType<< ", time stamp: " << fixed << header.timestamp << endl;;
  switch (msgType)
  {
    /////////////////////////////////////////////////
    ///////////// EXPERIMENT CONTROL ////////////////
    /////////////////////////////////////////////////
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
        controlData.world->deleteChild(objPtr);
        controlData.objectMap.erase(rmObj.objectName);
      }
      break;
    }
    
    case RESET_WORLD:
    {
      cout << "Received RESET_WORLD Message" << endl;
      unordered_map<string, cGenericObject*>::iterator objIt = controlData.objectMap.begin();
      while (objIt != controlData.objectMap.end()) {
        bool removedObj = controlData.world->deleteChild(objIt->second);
        objIt++;
      }
      unordered_map<string, cGenericEffect*>::iterator effIt = hapticsData.hapticEffectsMap.begin();
      while (effIt != hapticsData.hapticEffectsMap.end()) {
        bool removedEffect = controlData.world->removeEffect(effIt->second);
        effIt++;
      }
      controlData.objectMap.clear();
      // controlData.objectEffects.clear();
      hapticsData.hapticEffectsMap.clear();
      break;
    }


    /////////////////////////////////////////////////
    //////////////// CST  MESSAGES //////////////////
    /////////////////////////////////////////////////
    case CST_CREATE:
    {
      cout << "Received CST_CREATE Message" << endl;
      M_CST_CREATE cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      char* cstName = cstObj.cstName;
      if (controlData.objectMap.find(cstName) != controlData.objectMap.end()) {
        cout << "Task with name '" << cstName << "' already exists. Give the task a unique name." << endl;
        cout << "list of exsting objects: "; 
        for(auto it:controlData.objectMap) cout<<it.first<< ", ";
        cout<<endl;
      }
      else {
        cCST* cst = new cCST(controlData.world, cstObj.lambdaVal, 
            cstObj.forceMagnitude, cstObj.visionEnabled, cstObj.hapticEnabled);
        controlData.objectMap[cstName] = cst;
        graphicsData.visualEffectsList.push_back(cst);
        controlData.world->addEffect(cst);
        hapticsData.hapticEffectsMap[cstName] = cst;
        controlData.dataStreamersList.push_back(cst);
      }
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
        graphicsData.visualEffectsList.erase(find(graphicsData.visualEffectsList.begin(), graphicsData.visualEffectsList.end(), cst));
        controlData.dataStreamersList.erase(find(controlData.dataStreamersList.begin(), controlData.dataStreamersList.end(), cst));
        hapticsData.hapticEffectsMap.erase(cstObj.cstName);
        controlData.objectMap.erase(cstObj.cstName);
        controlData.world->removeEffect(cst);

      }
      break;
    }
    case CST_START:
    {
      cout << "Received CST_START Message" << endl;
      M_CST_START cstObj;
      memcpy(&cstObj, packet, sizeof(cstObj));
      cCST* cst = dynamic_cast<cCST*>(controlData.objectMap[cstObj.cstName]);
      hapticsData.tool->setShowEnabled(false); 
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
      hapticsData.tool->setShowEnabled(true); 
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

    /////////////////////////////////////////////////
    ///////////// CUP TASK MESSAGES /////////////////
    /////////////////////////////////////////////////
    case CUPTASK_CREATE:
    {
      cout << "Received CUPTASK_CREATE Message" << endl;
      M_CUPTASK_CREATE msg;
      memcpy(&msg, packet, sizeof(msg));
      char* cupTaskName = msg.cupTaskName;
      if (controlData.objectMap.find(cupTaskName) != controlData.objectMap.end()) {
        cout << "Task with name '" << cupTaskName << "' already exists. Give the task a unique name." << endl;
        cout << "list of exsting objects: "; 
        for(auto it:controlData.objectMap) cout<<it.first<< ", ";
        cout<<endl;        
      }
      else {
        cCupAndBall* cnb = new cCupAndBall(controlData.world, msg.escapeAngle, 
            msg.pendulumLength, msg.ballMass, msg.cartMass,
            msg.ballDamping, msg.cupDamping);
        controlData.objectMap[cupTaskName] = cnb;
        controlData.dataStreamersList.push_back(cnb);
        graphicsData.visualEffectsList.push_back(cnb);
        controlData.world->addEffect(cnb);
        hapticsData.hapticEffectsMap[cupTaskName] = cnb;
        hapticsData.tool->setShowEnabled(false);
      }
      break;
    }

    case CUPTASK_DESTRUCT:
    {
      cout << "Received CUPTASK_DESTRUCT Message" << endl;
      M_CUPTASK_DESTRUCT msg;
      memcpy(&msg, packet, sizeof(msg));
      if (controlData.objectMap.find(msg.cupTaskName) == controlData.objectMap.end()) {
        cout << msg.cupTaskName << " not found" << endl;
      }
      else {
        cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
        cnb->stopCupAndBall();
        cnb->destructCupAndBall();
        graphicsData.visualEffectsList.erase(find(graphicsData.visualEffectsList.begin(), graphicsData.visualEffectsList.end(), cnb));
        controlData.dataStreamersList.erase(find(controlData.dataStreamersList.begin(), controlData.dataStreamersList.end(), cnb));
        hapticsData.hapticEffectsMap.erase(msg.cupTaskName);
        controlData.objectMap.erase(msg.cupTaskName);
        controlData.world->removeEffect(cnb);
        hapticsData.tool->setShowEnabled(true);
      }
      break;           
    }

    case CUPTASK_START:
    {
      cout << "Received CUPTASK_START Message" << endl;
      M_CUPTASK_START msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->startCupAndBall();
      break;
    }
    case CUPTASK_STOP:
    {
      cout << "Received CUPTASK_STOP Message" << endl;
      M_CUPTASK_STOP msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->stopCupAndBall();
      break;
    }

    case CUPTASK_RESET:
    {
      cout << "Received CUPTASK_RESET Message" << endl;
      M_CUPTASK_RESET msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->resetCupAndBall();
      break;
    }

    case CUPTASK_SET_BALL_VISUALS:
    {
      cout << "Received CUPTASK_SET_BALL_VISUALS Message" << endl;
      M_CUPTASK_SET_BALL_VISUALS msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->setEnableBallVisuals(!cnb->getEnableBallVisuals());
      break;
    }

    case CUPTASK_SET_BALL_HAPTICS:
    {
      cout << "Received CUPTASK_SET_BALL_HAPTICS Message" << endl;
      M_CUPTASK_SET_BALL_HAPTICS msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->setEnableBallHaptics(!cnb->getEnableBallHaptics());
      break;
    }

    case CUPTASK_SET_CUP_VISUALS:
    {
      cout << "Received CUPTASK_SET_CUP_VISUALS Message" << endl;
      M_CUPTASK_SET_CUP_VISUALS msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->setEnableCupVisuals(!cnb->getEnableCupVisuals());
      break;
    }

    case CUPTASK_SET_PATH_CONSTRAINT:
    {
      cout << "Received CUPTASK_SET_PATH_CONSTRAINT Message" << endl;
      M_CUPTASK_SET_PATH_CONSTRAINT msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->setEnablePathConstraint(!cnb->getEnablePathConstraint());
      break;
    }

    case CUPTASK_SET_COLOR_BALL:
    {
      cout << "Received CUPTASK_SET_COLOR_BALL Message" << endl;
      M_CUPTASK_SET_COLOR_BALL msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->setColorBall(msg.color[0], msg.color[1], msg.color[2], msg.color[3]);
      break;
    }

    case CUPTASK_SET_COLOR_CUP:
    {
      cout << "Received CUPTASK_SET_COLOR_CUP Message" << endl;
      M_CUPTASK_SET_COLOR_CUP msg;
      memcpy(&msg, packet, sizeof(msg));
      cCupAndBall* cnb = dynamic_cast<cCupAndBall*>(controlData.objectMap[msg.cupTaskName]);
      cnb->setColorCup(msg.color[0], msg.color[1], msg.color[2], msg.color[3]);
      break;
    }

    /////////////////////////////////////////////////
    ////////////// GENERIC HAPTIC EFFECTS ///////////
    /////////////////////////////////////////////////
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
      cGenericEffect* fieldEffect = hapticsData.hapticEffectsMap[effectName];
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
      if (controlData.objectMap.find(bpMsg.effectName) != controlData.objectMap.end()) {
        cout << "Effect with name '" << bpMsg.effectName << "' already exists. Give it a unique name." << endl;
        cout << "list of exsting effects: "; 
        for(auto it:controlData.objectMap) cout<<it.first<< ", ";
        cout<<endl;        
      }
      else{  
        double bWidth = bpMsg.bWidth;
        double bHeight = bpMsg.bHeight;
        int stiffness = hapticsData.hapticDeviceInfo.m_maxLinearStiffness;
        double toolRadius = hapticsData.toolRadius;
        cBoundingPlane* bp = new cBoundingPlane(stiffness, toolRadius, bWidth, bHeight);
        controlData.world->addChild(bp->getLowerBoundingPlane());
        controlData.world->addChild(bp->getUpperBoundingPlane());
        controlData.world->addChild(bp->getTopBoundingPlane());
        controlData.world->addChild(bp->getBottomBoundingPlane());
        controlData.world->addChild(bp->getLeftBoundingPlane());
        controlData.world->addChild(bp->getRightBoundingPlane());
        controlData.objectMap[bpMsg.effectName] = bp;
      }
      break;
    }
    
    case HAPTICS_CONSTANT_FORCE_FIELD:
    {
      cout << "Received HAPTICS_CONSTANT_FORCE_FIELD Message" << endl;
      M_HAPTICS_CONSTANT_FORCE_FIELD cffInfo;
      memcpy(&cffInfo, packet, sizeof(cffInfo));
      if (hapticsData.hapticEffectsMap.find(cffInfo.effectName) != hapticsData.hapticEffectsMap.end()) {
        cout << "Effect with name '" << cffInfo.effectName << "' already exists. Give it a unique name." << endl;
        cout << "list of exsting effects: "; 
        for(auto it:hapticsData.hapticEffectsMap) cout<<it.first<< ", ";
        cout<<endl;        
      }
      else{  
        double d = cffInfo.direction;
        double m = cffInfo.magnitude;
        cConstantForceFieldEffect* cFF = new cConstantForceFieldEffect(controlData.world, d, m);
        controlData.world->addEffect(cFF);
        hapticsData.hapticEffectsMap[cffInfo.effectName] = cFF;
      }
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
      cViscosityEffect* vFF = new cViscosityEffect(controlData.world, B);
      controlData.world->addEffect(vFF);
      hapticsData.hapticEffectsMap[vF.effectName] = vFF;
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
      cFreezeEffect* freezeEff = new cFreezeEffect(controlData.world, maxStiffness, currentPos);
      controlData.world->addEffect(freezeEff);
      hapticsData.hapticEffectsMap[freeze.effectName] = freezeEff;
      break;  
    }

    case HAPTICS_LINE_CONSTRAINT:
    {
      cout << "Received HAPTICS_LINE_CONSTRAINT Message" << endl;
      M_HAPTICS_LINE_CONSTRAINT msg;
      memcpy(&msg, packet, sizeof(msg));
      if (hapticsData.hapticEffectsMap.find(msg.effectName) != hapticsData.hapticEffectsMap.end()) {
        cout << "Effect with name '" << msg.effectName << "' already exists. Give it a unique name." << endl;
        cout << "list of exsting effects: "; 
        for(auto it:hapticsData.hapticEffectsMap) cout<<it.first<< ", ";
        cout<<endl;        
      }
      else{        
        cVector3d* point1 = new cVector3d(msg.point1[0],msg.point1[1],msg.point1[2]);
        cVector3d* point2 = new cVector3d(msg.point2[0],msg.point2[1],msg.point2[2]);
        cConstrainToLine* lineEffect = new cConstrainToLine(controlData.world, point1, point2, msg.stifness, msg.damping);
        lineEffect->setEnabled(true); 
        controlData.world->addEffect(lineEffect);
        hapticsData.hapticEffectsMap[msg.effectName] = lineEffect;
      }
      break;  
    }

    case HAPTICS_EDGE_STIFFNESS:
    {
      cout << "Received HAPTICS_EDGE_STIFFNESS Message" << endl;
      M_HAPTICS_EDGE_STIFFNESS msg;
      memcpy(&msg, packet, sizeof(msg));
      if (hapticsData.hapticEffectsMap.find(msg.effectName) != hapticsData.hapticEffectsMap.end()) {
        cout << "Effect with name '" << msg.effectName << "' already exists. Give it a unique name." << endl;
        cout << "list of exsting effects: "; 
        for(auto it:hapticsData.hapticEffectsMap) cout<<it.first<< ", ";
        cout<<endl;        
      }
      else{        
        cEdgeStiffness* edgeStiffness = new cEdgeStiffness(controlData.world, msg.center[0],msg.center[1], msg.center[2], 
                      msg.width, msg.height, msg.depth, msg.stifness, msg.damping);
        edgeStiffness->setEnabled(true); 
        hapticsData.hapticEffectsMap[msg.effectName] = edgeStiffness;
      }
      break;  
    }    

    case HAPTICS_IMPULSE_PERT:
    {
      cout << "Received HAPTICS_IMPULSE_PERT Message" << endl;
      M_HAPTICS_IMPULSE_PERT msg;
      memcpy(&msg, packet, sizeof(msg));
      if (hapticsData.hapticEffectsMap.find(msg.effectName) != hapticsData.hapticEffectsMap.end()) {
        cout << "Effect with name '" << msg.effectName << "' already exists. Give it a unique name." << endl;
        cout << "list of exsting effects: "; 
        for(auto it:hapticsData.hapticEffectsMap) cout<<it.first<< ", ";
        cout<<endl;        
      }
      else{        
        cVector3d* F = new cVector3d(msg.forceVector[0],msg.forceVector[1], msg.forceVector[2]);
        double d = msg.impulseDuration;
        cVector3d* N = new cVector3d(msg.normalToPlane[0],msg.normalToPlane[1], msg.normalToPlane[2]);
        cVector3d* P = new cVector3d(msg.pointOnPlane[0],msg.pointOnPlane[1], msg.pointOnPlane[2]);
        cImpulsePerturbation* pert = new cImpulsePerturbation(controlData.world, F, d, P, N);
        pert->setEnabled(true); 
        controlData.world->addEffect(pert);
        hapticsData.hapticEffectsMap[msg.effectName] = pert;
      }
      break;  
    } 

    case HAPTICS_REMOVE_WORLD_EFFECT:
    {
      cout << "Received HAPTICS_REMOVE_FIELD_EFFECT Message" << endl;
      M_HAPTICS_REMOVE_WORLD_EFFECT rmField;
      memcpy(&rmField, packet, sizeof(rmField));
      cGenericEffect* fieldEffect = hapticsData.hapticEffectsMap[rmField.effectName];
      controlData.world->removeEffect(fieldEffect);
      hapticsData.hapticEffectsMap.erase(rmField.effectName);
      break;
    }


    /////////////////////////////////////////////////
    ////////////// GENERIC VISUAL EFFECTS ///////////
    /////////////////////////////////////////////////
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
      controlData.world->setBackgroundColor(red, green, blue);
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
      controlData.world->addChild(myPipe->getPipeObj());
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
      controlData.world->addChild(myArrow->getArrowObj());
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

    case GRAPHICS_SET_OBJECT_LOCALPOS:
    {
      cout << "Received GRAPHICS_SET_OBJECT_LOCALPOS Message" << endl;
      M_GRAPHICS_SET_OBJECT_LOCALPOS msg;
      memcpy(&msg, packet, sizeof(msg));
      cGenericObject* obj = controlData.objectMap[msg.objectName];
      obj->setLocalPos(msg.pos[0], msg.pos[1], msg.pos[2]);
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
      graphicsData.visualEffectsList.push_back(md);
      controlData.world->addChild(md->getMovingPoints());
      controlData.world->addChild(md->getRandomPoints());
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
      controlData.world->addChild(boxObj);
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
      controlData.world->addChild(sphereObj);
      break;
    }
    case GRAPHICS_SHAPE_TORUS:
    {
      cout << "Received GRAPHICS_SHAPE_TORUS Message" << endl;
      M_GRAPHICS_SHAPE_TORUS torus;
      memcpy(&torus, packet, sizeof(torus));
      cShapeTorus* torusObj = new cShapeTorus(torus.innerRadius, torus.outerRadius);
      controlData.world->addChild(torusObj);
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
