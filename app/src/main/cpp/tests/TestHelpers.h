#pragma once
#include "Transmitter.h"
#include "Gamepad.h"
#include "JNIAdapt.h"
#include <string>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <unistd.h>

extern std::string gCfgPath;
extern struct android_app *gApp;

using tp_clock = std::chrono::high_resolution_clock;

class TransmitterTestAccess {
public:
    static void ResetInstance() {
        Transmitter::_singleton = nullptr;
    }

    static KEY_STATE* GetKeysState(Transmitter* t) { return t->mKeysState; }
    static FrameData* GetFrameData(Transmitter* t) { return &t->mFrameData; }
    static AxisValue* GetJoystick(Transmitter* t) { return t->mJoystick; }
    static char* GetFrameBuffer(Transmitter* t) { return t->mFrameBuffer; }
    static Config* GetConfig(Transmitter* t) { return &t->mCfg; }
    static bool* GetTurboMark(Transmitter* t) { return t->mTurboMark; }
    static bool* GetTurboActive(Transmitter* t) { return t->mTurboActive; }
    static N3DS_KEY_INDEX GetCaptureTarget(Transmitter* t) { return t->mCaptureTargetN3dsKey; }
    static INPUT_KEY_INDEX GetConflictInput(Transmitter* t) { return t->mConflictInputIdx; }
    static N3DS_KEY_INDEX GetConflictOldN3ds(Transmitter* t) { return t->mConflictOldN3dsIdx; }
    static int GetSock(Transmitter* t) { return t->mSock; }

    // Direct state manipulation helpers
    static void SetKeyState(Transmitter* t, INPUT_KEY_INDEX idx, KEY_STATE state) {
        t->mKeysState[idx] = state;
    }
    static void SetCaptureTarget(Transmitter* t, N3DS_KEY_INDEX target) {
        t->mCaptureTargetN3dsKey = target;
    }
    static void SetConflictState(Transmitter* t, INPUT_KEY_INDEX inputIdx, N3DS_KEY_INDEX oldN3dsIdx) {
        t->mConflictInputIdx = inputIdx;
        t->mConflictOldN3dsIdx = oldN3dsIdx;
    }
    static void ClearConflictState(Transmitter* t) {
        t->mConflictInputIdx = INPUT_KEY_INDEX_INVALID;
        t->mConflictOldN3dsIdx = N3DS_KEY_INDEX_INVALID;
    }

    // Turbo state manipulation for functional tests
    static void SetTurboMarkAt(Transmitter* t, int idx, bool val) {
        t->mTurboMark[idx] = val;
    }
    static bool GetTurboMarkAt(Transmitter* t, int idx) {
        return t->mTurboMark[idx];
    }
    static void SetLastTurboTimeAt(Transmitter* t, int idx, decltype(tp_clock::now()) tp) {
        t->mLastTurboTime[idx] = tp;
    }
    static void SetTurboActiveAt(Transmitter* t, int idx, bool val) {
        t->mTurboActive[idx] = val;
    }
    static bool GetTurboActiveAt(Transmitter* t, int idx) {
        return t->mTurboActive[idx];
    }

    // Capture tracking globals reset (defined in main.cpp)
    static void ResetCaptureTracking();
};

// Shared test fixture — eliminates duplicated setup/teardown across test files
struct TransmitterTestHarness {
    Transmitter* tr = nullptr;
    char tempDir[256] = {};
    struct android_app stubApp;

    void setUp(bool setDefaults = true) {
        snprintf(tempDir, sizeof(tempDir), "/tmp/irc_utest_XXXXXX");
        char* dir = mkdtemp(tempDir);
        if (dir) gCfgPath = dir;

        memset(&stubApp, 0, sizeof(stubApp));
        gApp = &stubApp;
        TransmitterTestAccess::ResetInstance();
        Transmitter::CreateInstance(&stubApp);
        tr = Transmitter::GetInstance();
        if (setDefaults) tr->SetDefaultConfigValue();
    }

    void tearDown() {
        if (tr) { tr->DestroyInstance(); }
        TransmitterTestAccess::ResetInstance();
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", tempDir);
        system(cmd);
    }

    void pressKey(int keyCode, int scanCode = 0) {
        GameActivityKeyEvent ev = {};
        ev.keyCode = keyCode;
        ev.scanCode = scanCode;
        ev.action = AKEY_EVENT_ACTION_DOWN;
        ev.source = AINPUT_SOURCE_GAMEPAD;
        tr->HandleKeyEvent(&ev);
    }

    void releaseKey(int keyCode, int scanCode = 0) {
        GameActivityKeyEvent ev = {};
        ev.keyCode = keyCode;
        ev.scanCode = scanCode;
        ev.action = AKEY_EVENT_ACTION_UP;
        ev.source = AINPUT_SOURCE_GAMEPAD;
        tr->HandleKeyEvent(&ev);
    }

    void pressScanCode(int scanCode) { pressKey(0, scanCode); }
    void pressKeyCode(int keyCode) { pressKey(keyCode, 0); }
};
