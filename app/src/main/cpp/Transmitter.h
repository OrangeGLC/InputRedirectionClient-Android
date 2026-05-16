//
// Created by 荆荣 on 2025/8/9.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_TRANSMITTER_H
#define INPUTREDIRECTIONCLIENT_ANDROID_TRANSMITTER_H
#include <jni.h>
#include <chrono>
#include <mutex>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include "Config.h"
typedef unsigned int u32;

struct cJSON;

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
#ifdef UNIT_TESTING
    friend class TransmitterTestAccess;
#endif
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
    void ParseJsonConfig(cJSON *root);
    void GenerateFrame();
    RetVal GetCfgIP(char *buffer, size_t size);
    void SetCfgIP(const char* ip);
    void SetInvertAB(bool flg);
    bool GetInvertAB();
    void SetInvertXY(bool flg);
    bool GetInvertXY();
    void SetKeyMapMode(int mode);
    int GetKeyMapMode();
    void SetSwapJoysticks(bool flg);
    bool GetSwapJoysticks();
    void SetKeyMapping(int inputIdx, int targetIdx);
    int GetKeyMapping(int inputIdx);
    void EnterKeyCapture(int n3dsKeyIndex);
    void ExitKeyCapture();
    void ResolveKeyConflict(bool accept, int sessionId);
    int GetCaptureSessionId();
    void ResetKeyMapping();
    void EnterSpecialCapture(N3DS_KEY_INDEX target);
    void ExitSpecialCapture();
    int GetSpecialComboKey(N3DS_KEY_INDEX target, int index);
    void SetTurbo(N3DS_KEY_INDEX index, bool flg);
    bool GetTurbo(N3DS_KEY_INDEX index);
    void SetTurboInterval(u32 ms);
    u32 GetTurboInterval();
    void SetTurboMode(int index, bool fullAuto);
    bool GetTurboMode(int index);
    void SetHomeMap(bool flg);
    bool GetHomeMap();
    void SetPowerMap(bool flg);
    bool GetPowerMap();
    void SetPowerOffMap(bool flg);
    bool GetPowerOffMap();
    void AdaptToCtrlType(CONTROLLER_TYPE type);
private:
    Transmitter(struct android_app *app);
    ~Transmitter();
private:
    struct android_app *mApp;
    const char* mConfigName = "/config.cfg";
    std::string mConfigPath;
    bool mHasFocus, mIsVisible, mHasWindow;
    bool mTurboMark[MAX_N3DS_KEY_TURBO_INDEX];
    bool mTurboActive[MAX_N3DS_KEY_TURBO_INDEX];
    N3DS_KEY_INDEX mCaptureTargetN3dsKey = N3DS_KEY_INDEX_INVALID;
    INPUT_KEY_INDEX mConflictInputIdx = INPUT_KEY_INDEX_INVALID;
    N3DS_KEY_INDEX mConflictOldN3dsIdx = N3DS_KEY_INDEX_INVALID;
    int mCaptureSessionId = 0;
    int mConflictSessionId = 0;
    std::mutex mCaptureMutex;
    N3DS_KEY_INDEX mSpecialTarget = N3DS_KEY_INDEX_INVALID;
    int mSpecialKeyCount = 0;
    INPUT_KEY_INDEX* GetComboKeysForTarget(N3DS_KEY_INDEX target);
    bool IsCapturableKey(INPUT_KEY_INDEX idx);
    bool IsInputKeyRelevantForCtrlType(INPUT_KEY_INDEX idx, CONTROLLER_TYPE type);
    CONTROLLER_TYPE DetectCtrlTypeFromKey(INPUT_KEY_INDEX idx);
    INPUT_KEY_INDEX FindPhysKeyForN3dsKey(N3DS_KEY_INDEX n3dsKey, CONTROLLER_TYPE type,
                                            INPUT_KEY_INDEX skipIdx = INPUT_KEY_INDEX_INVALID);
    using clock = std::chrono::high_resolution_clock;
    decltype(clock::now()) mLastTurboTime[MAX_N3DS_KEY_TURBO_INDEX];
    decltype(clock::now()) mLastSendTime;
    Config mCfg;
    KEY_STATE mKeysState[MAX_INPUT_KEY_INDEX];
    AxisValue mJoystick[MAX_JOYSTICK_INDEX];
    FrameData mFrameData;
    char mFrameBuffer[20];
    int mSock;
};

#endif //INPUTREDIRECTIONCLIENT_ANDROID_TRANSMITTER_H
