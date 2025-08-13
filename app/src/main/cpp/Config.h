//
// Created by 荆荣 on 2025/8/8.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H
#define INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H
#include <string>
#include "Gamepad.h"


/* typedef struct StickCalibratCfg
{
    bool enable;
    AxisValue offset[JOYSTICK_INVALID];
}StickCalibratCfg; */

typedef enum MULT_TRIGGER_TYPE
{
    MULT_TRIGGER_DISABLE = 0,
    MULT_TRIGGER_ENABLE,
    MULT_TRIGGER_NOT_SUPPORT,
}MULT_TRIGGER_TYPE;

typedef struct KeyMapCfg
{
    N3DS_KEY_INDEX outKeyIndex;
    MULT_TRIGGER_TYPE multTrigger;
}KeyMapCfg;

typedef struct GamepadCfg
{
    float deadZone[MAX_JOYSTICK_INDEX];
    bool invertAB;
    bool invertXY;
    KeyMapCfg  keyMapCfg[MAX_INPUT_KEY_INDEX];
    //StickCalibratCfg stkCalibratCfg;
}GamepadCfg;

typedef struct Config
{
    u32 cfgSize;
    bool isFirstRun; /* Must as second item */
    std::string ip;
    GamepadCfg gamepadCfg;
}Config;

#endif //INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H