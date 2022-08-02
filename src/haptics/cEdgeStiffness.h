#pragma once
#include "chai3d.h"

using namespace chai3d;
using namespace std;

/**
 * @file cEdgeStiffness.h
 * @class cEdgeStiffness
 *
 * @brief Constrains movement of the haptic device to a line between two points 
 *
 * Creates a line between two points that has a strong magnetic effect. Can be enabled or disabled.
 * As with other haptic effects, this class inherits from Chai3D's cGenericEFfect. 
 */
class cEdgeStiffness : public cGenericEffect
{
  private:
    double O_x,O_y,O_z;
    double w,h,d;
    bool enabled;
    double stiffness; 
    double damping;
    cWorld* world;
    cShapeBox* frontEdge;
    cShapeBox* backEdge;
    cShapeBox* leftEdge;
    cShapeBox* rightEdge;
    cShapeBox* topEdge;
    cShapeBox* bottomEdge;
  public:
    cEdgeStiffness(cWorld* worldPtr, double centerX, double centerY, double centerZ,
                    double width, double height, double depth, double wallStiffness, double wallDamping);
    void setEnabled(bool e);
    bool getEnabled();
    bool computeForce(const cVector3d& a_toolPos, const cVector3d& a_toolVel, 
                      const unsigned int& a_toolID, cVector3d& a_reactionForce);
    void setEdgeProperties(cShapeBox* boxPrt, double posX, double posY, double posZ);
};
