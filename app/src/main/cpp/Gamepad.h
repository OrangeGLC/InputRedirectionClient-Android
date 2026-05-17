//
// Created by 荆荣 on 2025/8/6.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_GAMEPAD_H
#define INPUTREDIRECTIONCLIENT_ANDROID_GAMEPAD_H

#define DEADZONE_MIN    0.05f

/* Key Code */
#define GAMEPAD_BUTTON_A        (96)
#define GAMEPAD_BUTTON_B        (97)
#define GAMEPAD_BUTTON_X        (99)
#define GAMEPAD_BUTTON_Y        (100)
#define GAMEPAD_BUTTON_UP       (19) //ns pro controller only
#define GAMEPAD_BUTTON_DOWN     (20) //ns pro controller only
#define GAMEPAD_BUTTON_LEFT     (21) //ns pro controller only
#define GAMEPAD_BUTTON_RIGHT    (22) //ns pro controller only
#define GAMEPAD_BUTTON_LB       (102)
#define GAMEPAD_BUTTON_RB       (103)
#define GAMEPAD_BUTTON_LT       (104) //ns pro controller only
#define GAMEPAD_BUTTON_RT       (105) //ns pro controller only
#define GAMEPAD_BUTTON_START    (108)
#define GAMEPAD_BUTTON_SELECT   (109)
#define GAMEPAD_BUTTON_L3       (106)
#define GAMEPAD_BUTTON_R3       (107)
#define GAMEPAD_BUTTON_SCRSHOT  (309) //ns pro controller only (capture button scanCode)
#define GAMEPAD_BUTTON_HOME     (110) //ns pro controller only
#define GAMEPAD_BUTTON_SHARE    (130) //xbox controller only

/*Indentify via scanCode, left Joy-con only*/
#define GAMEPAD_BUTTON_JCL_UP       (544)
#define GAMEPAD_BUTTON_JCL_DOWN     (545)
#define GAMEPAD_BUTTON_JCL_LEFT     (546)
#define GAMEPAD_BUTTON_JCL_RIGHT    (547)

/* Offset value, use to generate net frame */
#define N3DS_BUTTON_A           (0)
#define N3DS_BUTTON_B           (1)
#define N3DS_BUTTON_SELECT      (2)
#define N3DS_BUTTON_START       (3)
#define N3DS_BUTTON_RIGHT       (4)
#define N3DS_BUTTON_LEFT        (5)
#define N3DS_BUTTON_UP          (6)
#define N3DS_BUTTON_DOWN        (7)
#define N3DS_BUTTON_R           (8)
#define N3DS_BUTTON_L           (9)
#define N3DS_BUTTON_ZR          (1)
#define N3DS_BUTTON_ZL          (2)
#define N3DS_BUTTON_X           (10)
#define N3DS_BUTTON_Y           (11)
#define N3DS_BUTTON_HOME        (0)
#define N3DS_BUTTON_POWER       (1)
#define N3DS_BUTTON_SHUTDOWN    (2)

typedef unsigned int  u32;

/*The N3DS controller network frame has a total of 20 bytes,
 * which is divided into 5 parts with each part being 4 bytes.
 * First(Byte0-3):hidPad
 * SECOND(Byte4-7):touchScreenState
 * THIRD(Byte8-11):circlePadState
 * FOURTH(Byte12-15):cppState
 * FIFTH(Byte16-19):interfaceButtons
 * */
typedef enum CODE_OPERATE_OBJ
{
    FIRST,
    SECOND,
    THIRD,
    FOURTH,
    FIFTH
}CODE_OPERATE_OBJ;

typedef struct N3dsKeyInfo
{
    const char* name;
    const u32 offset;
    const CODE_OPERATE_OBJ obj;
}N3dsKeyInfo;

typedef enum CONTROLLER_TYPE
{
    CONTROLLER_TYPE_XBOX,
    CONTROLLER_TYPE_JOYCON,
    CONTROLLER_TYPE_UNKNOWN
}CONTROLLER_TYPE;

