#pragma once
#include "chai3d.h"

using namespace chai3d;
using namespace std;

/**
 * @file cImpulsePerturbation.h
 * @class cImpulsePerturbation
 *
 * @brief Constrains movement of the haptic device to a line between two points 
 *
 * Creates a line between two points that has a strong magnetic effect. Can be enabled or disabled.
 * As with other haptic effects, this class inherits from Chai3D's cGenericEFfect. 
 */
class cImpulsePerturbation : public cGenericEffect
{
  private:
    cVector3d* point;
    cVector3d* forceVec;
    cVector3d* normalVec;
    double duration;

    int state;

    #define STATE_ARMED 1
    #define STATE_ACTIVE 2
    #define STATE_FINISHED 3

    bool enabled;
    cPrecisionClock* perturbationClock;
  public:
    cImpulsePerturbation(cWorld* worldPtr, cVector3d* F, double d, 
            cVector3d* P, cVector3d* N);
    void setEnabled(bool e);
    bool getEnabled();
    int setPerturbationState(int s);
    int getPerturbationState();
    bool computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel, 
                      const unsigned int& a_toolID, cVector3d& a_reactionForce);
};
