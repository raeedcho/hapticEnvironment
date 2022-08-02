#include "cConstrainToLine.h"

/**
 * @param worldPtr Pointer to the world, since haptic effects are global.
 * @param p1 First endpoint of line 
 * @param p2 Second endpoint of line 
 * @param s Stifness
 * @param d Damping
 *
 * Constructor for a magnetic line to which the haptic tool is pulled
 */
cConstrainToLine::cConstrainToLine(cWorld* worldPtr, cVector3d* p1, cVector3d* p2, double s, double d):cGenericEffect(worldPtr)
{
  point1 = p1;
  point2 = p2;
  stiffness = s;
  damping = d;
}

/**
 * @param e True enables this effect, false deactivates it
 *
 * Set whether the haptic tool is being constrained to a line
 */
void cConstrainToLine::setEnabled(bool e)
{
  enabled = e;
}

/**
 * Determine whether the line constraint is enabled. True if it is, false otherwise
 */
bool cConstrainToLine::getEnabled()
{
  return enabled;
}

/**
 * @param a_toolPos Position of the haptic tool 
 * @param a_toolVel Velocity of the haptic tool 
 * @param a_toolID ID number of the haptic tool 
 * @param a_reactionForce Vector that stores force to be applied to haptic tool.
 *
 * This function is inherited from cGenericEffect and is called on each iteration of the haptic
 * update thread. The haptic tool is constrained to a line by computing the projection between the
 * tool and the line and applying a force that is normal from the point to the line. Essentially,
 * the haptic tool is connected to the line by a spring along the projection from the tool to the
 * line.
 */
bool cConstrainToLine::computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel, 
                      const unsigned int& a_toolID, cVector3d& a_reactionForce)
{
  if (enabled == true) {

    cVector3d lineDirection = cSub(*point2, *point1);
    lineDirection = cNormalize(lineDirection);
    cVector3d projectedPont = cProjectPointOnLine(a_toolPos, *point1, lineDirection); //distance to line 
    a_reactionForce = cMul(stiffness, cSub(projectedPont, a_toolPos));

    cVector3d normalVel = cProjectPointOnPlane(a_toolVel,cVector3d(0,0,0),lineDirection);
    cVector3d dampingForce = cMul(-damping, normalVel);

    a_reactionForce.add(dampingForce);

    return true;
  }
  else {
    a_reactionForce.zero();
    return true;
  }
}
