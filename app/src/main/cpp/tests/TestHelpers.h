#pragma once
#include "Transmitter.h"
#include "Gamepad.h"
#include <string>

extern std::string gCfgPath;
extern struct android_app *gApp;

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
};
