#include "cImpulsePerturbation.h"

/**
 * @param worldPtr Pointer to the world, since haptic effects are global.
 * @param F Impulsive force vector
 * @param d Duration of the impulse
 * @param p A point on the edge of the half-space
 * @param N Normal vector defining the orientation of the half-space
 *
 * Constructor for an impulsive perturbation that happnes only once when the tool enters
 * in a half-space defined by a normal vector and a line
 */
cImpulsePerturbation::cImpulsePerturbation(cWorld* worldPtr, cVector3d* F, double d, 
            cVector3d* P, cVector3d* N):cGenericEffect(worldPtr)
{
    point = P;
    normalVec = N;
    forceVec = F;
    duration = d;
    perturbationClock = new cPrecisionClock();
    setEnabled(true);
    state = STATE_ARMED;
}

/**
 * @param e True enables this effect, false deactivates it
 *
 * Set whether the haptic tool is being constrained to a line
 */
void cImpulsePerturbation::setEnabled(bool e)
{
    enabled = e;
}

/**
 * Determine whether the line constraint is enabled. True if it is, false otherwise
 */
bool cImpulsePerturbation::getEnabled()
{
    return enabled;
}

/**
 * Sets the state of the perturbation: 1: armed. 2: active. 3: finished.
 */
int cImpulsePerturbation::setPerturbationState(int s)
{   if (s==STATE_ARMED || s==STATE_ACTIVE || s==STATE_FINISHED)
        state = s;
    return state;
}

/**
 * Returns the state of the perturbation: 1: armed. 2: active. 3: finished.
 */
int cImpulsePerturbation::getPerturbationState()
{
    return state;
}

/**
 * @param a_toolPos Position of the haptic tool 
 * @param a_toolVel Velocity of the haptic tool 
 * @param a_toolID ID number of the haptic tool 
 * @param a_reactionForce Vector that stores force to be applied to haptic tool.
 *
 * This function is inherited from cGenericEffect and is called on each iteration of the haptic
 * update thread. The haptic tool experiences an impulse (duration*forceVec) for the first time
 * it enters the half-space that is defined by a point and a normal vector.
 */
bool cImpulsePerturbation::computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel, 
                      const unsigned int& a_toolID, cVector3d& a_reactionForce)
{
    a_reactionForce.zero();
    if (enabled == true) {
        switch (state){
            case STATE_ARMED: {
                cVector3d V = cSub(a_toolPos, *point);
                double distance = cDot(V,*normalVec);
                if (distance<=0) { 
                    state = STATE_ACTIVE;
                    perturbationClock->start();
                    perturbationClock->reset();
                }
                break;
            }
            case STATE_ACTIVE: {
                a_reactionForce.add(*forceVec);
                if (perturbationClock->getCurrentTimeSeconds() >= duration){
                    perturbationClock->stop();
                    state = STATE_FINISHED;
                }
                break;
            }
            case STATE_FINISHED:
                break;

            default:
                break;
        }
    }
    return true;
}
