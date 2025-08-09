//
// Created by OrangelGLC on 2025/8/6.
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
#define GAMEPAD_BUTTON_SCRSHOT  (101) //ns pro controller only
#define GAMEPAD_BUTTON_HOME     (110) //ns pro controller only
#define GAMEPAD_BUTTON_SHARE    (130) //xbox controller only

/*Indentify via scanCode, left Joy-con only*/
#define GAMEPAD_BUTTON_JCL_UP       (544)
#define GAMEPAD_BUTTON_JCL_Down     (545)
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
#define N3DS_BUTTON_POWER       (2)

enum KEY_INDEX
{
    KEY_INDEX_A = 0,
    KEY_INDEX_B,
    KEY_INDEX_X,
    KEY_INDEX_Y,
    KEY_INDEX_SELECT,
    KEY_INDEX_START,
    KEY_INDEX_LB,
    KEY_INDEX_RB,
    KEY_INDEX_LT,
    KEY_INDEX_RT,
    KEY_INDEX_L3,
    KEY_INDEX_R3,
    KEY_INDEX_UP,
    KEY_INDEX_DOWN,
    KEY_INDEX_LEFT,
    KEY_INDEX_RIGHT,
    KEY_INDEX_HOME,
    KEY_INDEX_INVALID, //Must keep this index as the last one.
};

enum KEY_STATE
{
    KEY_STATE_UP = 0,
    KEY_STATE_DOWN
};

enum JOYSTICK_INDEX
{
    JOYSTICK_L = 0,
    JOYSTICK_R,
    JOYSTICK_INVALID
};

typedef struct AxisValue
{
    float x;
    float y;
}AxisValue;

#endif //INPUTREDIRECTIONCLIENT_ANDROID_GAMEPAD_H
