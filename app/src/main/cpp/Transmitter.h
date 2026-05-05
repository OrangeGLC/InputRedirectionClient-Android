//
// Created by 荆荣 on 2025/8/9.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_TRANSMITTER_H
#define INPUTREDIRECTIONCLIENT_ANDROID_TRANSMITTER_H
#include <jni.h>
#include <chrono>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include "Config.h"
typedef unsigned int u32;

typedef struct FrameData
{
    u32 hidPad = 0xfff;
    u32 touchScreenState = 0x2000000;
    u32 circlePadState = 0x7ff7ff;
    u32 irButtonsState = 0;
    u32 cppState = 0x80800081;
    u32 interfaceButtons = 0;
}FrameData;

class Transmitter {

public:
    static Transmitter *_singleton;
    enum RetVal {OK, NOK};
    static Transmitter* GetInstance();
    static Transmitter* CreateInstance(struct android_app *app);
    void DestroyInstance();
    void TaskLoop();
    void LoadConfig();
    RetVal SaveConfig();
    void SendFrame();
    void HandleInputEvent();
    void HandleKeyEvent(GameActivityKeyEvent* keyEvent);
    void HandleMotionEvent(GameActivityMotionEvent* motionEvent);
    void SetDefaultConfigValue();
    void SetDefaultKeyMapValue();
    N3DS_KEY_INDEX GetOutputKeyIndex(INPUT_KEY_INDEX inKeyIndex); /* No use yet. */
    INPUT_KEY_INDEX GetInputKeyIndex(GameActivityKeyEvent* keyEvent);
    bool NeedTurbo();
    void KeyEventToFrameData();
    void MotionEventToFrameData();
    void OutputKeyIndexToFrameData(N3DS_KEY_INDEX outIndex);
    void GenerateFrame();
    RetVal GetCfgIP(char *buffer, size_t size);
    void SetCfgIP(const char* ip);
    void SetInvertAB(bool flg);
    bool GetInvertAB();
    void SetInvertXY(bool flg);
    bool GetInvertXY();
    void SetTurbo(N3DS_KEY_INDEX index, bool flg);
    bool GetTurbo(N3DS_KEY_INDEX index);
    void SetTurboInterval(u32 ms);
    u32 GetTurboInterval();
    void SetHomeMap(bool flg);
    bool GetHomeMap();
    void SetPowerMap(bool flg);
    bool GetPowerMap();
    void SetPowerOffMap(bool flg);
    bool GetPowerOffMap();
private:
    Transmitter(struct android_app *app);
    ~Transmitter();
private:
    struct android_app *mApp;
    const char* mConfigName = "/config.cfg";
    std::string mConfigPath;
    bool mHasFocus, mIsVisible, mHasWindow;
    bool mTurboMark[MAX_N3DS_KEY_TURBO_INDEX];
    using clock = std::chrono::high_resolution_clock;
    typeof(clock::now()) mLastTurboTime;
    typeof(clock::now()) mLastSendTime;
    Config mCfg;
    KEY_STATE mKeysState[MAX_INPUT_KEY_INDEX];
    AxisValue mJoystick[MAX_JOYSTICK_INDEX];
    FrameData mFrameData;
    char mFrameBuffer[20];
    int mSock;
};

#endif //INPUTREDIRECTIONCLIENT_ANDROID_TRANSMITTER_H
