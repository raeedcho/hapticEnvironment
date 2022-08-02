#include "streamer.h"

using namespace chai3d;
using namespace std;

/**
 * @file streamer.h
 * @file streamer.cpp
 * @brief Data streaming functionality
 *
 * Starts thread to start streaming position, velocity, and force data of the robot
 */

extern ControlData controlData;
extern HapticData hapticsData;


/**
 * Start the data streaming thread. The pointer to the thread is stored in the ControlData struct
 */
void startStreamerThread(void)
{
  controlData.streamerThread = new cThread();
  controlData.streamerThread->start(updateStreamer, CTHREAD_PRIORITY_HAPTICS);
  controlData.streamerUp = true;
}

/**
 * This function streams out the position, velocity, and force data of the robot.
 * It also calls the sendData() functions of all objects listed in controlData.stramingObjectsList
 */
void updateStreamer(void)
{
  cVector3d pos;
  cVector3d vel;
  cVector3d force;

  cPrecisionClock clock;
  clock.start();
  double currentTime, lastDataTransmitTime = -1;

  while (controlData.simulationRunning)
  {
    currentTime = clock.getCurrentTimeSeconds();
    if (currentTime-lastDataTransmitTime >= 0.001)
    {
      lastDataTransmitTime = clock.getCurrentTimeSeconds();

      ////////////////////////////////////////////////////////////////
      /////////////// transmit robot's states ////////////////////////
      ////////////////////////////////////////////////////////////////
      pos = hapticsData.tool->getDeviceGlobalPos(); //todo: change to local?
      vel = hapticsData.tool->getDeviceGlobalLinVel();
      force = hapticsData.tool->getDeviceGlobalForce();
      
      M_HAPTIC_DATA_STREAM toolData;
      memset(&toolData, 0, sizeof(toolData)); 
      toolData.header = createMesssageHeader(HAPTIC_DATA_STREAM);
      toolData.posX = pos.x();
      toolData.posY = pos.y();
      toolData.posZ = pos.z();
      toolData.velX = vel.x();
      toolData.velY = vel.y();
      toolData.velZ = vel.z();
      toolData.forceX = force.x();
      toolData.forceY = force.y();
      toolData.forceZ = force.z();
      char collisions[4][MAX_STRING_LENGTH];
      memset(&collisions, 0, sizeof(collisions));
      int collisionIdx = 0;
      unordered_map<string, cGenericObject*>::iterator objectItr;
      for (objectItr = controlData.objectMap.begin(); objectItr != controlData.objectMap.end(); objectItr++)
      {
        if (hapticsData.tool->isInContact(objectItr->second)) {
          int n = objectItr->first.length();
          char objectName[n+1];
          strcpy(objectName, objectItr->first.c_str());
          memcpy(&(collisions[collisionIdx]), objectName, sizeof(collisions[collisionIdx]));
          collisionIdx++;
        }
      }
      memcpy(&(toolData.collisions), collisions, sizeof(toolData.collisions));

      char packet[sizeof(toolData)];
      memcpy(&packet, &toolData, sizeof(toolData));
      vector<char> packetData(packet, packet+sizeof(packet)/sizeof(char));
      auto sendInt = controlData.client->async_call("sendMessage", packetData, sizeof(toolData), controlData.MODULE_NUM);
      if (controlData.loggingData == true)
      {
        controlData.dataFile.write((const char*) packet, sizeof(toolData));
      }
      sendInt.wait();


      ////////////////////////////////////////////////////////////////
      //////////// transmit states of the custom objects /////////////
      ////////////////////////////////////////////////////////////////
      streamObjectData(controlData.dataStreamersList);
    }
  }
  closeMessagingSocket();
  controlData.streamerUp = false;
}


/**
 * This function iterates through objects in the list and calls their sendData() function;
 */
void streamObjectData(vector<cGenericStreamerObject*> list)
{
    for(vector<cGenericStreamerObject*>::iterator itr = list.begin(); itr != list.end(); itr++){
      (*itr)->sendData();
    }
}