#define DEFAULT_IP "localhost:10000"
#define MAX_PACKET_LENGTH 8192 // arbitrary 
#define MAX_STRING_LENGTH 128  // also arbitrary

// Test Packet 
#define TEST_PACKET 9000

// Experiment Control Messages 1-500 
// Sometimes 0 is poorly parsed as a truncation signal
#define SESSION_START 1
#define SESSION_END 2
#define TRIAL_START 3
#define TRIAL_END 4
#define START_RECORDING 5
#define STOP_RECORDING 6
#define REMOVE_OBJECT 7
#define KEYPRESS 8
#define PAUSE_RECORDING 9
#define RESUME_RECORDING 10
#define RESET_WORLD 11
#define REWARD 12

// Combined/Complex Object Messages 500-1000
#define CST_CREATE 500
#define CST_DESTRUCT 501 
#define CST_START 502
#define CST_STOP 503
#define CST_SET_VISUAL 504
#define CST_SET_HAPTIC 505
#define CST_SET_LAMBDA 506
#define CST_DATA 507

#define CUPTASK_CREATE 600
#define CUPTASK_DESTRUCT 601
#define CUPTASK_START 602
#define CUPTASK_STOP 603
#define CUPTASK_DATA 604 
#define CUPTASK_RESET 605
#define CUPTASK_SET_BALL_VISUALS 606
#define CUPTASK_SET_BALL_HAPTICS 607
#define CUPTASK_SET_CUP_VISUALS 608
#define CUPTASK_SET_PATH_CONSTRAINT 609
#define CUPTASK_SET_COLOR_BALL 610
#define CUPTASK_SET_COLOR_CUP 611

// Haptics Messages 1000-2000
#define HAPTIC_DATA_STREAM 1000
#define HAPTICS_SET_ENABLED 1001
#define HAPTICS_SET_ENABLED_WORLD 1002
#define HAPTICS_SET_STIFFNESS 1008
#define HAPTICS_BOUNDING_PLANE 1009 
#define HAPTICS_CONSTANT_FORCE_FIELD 1010
#define HAPTICS_VISCOSITY_FIELD 1011
#define HAPTICS_FREEZE_EFFECT 1012
#define HAPTICS_LINE_CONSTRAINT 1013
#define HAPTICS_EDGE_STIFFNESS 1014
#define HAPTICS_IMPULSE_PERT 1015
#define HAPTICS_REMOVE_WORLD_EFFECT 1016

// Graphics Messages are 2000-3000 
#define GRAPHICS_SET_ENABLED 2000
#define GRAPHICS_CHANGE_BG_COLOR 2001
#define GRAPHICS_PIPE 2002
#define GRAPHICS_ARROW 2003
#define GRAPHICS_CHANGE_OBJECT_COLOR 2004
#define GRAPHICS_SET_OBJECT_LOCALPOS 2005
#define GRAPHICS_MOVING_DOTS 2014
#define GRAPHICS_SHAPE_BOX 2046
#define GRAPHICS_SHAPE_SPHERE 2050
#define GRAPHICS_SHAPE_TORUS 2051

/**
 * MSG_HEADER is included in all messages that are sent. It contains metadata about the time and
 * type of message
 */
typedef struct {
  int serial_no; /**< Serial Number of message, received from MessageHandler.*/
  int msg_type; /**< Type of message should correspond to one of the integers listed in messageDefinitions.h.*/
  double reserved; /**< Reserved for now */
  double timestamp; /**< Time MessageHandler made the message.*/ 
} MSG_HEADER;

/**
 * M_TEST_PACKET is used for testing to ensure that message sending is working.
 */
typedef struct {
  MSG_HEADER header; /**< Standard message header */
  int a; /**< First test value */
  int b; /**< Second test value */
} M_TEST_PACKET;



/////////////////////////////////////////////////
///////////// EXPERIMENT CONTROL ////////////////
/////////////////////////////////////////////////
typedef struct {
  MSG_HEADER header;
} M_SESSION_START;

typedef struct {
  MSG_HEADER header;
} M_SESSION_END;

typedef struct {
  MSG_HEADER header;
  int trialNum;
} M_TRIAL_START;

typedef struct {
  MSG_HEADER header;
} M_TRIAL_END;

typedef struct {
  MSG_HEADER header;
  char filename[MAX_STRING_LENGTH];
} M_START_RECORDING;

typedef struct {
  MSG_HEADER header;
} M_STOP_RECORDING;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
} M_REMOVE_OBJECT;

typedef struct {
  MSG_HEADER header;
  char keyname[MAX_STRING_LENGTH];
} M_KEYPRESS;

typedef struct {
  MSG_HEADER header;
} M_PAUSE_RECORDING;

typedef struct {
  MSG_HEADER header;
} M_RESUME_RECORDING;

typedef struct {
  MSG_HEADER header;
} M_RESET_WORLD;

typedef struct {
  MSG_HEADER header;
  bool release;
} M_REWARD;

/////////////////////////////////////////////////
//////////////// CST  MESSAGES //////////////////
/////////////////////////////////////////////////
typedef struct {
  MSG_HEADER header;
  char cstName[MAX_STRING_LENGTH];
  double lambdaVal;
  double forceMagnitude;
  int visionEnabled;
  int hapticEnabled;
} M_CST_CREATE;

typedef struct {
  MSG_HEADER header;
  char cstName[MAX_STRING_LENGTH];
} M_CST_DESTRUCT;

typedef struct {
  MSG_HEADER header;
  char cstName[MAX_STRING_LENGTH];
} M_CST_START;

