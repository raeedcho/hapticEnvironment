#pragma once 

#ifndef _SENDER_H_
#define _SENDER_H_ 

#include <stdlib.h>
#include "chai3d.h"
#include <vector>

#include "haptics/haptics.h"
#include "network.h"
#include "cGenericStreamerObject.h"

void startStreamerThread(void);
//void closeStreamer(void);
void updateStreamer(void);

void streamObjectData(vector<cGenericStreamerObject*> list);
#endif
