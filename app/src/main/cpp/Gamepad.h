//
// Created by OrangelGLC on 2025/8/6.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_GAMEPAD_H
#define INPUTREDIRECTIONCLIENT_ANDROID_GAMEPAD_H

#define DEADZONE_MIN    0.05f

#define GAMEPAD_BUTTON_A    (96)
#define GAMEPAD_BUTTON_B    (97)
#define GAMEPAD_BUTTON_X    (99)
#define GAMEPAD_BUTTON_Y    (100)
#define GAMEPAD_BUTTON_UP    (19) //ns pro controller only
#define GAMEPAD_BUTTON_DOWN  (20) //ns pro controller only
#define GAMEPAD_BUTTON_LEFT    (21) //ns pro controller only
#define GAMEPAD_BUTTON_RIGHT    (22) //ns pro controller only
#define GAMEPAD_BUTTON_LB   (102)
#define GAMEPAD_BUTTON_RB   (103)
#define GAMEPAD_BUTTON_L3   (106)
#define GAMEPAD_BUTTON_R3   (107)
#define GAMEPAD_BUTTON_LT   (104) //ns pro controller only
#define GAMEPAD_BUTTON_RT   (105) //ns pro controller only
#define GAMEPAD_BUTTON_START    (108)
#define GAMEPAD_BUTTON_SELECT   (109)
#define GAMEPAD_BUTTON_SCRSHOT   (110) //ns pro controller only
#define GAMEPAD_BUTTON_SHARE   (130) //xbox controller only

#define GAMEPAD_BUTTON_A_OFFSET    (0)
#define GAMEPAD_BUTTON_B_OFFSET    (1)
#define GAMEPAD_BUTTON_SELECT_OFFSET   (2)
#define GAMEPAD_BUTTON_START_OFFSET    (3)
#define GAMEPAD_BUTTON_RIGHT_OFFSET    (4) //ns pro controller only
#define GAMEPAD_BUTTON_LEFT_OFFSET    (5) //ns pro controller only
#define GAMEPAD_BUTTON_UP_OFFSET    (6) //ns pro controller only
#define GAMEPAD_BUTTON_DOWN_OFFSET  (7) //ns pro controller only
#define GAMEPAD_BUTTON_RB_OFFSET   (8)
#define GAMEPAD_BUTTON_LB_OFFSET   (9)
#define GAMEPAD_BUTTON_RT_OFFSET   (1) //ns pro controller only
#define GAMEPAD_BUTTON_LT_OFFSET   (2) //ns pro controller only
#define GAMEPAD_BUTTON_X_OFFSET    (10)
#define GAMEPAD_BUTTON_Y_OFFSET    (11)

#define GAMEPAD_BUTTON_L3_OFFSET   (106)
#define GAMEPAD_BUTTON_R3_OFFSET   (107)
#define GAMEPAD_BUTTON_SCRSHOT_OFFSET   (110) //ns pro controller only
#define GAMEPAD_BUTTON_SHARE_OFFSET   (130) //xbox controller only

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
    KEY_INDEX_USER_1,
    KEY_INDEX_INVALID,
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