typedef struct {
  MSG_HEADER header;
  char cstName[MAX_STRING_LENGTH];
} M_CST_STOP;

typedef struct {
  MSG_HEADER header;
  char cstName[MAX_STRING_LENGTH];
  int visionEnabled;
} M_CST_SET_VISUAL;

typedef struct {
  MSG_HEADER header;
  char cstName[MAX_STRING_LENGTH];
  int hapticEnabled;
} M_CST_SET_HAPTIC;

typedef struct {
  MSG_HEADER header;
  char cstName[MAX_STRING_LENGTH];
  double lambdaVal;
} M_CST_SET_LAMBDA;

typedef struct {
  MSG_HEADER header;
  double time;
  double cursorX;
  double cursorY;
  double cursorZ;
  double reactionForce;
} M_CST_DATA;



/////////////////////////////////////////////////
///////////// CUP TASK MESSAGES /////////////////
/////////////////////////////////////////////////
typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
  double escapeAngle;
  double pendulumLength;
  double ballMass;
  double cartMass;
  double ballDamping;
  double cupDamping;
} M_CUPTASK_CREATE;

typedef struct  {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
} M_CUPTASK_DESTRUCT;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
} M_CUPTASK_START;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
} M_CUPTASK_STOP;

typedef struct {
  MSG_HEADER header;
  double time;
  double ballAngle;
  double ballForce;
  double interactionForce;
  double cupPos; 
} M_CUPTASK_DATA;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
} M_CUPTASK_RESET;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
  bool visualEnalbed;
} M_CUPTASK_SET_BALL_VISUALS;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
  bool hapticEnabled;
} M_CUPTASK_SET_BALL_HAPTICS;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
  bool visualEnalbed;
} M_CUPTASK_SET_CUP_VISUALS;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
  bool pathConstraintEnabled;
} M_CUPTASK_SET_PATH_CONSTRAINT;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
  float color[4];
} M_CUPTASK_SET_COLOR_BALL;

typedef struct {
  MSG_HEADER header;
  char cupTaskName[MAX_STRING_LENGTH];
  float color[4];
} M_CUPTASK_SET_COLOR_CUP;



/////////////////////////////////////////////////
////////////// GENERIC HAPTIC EFFECTS ///////////
/////////////////////////////////////////////////
typedef struct {
  MSG_HEADER header;
  double posX;
  double posY;
  double posZ;
  double velX;
  double velY;
  double velZ;
  double forceX;
  double forceY;
  double forceZ;
  char collisions[4][MAX_STRING_LENGTH]; // 4 object collisions at a time
} M_HAPTIC_DATA_STREAM;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  int enabled;
} M_HAPTICS_SET_ENABLED;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
  int enabled;
} M_HAPTICS_SET_ENABLED_WORLD;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  double stiffness;
} M_HAPTICS_SET_STIFFNESS;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
  double bWidth;
  double bHeight;
} M_HAPTICS_BOUNDING_PLANE;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
  double direction;
  double magnitude;
} M_HAPTICS_CONSTANT_FORCE_FIELD;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
  double viscosityMatrix[9];
} M_HAPTICS_VISCOSITY_FIELD;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
} M_HAPTICS_FREEZE_EFFECT;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
  double point1[3];
  double point2[3];
  double stifness;
  double damping;
} M_HAPTICS_LINE_CONSTRAINT;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
  double height;
  double width;
  double depth;
  double center[3];
  double stifness;
  double damping;
} M_HAPTICS_EDGE_STIFFNESS;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
  double forceVector[3];
  double impulseDuration;
  double normalToPlane[3];
  double pointOnPlane[3];
} M_HAPTICS_IMPULSE_PERT;

typedef struct {
  MSG_HEADER header;
  char effectName[MAX_STRING_LENGTH];
} M_HAPTICS_REMOVE_WORLD_EFFECT;



/////////////////////////////////////////////////
////////////// GENERIC VISUAL EFFECTS ///////////
/////////////////////////////////////////////////
typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  int enabled;
} M_GRAPHICS_SET_ENABLED;

typedef struct {
  MSG_HEADER header;
  float color[4]; 
} M_GRAPHICS_CHANGE_BG_COLOR;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  double height;
  double innerRadius;
  double outerRadius;
  unsigned int numSides;
  unsigned int numHeightSegments;
  double position[3];
  double rotation[9];
  float color[4]; 
} M_GRAPHICS_PIPE;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  double aLength;
  double shaftRadius;
  double lengthTip;
  double radiusTip;
  int bidirectional;
  unsigned int numSides;
  double direction[3];
  double position[3];
  float color[4];
} M_GRAPHICS_ARROW;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  float color[4];
} M_GRAPHICS_CHANGE_OBJECT_COLOR;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  double pos[3];
} M_GRAPHICS_SET_OBJECT_LOCALPOS;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  int numDots;
  double coherence;
  double direction;
  double magnitude;
} M_GRAPHICS_MOVING_DOTS;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  double sizeX;
  double sizeY;
  double sizeZ;
  double localPosition[3];
  float color[4];
} M_GRAPHICS_SHAPE_BOX;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  double radius;
  double localPosition[3];
  float color[4];
} M_GRAPHICS_SHAPE_SPHERE;

typedef struct {
  MSG_HEADER header;
  char objectName[MAX_STRING_LENGTH];
  double innerRadius;
  double outerRadius;
  double localPosition[3];
  float color[4];
} M_GRAPHICS_SHAPE_TORUS;
