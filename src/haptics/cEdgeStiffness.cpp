#include "cEdgeStiffness.h"

/**
 * @param worldPtr Pointer to the world, since haptic effects are global.
 * @param width Width of the workspace
 * @param height Height of the workspace
 * @param depth Depth of the workspace
 * @param s Stifness
 * @param d Damping
 *
 * Constructor for a bounding boxes that constrain the workspace
 */
cEdgeStiffness::cEdgeStiffness(cWorld* worldPtr, double centerX, double centerY, double centerZ,
             double width, double height, double depth, double wallStiffness, double wallDamping):cGenericEffect(worldPtr)
{
    world = worldPtr;

    stiffness = wallStiffness;
    damping = wallDamping;

    O_x = centerX;
    O_y = centerY;
    O_z = centerZ;
    
    w = width/2;
    h = height/2;
    d = depth/2;

    double boxThickness = 0.1;
    double boxlength = 1;

    frontEdge = new cShapeBox(2*boxThickness, boxlength, boxlength);
    setEdgeProperties(frontEdge, d+boxThickness, 0, 0);
    world->addChild(frontEdge);

    backEdge = new cShapeBox(2*boxThickness, boxlength, boxlength);
    setEdgeProperties(backEdge, -d-boxThickness, 0, 0);
    world->addChild(backEdge);

    rightEdge = new cShapeBox(boxlength, 2*boxThickness, boxlength);
    setEdgeProperties(rightEdge, 0, w+boxThickness, 0);
    world->addChild(rightEdge);         

    leftEdge = new cShapeBox(boxlength, 2*boxThickness, boxlength);
    setEdgeProperties(leftEdge, 0, -w-boxThickness, 0);
    world->addChild(leftEdge);

    topEdge = new cShapeBox(boxlength, boxlength, 2*boxThickness);
    setEdgeProperties(topEdge, 0, 0, h+boxThickness);
    world->addChild(topEdge);  
    
    bottomEdge = new cShapeBox(boxlength, boxlength, 2*boxThickness);
    setEdgeProperties(bottomEdge, 0, 0, -h-boxThickness);
    world->addChild(bottomEdge);        
}

/**
 * @param e True enables this effect, false deactivates it
 *
 * Set whether the haptic tool is being constrained to a rectangular space
 */
void cEdgeStiffness::setEnabled(bool e)
{
  enabled = e;
  leftEdge->setHapticEnabled(enabled);
  rightEdge->setHapticEnabled(enabled);
  topEdge->setHapticEnabled(enabled);
  bottomEdge->setHapticEnabled(enabled);
  frontEdge->setHapticEnabled(enabled);
  backEdge->setHapticEnabled(enabled);
}

/**
 * Determine whether the constraint is enabled. True if it is, false otherwise
 */
bool cEdgeStiffness::getEnabled()
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
bool cEdgeStiffness::computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel, 
                      const unsigned int& a_toolID, cVector3d& a_reactionForce)
{
    // A dummy function to be compatible with cGenericEffect template. 
    // This "effect" does not add extra reaction force. Only bounding boxes create force.
    a_reactionForce.zero();
    return true;
}


void cEdgeStiffness::setEdgeProperties(cShapeBox* boxPrt, double posX, double posY, double posZ)
{
    boxPrt->setLocalPos(posX+O_x, posY+O_y, posZ+O_z);
    boxPrt->setStiffness(stiffness);
    // boxPrt->m_material->setViscosity(damping); //feels very sticky with damping
    // boxPrt->createEffectViscosity();
    boxPrt->createEffectSurface();
    boxPrt->setEnabled(true);
    boxPrt->setShowEnabled(false);
    // boxPrt->setTransparencyLevel(0.1);
}