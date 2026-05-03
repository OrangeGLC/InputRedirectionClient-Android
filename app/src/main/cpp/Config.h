//
// Created by 荆荣 on 2025/8/8.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H
#define INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H
#include "Gamepad.h"

#define CONFIG_IP_MAX_LEN 64


/* typedef struct StickCalibratCfg
{
    bool enable;
    AxisValue offset[JOYSTICK_INVALID];
}StickCalibratCfg; */

typedef enum TURBO_STATE
{
    TURBO_DISABLE = 0,
    TURBO_ENABLE,
    TURBO_NOT_SUPPORT,
}TURBO_STATE;

/* NOTE: REMEMBER to set default value in
 * Transmiter::SetDefaultConfigValue
 * */
typedef struct GamepadCfg
{
    float deadZone[MAX_JOYSTICK_INDEX];
    bool invertAB;
    bool invertXY;
    bool mapHome;
    bool mapPower;
    bool mapShut;
    N3DS_KEY_INDEX  targetKeyIndex[MAX_INPUT_KEY_INDEX];
    TURBO_STATE turbo[MAX_N3DS_KEY_TURBO_INDEX]; /* Target keys on N3DS */
    u32 turboIntervalMs;
    //StickCalibratCfg stkCalibratCfg;
}GamepadCfg;

typedef struct Config
{
    u32 cfgSize;
    bool isFirstRun; /* Must as second item because of function Transmiter::LoadConfig */
    char ip[CONFIG_IP_MAX_LEN];
    GamepadCfg gamepadCfg;
}Config;

#endif //INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H