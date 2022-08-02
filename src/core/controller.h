#pragma once 

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_  

#include "../../external/chai3d-3.2.0/src/chai3d.h"
#include "GLFW/glfw3.h"
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>
#include "network/network.h"
#include "network/streamer.h"
#include "network/listener.h"
#include "haptics/haptics.h"
#include "graphics/graphics.h"
#include "combined/combined.h"
#include "network/cGenericStreamerObject.h"
#include <fstream>
#include "rpc/client.h"


using namespace chai3d;
using namespace std;

struct ControlData
{
  // State variables
  bool simulationRunning;
  bool simulationFinished;
  bool hapticsUp;
  bool listenerUp;
  bool streamerUp;
  bool loggingData;
  cWorld* world;

  // Messaging and Data Logging Variables
  int MODULE_NUM;
  const char* IPADDR;
  int PORT;
  int msg_socket;
  const char* MH_IP;
  int MH_PORT;
  rpc::client* client;
  
  cThread* streamerThread; // for streaming haptic data only
  cThread* listenerThread; // for listening to messageHandler messages
  ofstream dataFile;

  //a map to keep a track of all objects
  unordered_map<string, cGenericObject*> objectMap;
  //a list to track objects that need to transmit data. Include an object (e.g. a cuptask object) in this list if it needs to send out data
  vector<cGenericStreamerObject*> dataStreamersList;
};

bool allThreadsDown(void);
void close(void);
void parsePacket(char* packet);
#endif