typedef enum N3DS_KEY_INDEX
{
    N3DS_KEY_INDEX_A = 0,
    N3DS_KEY_INDEX_B,
    N3DS_KEY_INDEX_X,
    N3DS_KEY_INDEX_Y,
    N3DS_KEY_INDEX_L,
    N3DS_KEY_INDEX_R,
    N3DS_KEY_INDEX_ZL,
    N3DS_KEY_INDEX_ZR,
    N3DS_KEY_INDEX_SELECT,
    N3DS_KEY_INDEX_START,
    N3DS_KEY_INDEX_HOME,
    N3DS_KEY_INDEX_POWER,
    N3DS_KEY_INDEX_UP,
    N3DS_KEY_INDEX_DOWN,
    N3DS_KEY_INDEX_LEFT,
    N3DS_KEY_INDEX_RIGHT,
    N3DS_KEY_INDEX_SHUTDOWN,
    N3DS_KEY_INDEX_INVALID, //Must keep this index as the last one.
}N3DS_KEY_INDEX;

const int MAX_N3DS_KEY_INDEX = N3DS_KEY_INDEX_INVALID;
const int MAX_N3DS_KEY_TURBO_INDEX = 8; /* Only A B X Y L R ZL ZR */
const N3dsKeyInfo gN3DsKeyTab[MAX_N3DS_KEY_INDEX] =
{
/* N3DS_KEY_INDEX_A */{"A", N3DS_BUTTON_A, FIRST},
/* N3DS_KEY_INDEX_B */{"B", N3DS_BUTTON_B, FIRST},
/* N3DS_KEY_INDEX_X */{"X", N3DS_BUTTON_X, FIRST},
/* N3DS_KEY_INDEX_Y */{"Y", N3DS_BUTTON_Y, FIRST},
/* N3DS_KEY_INDEX_L */{"L", N3DS_BUTTON_L, FIRST},
/* N3DS_KEY_INDEX_R */{"R", N3DS_BUTTON_R, FIRST},
/* N3DS_KEY_INDEX_ZL */{"ZL", N3DS_BUTTON_ZL, FOURTH},
/* N3DS_KEY_INDEX_ZR */{"ZR", N3DS_BUTTON_ZR, FOURTH},
/* N3DS_KEY_INDEX_SELECT */{"SELECT", N3DS_BUTTON_SELECT, FIRST},
/* N3DS_KEY_INDEX_START */{"START", N3DS_BUTTON_START, FIRST},
/* N3DS_KEY_INDEX_HOME */{"HOME", N3DS_BUTTON_HOME, FIFTH},
/* N3DS_KEY_INDEX_POWER */{"POWER", N3DS_BUTTON_POWER, FIFTH},
/* N3DS_KEY_INDEX_UP */{"+↑", N3DS_BUTTON_UP, FIRST},
/* N3DS_KEY_INDEX_DOWN */{"+↓", N3DS_BUTTON_DOWN, FIRST},
/* N3DS_KEY_INDEX_LEFT */{"+←", N3DS_BUTTON_LEFT, FIRST},
/* N3DS_KEY_INDEX_RIGHT */{"+→", N3DS_BUTTON_RIGHT, FIRST},
/* N3DS_KEY_INDEX_SHUTDOWN */{"POWEROFF", N3DS_BUTTON_SHUTDOWN, FIFTH},
};

typedef enum INPUT_KEY_INDEX
{
    INPUT_KEY_INDEX_A = 0,
    INPUT_KEY_INDEX_B,
    INPUT_KEY_INDEX_X,
    INPUT_KEY_INDEX_Y,
    INPUT_KEY_INDEX_SELECT,
    INPUT_KEY_INDEX_START,
    INPUT_KEY_INDEX_LB,
    INPUT_KEY_INDEX_RB,
    INPUT_KEY_INDEX_LT,
    INPUT_KEY_INDEX_RT,
    INPUT_KEY_INDEX_L3,
    INPUT_KEY_INDEX_R3,
    INPUT_KEY_INDEX_UP,
    INPUT_KEY_INDEX_DOWN,
    INPUT_KEY_INDEX_LEFT,
    INPUT_KEY_INDEX_RIGHT,
    INPUT_KEY_INDEX_HOME,
    INPUT_KEY_INDEX_SHARE,
    INPUT_KEY_INDEX_SCRSHOT,//NS Pro Controller only
    INPUT_KEY_INDEX_JCL_UP,
    INPUT_KEY_INDEX_JCL_DOWN,
    INPUT_KEY_INDEX_JCL_LEFT,
    INPUT_KEY_INDEX_JCL_RIGHT,
    INPUT_KEY_INDEX_INVALID, //Must keep this index as the last one.
}INPUT_KEY_INDEX;

