#pragma once
#include "chai3d.h"

using namespace chai3d;
using namespace std;


/**
 * @file cGenericStreamerObject.h
 *
 * @class cGenericStreamerObject
 *
 * @brief A generic abstract class for any object that needs to stream the data out to other modules.
 * 
 * All objects that need to stream data out inherit this class and override the sendData() function.
 * This function is called in the streamer update loop for each object. 
 */
class cGenericStreamerObject
{
  public:
    cGenericStreamerObject();
    ~cGenericStreamerObject();
    virtual void sendData() {};
};

