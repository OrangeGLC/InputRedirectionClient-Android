//
// Created by 荆荣 on 2025/8/8.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H
#define INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H
#include "Gamepad.h"

#define CONFIG_IP_MAX_LEN 64
#define CONFIG_JSON_VERSION 1


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

typedef enum TURBO_MODE
{
    TURBO_MODE_SEMI = 0,
    TURBO_MODE_FULL,
}TURBO_MODE;

typedef enum KEYMAP_MODE
{
    KEYMAP_MODE_SIMPLE = 0,
    KEYMAP_MODE_CUSTOM
}KEYMAP_MODE;

/* NOTE: REMEMBER to set default value in
 * Transmiter::SetDefaultConfigValue
 * */
typedef struct GamepadCfg
{
    float deadZone[MAX_JOYSTICK_INDEX];
    KEYMAP_MODE keyMapMode;
    bool swapJoysticks;
    bool invertAB;
    bool invertXY;
    bool mapHome;
    bool mapPower;
    bool mapShut;
    N3DS_KEY_INDEX  targetKeyIndex[MAX_INPUT_KEY_INDEX];
    TURBO_STATE turbo[MAX_N3DS_KEY_TURBO_INDEX]; /* Target keys on N3DS */
    u32 turboIntervalMs;
    TURBO_MODE turboMode[MAX_N3DS_KEY_TURBO_INDEX];
    CONTROLLER_TYPE ctrlType;
    INPUT_KEY_INDEX homeKeys[2];  // HOME combo
    INPUT_KEY_INDEX powerKeys[2]; // POWER combo
    INPUT_KEY_INDEX shutKeys[2];  // SHUTDOWN combo
}GamepadCfg;

typedef struct Config
{
    char ip[CONFIG_IP_MAX_LEN];
    GamepadCfg gamepadCfg;
}Config;

#endif //INPUTREDIRECTIONCLIENT_ANDROID_CONFIG_H