const int MAX_INPUT_KEY_INDEX=INPUT_KEY_INDEX_INVALID;
typedef struct InputKeyInfo
{
    const char* name;
    const bool isScanCode;
    const u32 keycode;
}InputKeyInfo;
const InputKeyInfo gInputKeyTab[MAX_INPUT_KEY_INDEX]=
{
/* KEY_INDEX_A */{"A", false, GAMEPAD_BUTTON_A},
/* KEY_INDEX_B */{"B", false, GAMEPAD_BUTTON_B},
/* KEY_INDEX_X */{"X", false, GAMEPAD_BUTTON_X},
/* KEY_INDEX_Y */{"Y", false, GAMEPAD_BUTTON_Y},
/* KEY_INDEX_SELECT */{"SELECT", false, GAMEPAD_BUTTON_SELECT},
/* KEY_INDEX_START */{"START", false, GAMEPAD_BUTTON_START},
/* KEY_INDEX_LB */{"L", false, GAMEPAD_BUTTON_LB},
/* KEY_INDEX_RB */{"R", false, GAMEPAD_BUTTON_RB},
/* KEY_INDEX_LT */{"ZL", false, GAMEPAD_BUTTON_LT},
/* KEY_INDEX_RT */{"ZR", false, GAMEPAD_BUTTON_RT},
/* KEY_INDEX_L3 */{"L3", false, GAMEPAD_BUTTON_L3},
/* KEY_INDEX_R3 */{"R3", false, GAMEPAD_BUTTON_R3},
/* KEY_INDEX_UP */{"+↑", false, GAMEPAD_BUTTON_UP},
/* KEY_INDEX_DOWN */{"+↓", false, GAMEPAD_BUTTON_DOWN},
/* KEY_INDEX_LEFT */{"+←", false, GAMEPAD_BUTTON_LEFT},
/* KEY_INDEX_RIGHT */{"+→", false, GAMEPAD_BUTTON_RIGHT},
/* KEY_INDEX_HOME */{"HOME", false, GAMEPAD_BUTTON_HOME},
/* KEY_INDEX_SHARE */{"SHARE", false, GAMEPAD_BUTTON_SHARE},
/* KEY_INDEX_SCRSHOT */{"SHARE", true, GAMEPAD_BUTTON_SCRSHOT},
/* KEY_INDEX_JCL_UP */{"+↑", true, GAMEPAD_BUTTON_JCL_UP},
/* KEY_INDEX_JCL_DOWN */{"+↓", true, GAMEPAD_BUTTON_JCL_DOWN},
/* KEY_INDEX_JCL_LEFT */{"+←", true, GAMEPAD_BUTTON_JCL_LEFT},
/* KEY_INDEX_JCL_RIGHT */{"+→", true, GAMEPAD_BUTTON_JCL_RIGHT},
};

typedef enum KEY_STATE
{
    KEY_STATE_UP = 0,
    KEY_STATE_DOWN
}KEY_STATE;

enum JOYSTICK_INDEX
{
    JOYSTICK_L = 0,
    JOYSTICK_R,
    JOYSTICK_INVALID
};
const int MAX_JOYSTICK_INDEX = JOYSTICK_INVALID;

typedef struct AxisValue
{
    float x;
    float y;
}AxisValue;

/* N3DS network protocol */
#define N3DS_UDP_PORT           4950
#define N3DS_FRAME_SIZE         20

/* Neutral frame values */
#define HID_PAD_NEUTRAL         0xfff
#define CIRCLE_PAD_NEUTRAL      0x7ff7ff
#define CPP_STATE_NEUTRAL       0x80800081
#define TOUCH_SCREEN_NEUTRAL    0x2000000
#define CIRCLE_PAD_CENTER       0x800
#define CIRCLE_PAD_MAX          0xfff
#define CIRCLE_PAD_MIN          0x000
#define CPP_CENTER              0x80
#define CPP_MAX                 0xff
#define CPP_MIN                 0x00
#define CPP_MAGIC               0x81

/* Config and timing */
#define CONFIG_FILE_MAX_SIZE    65536
#define IDLE_FRAME_INTERVAL_MS  5
#define DEFAULT_TURBO_INTERVAL  60
#define AXIS_INT_SCALE          1000

#endif //INPUTREDIRECTIONCLIENT_ANDROID_GAMEPAD_H
