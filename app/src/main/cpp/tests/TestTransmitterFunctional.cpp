#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <chrono>
#include <cmath>

// Define ResetCaptureTracking (declared in TestHelpers.h, globals from mock/JNIAdapt.h)
void TransmitterTestAccess::ResetCaptureTracking() {
    gCaptureResultCallCount = 0;
    gLastCaptureN3dsName = nullptr;
    gLastCapturePhysName = nullptr;
    gLastCaptureConflict = false;
    gLastCaptureConflictN3dsName = nullptr;
}

static struct android_app gStubApp;

TEST_GROUP(TransmitterFunctional)
{
    Transmitter* tr;
    char tempDir[256];

    void setup() override {
        snprintf(tempDir, sizeof(tempDir), "/tmp/irc_utest_XXXXXX");
        char* dir = mkdtemp(tempDir);
        if (dir) gCfgPath = dir;

        memset(&gStubApp, 0, sizeof(gStubApp));
        gApp = &gStubApp;
        TransmitterTestAccess::ResetInstance();
        Transmitter::CreateInstance(&gStubApp);
        tr = Transmitter::GetInstance();
        tr->SetDefaultConfigValue();
        TransmitterTestAccess::ResetCaptureTracking();
    }

    void teardown() override {
        tr->DestroyInstance();
        TransmitterTestAccess::ResetInstance();
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", tempDir);
        system(cmd);
    }

    void pressKey(int keyCode, int scanCode) {
        GameActivityKeyEvent ev = {};
        ev.keyCode = keyCode;
        ev.scanCode = scanCode;
        ev.action = AKEY_EVENT_ACTION_DOWN;
        ev.source = AINPUT_SOURCE_GAMEPAD;
        tr->HandleKeyEvent(&ev);
    }

    void releaseKey(int keyCode, int scanCode) {
        GameActivityKeyEvent ev = {};
        ev.keyCode = keyCode;
        ev.scanCode = scanCode;
        ev.action = AKEY_EVENT_ACTION_UP;
        ev.source = AINPUT_SOURCE_GAMEPAD;
        tr->HandleKeyEvent(&ev);
    }
};

// ==================== Turbo Semi-Auto Functional ====================

TEST(TransmitterFunctional, SemiAutoTurbo_AlternatesFrameOutput)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    tr->SetTurboInterval(1);  // minimal interval for fast testing
    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);

    // Force mLastTurboTime far into the past so elapsed check always passes
    auto longAgo = tp_clock::now() - std::chrono::hours(1);
    TransmitterTestAccess::SetLastTurboTimeAt(tr, N3DS_KEY_INDEX_A, longAgo);

    // First frame: turboMark toggles false→true, key NOT applied
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);

    // Second frame (immediate): turboMark still true, key still NOT applied
    tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);

    // Force time past interval again: turboMark toggles true→false, key APPLIED
    TransmitterTestAccess::SetLastTurboTimeAt(tr, N3DS_KEY_INDEX_A, longAgo);
    tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);

    // Next frame (immediate): turboMark still false, key still APPLIED
    tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
}

TEST(TransmitterFunctional, SemiAutoTurbo_RestoresNeutralWhenKeyUp)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    tr->SetTurboInterval(1);
    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);

    auto longAgo = tp_clock::now() - std::chrono::hours(1);
    // Skip first frame (turboMark toggles true), apply second
    TransmitterTestAccess::SetLastTurboTimeAt(tr, N3DS_KEY_INDEX_A, longAgo);
    tr->GenerateFrame();
    TransmitterTestAccess::SetLastTurboTimeAt(tr, N3DS_KEY_INDEX_A, longAgo);
    tr->GenerateFrame();  // key applied

    // Release key
    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_A, KEY_STATE_UP);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // Key up → no turbo invocation → neutral frame
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
}

// ==================== Turbo Full-Auto Functional ====================

TEST(TransmitterFunctional, FullAutoTurbo_TogglesOnKeyPress)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    tr->SetTurboMode(N3DS_KEY_INDEX_A, true);  // full-auto

    // Press A: full-auto toggles mTurboActive on
    pressKey(GAMEPAD_BUTTON_A, 0);
    CHECK(TransmitterTestAccess::GetTurboActiveAt(tr, N3DS_KEY_INDEX_A));

    // Press A again: full-auto toggles mTurboActive off
    pressKey(GAMEPAD_BUTTON_A, 0);
    CHECK_FALSE(TransmitterTestAccess::GetTurboActiveAt(tr, N3DS_KEY_INDEX_A));
}

TEST(TransmitterFunctional, FullAutoTurbo_GeneratesFramesWhenActive)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    tr->SetTurboInterval(1);
    TransmitterTestAccess::SetTurboActiveAt(tr, N3DS_KEY_INDEX_A, true);

    auto longAgo = tp_clock::now() - std::chrono::hours(1);
    TransmitterTestAccess::SetLastTurboTimeAt(tr, N3DS_KEY_INDEX_A, longAgo);

    // First frame: turboMark toggles false→true → key skipped
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);

    // Force time past interval: turboMark toggles true→false → key applied
    TransmitterTestAccess::SetLastTurboTimeAt(tr, N3DS_KEY_INDEX_A, longAgo);
    tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
}

TEST(TransmitterFunctional, DisableTurbo_ClearsActiveState)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    TransmitterTestAccess::SetTurboActiveAt(tr, N3DS_KEY_INDEX_A, true);

    tr->SetTurbo(N3DS_KEY_INDEX_A, false);
    CHECK_FALSE(TransmitterTestAccess::GetTurboActiveAt(tr, N3DS_KEY_INDEX_A));
}

// ==================== Key Capture via HandleKeyEvent ====================

TEST(TransmitterFunctional, Capture_DirectMappingViaHandleKeyEvent)
{
    // A already maps to A (default). Capture A, press A → no conflict.
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_A;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    tr->HandleKeyEvent(&ev);

    // Capture should exit
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
    // onCaptureResult called with no conflict
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
    // Mapping unchanged: A → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterFunctional, Capture_UnmappedKeyDisplacesOldMapping)
{
    // L3 is unmapped (N3DS_KEY_INDEX_INVALID). Capture X, press L3.
    tr->EnterKeyCapture(N3DS_KEY_INDEX_X);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    tr->HandleKeyEvent(&ev);

    // Capture exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
    // L3 → X
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old occupant of X (A) displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // onCaptureResult: success, no conflict
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterFunctional, Capture_ConflictDetected)
{
    // B maps to N3DS_KEY_INDEX_B. Capture A, press B → conflict.
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_B;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    tr->HandleKeyEvent(&ev);

    // Capture mode still active (waiting for conflict resolution)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(tr));
    // Conflict state recorded
    CHECK_EQUAL(INPUT_KEY_INDEX_B, TransmitterTestAccess::GetConflictInput(tr));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, TransmitterTestAccess::GetConflictOldN3ds(tr));
    // onCaptureResult: conflict=true
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("B", gLastCaptureConflictN3dsName);
    // Mapping unchanged (waiting for resolve)
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

TEST(TransmitterFunctional, Capture_RepeatCountIgnored)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_B;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    ev.repeatCount = 1;  // repeated event
    tr->HandleKeyEvent(&ev);

    // Capture still active, no result called
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(tr));
    CHECK_EQUAL(0, gCaptureResultCallCount);
}

TEST(TransmitterFunctional, Capture_NongamepadSourceIgnored)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_A;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = 0;  // not a gamepad
    tr->HandleKeyEvent(&ev);

    // Capture still active, no result
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(tr));
    CHECK_EQUAL(0, gCaptureResultCallCount);
}

// ==================== InvertAB / InvertXY Effect on Frame ====================

TEST(TransmitterFunctional, InvertAB_SwapsABInFrame)
{
    tr->SetInvertAB(true);

    // Press A physical → should produce N3DS B (bit 1 cleared, not bit 0)
    pressKey(GAMEPAD_BUTTON_A, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // A bit (0) should remain set, B bit (1) should be cleared
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) != 0);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_B)) == 0);
}

TEST(TransmitterFunctional, InvertXY_SwapsXYInFrame)
{
    tr->SetInvertXY(true);

    // Press X physical → should produce N3DS Y (bit 11 cleared, not bit 10)
    pressKey(GAMEPAD_BUTTON_X, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // X bit (10) should remain set, Y bit (11) should be cleared
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_X)) != 0);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_Y)) == 0);
}

// ==================== Swap Joysticks Effect on Frame ====================

TEST(TransmitterFunctional, SwapJoysticks_SwapsStickOutput)
{
    tr->SetSwapJoysticks(true);

    // Left stick (X,Y) = (0.5, 0.3), right stick (Z,RZ,RX,RY) = (0,0,0,0)
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 0.5f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Y] = -0.3f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Z] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RZ] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RX] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RY] = 0.0f;
    tr->HandleMotionEvent(&ev);
    tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // With swap: left stick input → JOYSTICK_R → cppState (non-neutral)
    // and JOYSTICK_L is (0,0) → circlePadState neutral
    CHECK_EQUAL(CIRCLE_PAD_NEUTRAL, fd->circlePadState);
    CHECK(fd->cppState != CPP_STATE_NEUTRAL);
}

// ==================== Shutdown Chord ====================

TEST(TransmitterFunctional, ShutdownChord_Disabled)
{
    tr->SetPowerOffMap(false);
    // L3 and R3 are unmapped by default (N3DS_KEY_INDEX_INVALID)
    // Only the chord path can produce SHUTDOWN when both are pressed

    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_L3, KEY_STATE_DOWN);
    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_R3, KEY_STATE_DOWN);
    tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // mapShut=false → chord guard blocks SHUTDOWN output
    CHECK_EQUAL(0, fd->interfaceButtons & (1 << N3DS_BUTTON_SHUTDOWN));
}

// ==================== Combined Multi-Section Keys ====================

TEST(TransmitterFunctional, CombinedKeys_ThreeFrameSections)
{
    // A (FIRST), ZL (FOURTH), HOME (FIFTH) simultaneously
    tr->SetHomeMap(true);
    tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);

    pressKey(GAMEPAD_BUTTON_A, 0);
    pressKey(GAMEPAD_BUTTON_LT, 0);   // LT → ZL (FOURTH)
    pressKey(GAMEPAD_BUTTON_HOME, 0);  // HOME (FIFTH)
    tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // hidPad: A bit cleared
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
    // irButtonsState: ZL bit set
    CHECK((fd->irButtonsState & (1 << N3DS_BUTTON_ZL)) != 0);
    // interfaceButtons: HOME bit set
    CHECK((fd->interfaceButtons & (1 << N3DS_BUTTON_HOME)) != 0);
}

// ==================== C-Stick 45-Degree Rotation ====================

TEST(TransmitterFunctional, CStick_RotationFormula)
{
    // rx=1.0, ry=0 → rotated: x=M_SQRT1_2*1.0*CPP_BOUND+CPP_CENTER, y=-M_SQRT1_2*1.0*CPP_BOUND+CPP_CENTER
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RX] = 1.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RY] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Z] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RZ] = 0.0f;
    tr->HandleMotionEvent(&ev);
    tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);

    float rx = 1.0f, ry = 0.0f;
    u32 expectedX = (u32)(M_SQRT1_2 * (rx + ry) * 0x7f + 0x80);
    u32 expectedY = (u32)(M_SQRT1_2 * (ry - rx) * 0x7f + 0x80);
    u32 expectedCpp = (expectedY << 24) | (expectedX << 16) | CPP_MAGIC;

    CHECK_EQUAL(expectedCpp, fd->cppState);
}

// ==================== Analog Trigger Full-Auto Turbo Toggle ====================

TEST(TransmitterFunctional, AnalogTrigger_FullAutoTurboToggle)
{
    // LT → ZL (default). Enable turbo + full-auto on ZL.
    tr->SetTurbo(N3DS_KEY_INDEX_ZL, true);
    tr->SetTurboMode(N3DS_KEY_INDEX_ZL, true);

    // Rising edge: BRAKE goes from 0 to 1.0 → toggle mTurboActive ON
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 1.0f;
    tr->HandleMotionEvent(&ev);

    CHECK(TransmitterTestAccess::GetTurboActiveAt(tr, N3DS_KEY_INDEX_ZL));

    // Release trigger (falling edge: BRAKE = 0)
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 0.0f;
    tr->HandleMotionEvent(&ev);
    // Active state unchanged by falling edge
    CHECK(TransmitterTestAccess::GetTurboActiveAt(tr, N3DS_KEY_INDEX_ZL));

    // Second rising edge → toggle OFF
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 1.0f;
    tr->HandleMotionEvent(&ev);
    CHECK_FALSE(TransmitterTestAccess::GetTurboActiveAt(tr, N3DS_KEY_INDEX_ZL));
}

// ==================== Full Config Round-Trip ====================

TEST(TransmitterFunctional, SaveLoad_FullConfigRoundTrip)
{
    tr->SetCfgIP("10.20.30.40");
    tr->SetInvertAB(true);
    tr->SetInvertXY(true);
    tr->SetSwapJoysticks(true);
    tr->SetHomeMap(true);
    tr->SetPowerMap(true);
    tr->SetPowerOffMap(true);
    tr->SetTurboInterval(120);
    tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_B);

    CHECK_EQUAL(Transmitter::OK, tr->SaveConfig());

    // Recreate instance (LoadConfig called in constructor)
    tr->DestroyInstance();
    TransmitterTestAccess::ResetInstance();
    Transmitter::CreateInstance(&gStubApp);
    tr = Transmitter::GetInstance();

    // Verify all settings restored
    char buf[64] = {};
    tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("10.20.30.40", buf);
    CHECK(tr->GetInvertAB());
    CHECK(tr->GetInvertXY());
    CHECK(tr->GetSwapJoysticks());
    CHECK(tr->GetHomeMap());
    CHECK(tr->GetPowerMap());
    CHECK(tr->GetPowerOffMap());
    CHECK_EQUAL(120, tr->GetTurboInterval());
    CHECK_EQUAL(KEYMAP_MODE_CUSTOM, tr->GetKeyMapMode());
    CHECK(tr->GetTurbo(N3DS_KEY_INDEX_A));
    CHECK(tr->GetTurboMode(N3DS_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

// ==================== Key State Tracking ====================

TEST(TransmitterFunctional, KeyDownThenUp_TracksState)
{
    pressKey(GAMEPAD_BUTTON_A, 0);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_A]);

    releaseKey(GAMEPAD_BUTTON_A, 0);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_A]);
}

// ==================== Frame Buffer Content ====================

TEST(TransmitterFunctional, FrameBuffer_AllFieldsEncoded)
{
    tr->SetHomeMap(true);
    tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);

    pressKey(GAMEPAD_BUTTON_A, 0);
    pressKey(GAMEPAD_BUTTON_HOME, 0);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 0.5f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Y] = -0.3f;
    tr->HandleMotionEvent(&ev);

    tr->GenerateFrame();

    char* buf = TransmitterTestAccess::GetFrameBuffer(tr);
    u32 bufHidPad, bufTouch, bufCirclePad, bufCpp, bufIfButtons;
    memcpy(&bufHidPad, buf, 4);
    memcpy(&bufTouch, buf + 4, 4);
    memcpy(&bufCirclePad, buf + 8, 4);
    memcpy(&bufCpp, buf + 12, 4);
    memcpy(&bufIfButtons, buf + 16, 4);

    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(fd->hidPad, bufHidPad);
    CHECK_EQUAL(fd->touchScreenState, bufTouch);
    CHECK_EQUAL(fd->circlePadState, bufCirclePad);
    CHECK_EQUAL(fd->cppState, bufCpp);
    CHECK_EQUAL(fd->interfaceButtons, bufIfButtons);
}

// ==================== Default Neutral Frame After Construction ====================

TEST(TransmitterFunctional, InitialNeutralFrame)
{
    // Constructor calls GenerateFrame() with default config
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
    CHECK_EQUAL(TOUCH_SCREEN_NEUTRAL, fd->touchScreenState);
    CHECK_EQUAL(CIRCLE_PAD_NEUTRAL, fd->circlePadState);
    CHECK_EQUAL(CPP_STATE_NEUTRAL, fd->cppState);
    CHECK_EQUAL(0, fd->interfaceButtons);
}

// ==================== Edge: HOME/POWER Guard ====================

TEST(TransmitterFunctional, HomeKey_NotOutputWhenMapDisabled)
{
    tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);
    tr->SetHomeMap(false);

    pressKey(GAMEPAD_BUTTON_HOME, 0);
    tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(0, fd->interfaceButtons & (1 << N3DS_BUTTON_HOME));
}

TEST(TransmitterFunctional, PowerKey_NotOutputWhenMapDisabled)
{
    tr->SetKeyMapping(INPUT_KEY_INDEX_SHARE, N3DS_KEY_INDEX_POWER);
    tr->SetPowerMap(false);

    pressKey(GAMEPAD_BUTTON_SHARE, 0);
    tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(0, fd->interfaceButtons & (1 << N3DS_BUTTON_POWER));
}

// ==================== L3 capture regression tests ====================

TEST(TransmitterFunctional, CaptureL3_PreservesOtherMappings)
{
    // Set up: A→B, B→A (non-default mappings to verify preservation)
    tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_B);
    tr->SetKeyMapping(INPUT_KEY_INDEX_B, N3DS_KEY_INDEX_A);
    int xBefore = tr->GetKeyMapping(INPUT_KEY_INDEX_X);

    // Capture X with L3
    tr->EnterKeyCapture(N3DS_KEY_INDEX_X);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    tr->HandleKeyEvent(&ev);

    // L3 → X
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old occupant of X displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // Other mappings preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_B));
    // onCaptureResult called
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterFunctional, CaptureL3_UnmappedKeyGetsMapping)
{
    // L3 is unmapped by default. Capture A with L3.
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));

    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    tr->HandleKeyEvent(&ev);

    // L3 → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old A occupant displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterFunctional, CaptureL3_WithJoyconType)
{
    // Simulate JOYCON type (L3 should still be capturable)
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    tr->HandleKeyEvent(&ev);

    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterFunctional, CaptureL3_DoesNotResetJoyConMappings)
{
    // Reproduce: ABXY mapped to JCL D-pad, then capture L3 for L
    // Setup Joy-Con mappings
    tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_A);
    tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN, N3DS_KEY_INDEX_B);
    tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT, N3DS_KEY_INDEX_X);
    tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT, N3DS_KEY_INDEX_Y);
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    // Capture L with L3
    tr->EnterKeyCapture(N3DS_KEY_INDEX_L);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    tr->HandleKeyEvent(&ev);

    // L3 → L
    CHECK_EQUAL(N3DS_KEY_INDEX_L, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Joy-Con D-pad mappings preserved (critical!)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));
    // ctrlType unchanged
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

// ==================== Joy-Con Capture Functional Tests ====================

TEST_GROUP(TransmitterJoyConCapture)
{
    Transmitter* tr;
    char tempDir[256];

    void setup() override {
        snprintf(tempDir, sizeof(tempDir), "/tmp/irc_utest_XXXXXX");
        char* dir = mkdtemp(tempDir);
        if (dir) gCfgPath = dir;

        memset(&gStubApp, 0, sizeof(gStubApp));
        gApp = &gStubApp;
        TransmitterTestAccess::ResetInstance();
        Transmitter::CreateInstance(&gStubApp);
        tr = Transmitter::GetInstance();
        tr->SetDefaultConfigValue();
        TransmitterTestAccess::ResetCaptureTracking();
    }

    void teardown() override {
        tr->DestroyInstance();
        TransmitterTestAccess::ResetInstance();
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", tempDir);
        system(cmd);
    }

    // Helper: press a key via keyCode (for LB, LT, L3)
    void pressKeyCode(int keyCode) {
        GameActivityKeyEvent ev = {};
        ev.keyCode = keyCode;
        ev.scanCode = 0;
        ev.action = AKEY_EVENT_ACTION_DOWN;
        ev.source = AINPUT_SOURCE_GAMEPAD;
        tr->HandleKeyEvent(&ev);
    }

    // Helper: press a key via scanCode (for JCL D-pad, SCRSHOT)
    void pressScanCode(int scanCode) {
        GameActivityKeyEvent ev = {};
        ev.keyCode = 0;
        ev.scanCode = scanCode;
        ev.action = AKEY_EVENT_ACTION_DOWN;
        ev.source = AINPUT_SOURCE_GAMEPAD;
        tr->HandleKeyEvent(&ev);
    }
};

// ---- IsCapturableKey: all Joy-Con keys pass whitelist ----
TEST(TransmitterJoyConCapture, JoyConJclUp_IsCapturable)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    CHECK_EQUAL(1, gCaptureResultCallCount); // captured, not ignored
}

TEST(TransmitterJoyConCapture, JoyConL_IsCapturable)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressKeyCode(GAMEPAD_BUTTON_LB);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

TEST(TransmitterJoyConCapture, JoyConZL_IsCapturable)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressKeyCode(GAMEPAD_BUTTON_LT);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

TEST(TransmitterJoyConCapture, JoyConL3_IsCapturable)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressKeyCode(GAMEPAD_BUTTON_L3);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

TEST(TransmitterJoyConCapture, JoyConScrShot_IsCapturable)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_SCRSHOT);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

// ---- DetectCtrlTypeFromKey: JCL D-pad → JOYCON, others keep current ----
TEST(TransmitterJoyConCapture, JclUp_SetsCtrlTypeToJoycon)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType); // default

    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_JCL_UP);

    // ctrlType auto-switched to JOYCON
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    // Capture succeeded: JCL_UP → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Other JCL keys got AdaptToCtrlType defaults (JCL_DOWN → DOWN)
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    // Old A occupant gets the AdaptToCtrlType default that JCL_UP had (UP)
    // i.e., A → UP (swap: JCL_UP takes A, A takes UP)
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterJoyConCapture, JclAllFour_SetCtrlTypeOnce)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    // Simulate: ctrlType already JOYCON, AdaptToCtrlType already applied,
    // then user wants to remap JCL_UP→A and JCL_DOWN→B
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    // Clear AdaptToCtrlType defaults for keys being remapped
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_INVALID;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_INVALID;

    // First capture: JCL_UP → A
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));

    TransmitterTestAccess::ResetCaptureTracking();

    // Second capture: JCL_DOWN → B (ctrlType still JOYCON, no AdaptToCtrlType)
    tr->EnterKeyCapture(N3DS_KEY_INDEX_B);
    pressScanCode(GAMEPAD_BUTTON_JCL_DOWN);
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    // First mapping preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Second mapping applied
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
}

TEST(TransmitterJoyConCapture, L3_DoesNotChangeCtrlType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    pressKeyCode(GAMEPAD_BUTTON_L3);

    // ctrlType stays JOYCON (L3 is not a D-pad key)
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

TEST(TransmitterJoyConCapture, L_DoesNotChangeCtrlType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressKeyCode(GAMEPAD_BUTTON_LB);

    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

TEST(TransmitterJoyConCapture, ZL_DoesNotChangeCtrlType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressKeyCode(GAMEPAD_BUTTON_LT);

    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

// ---- IsInputKeyRelevantForCtrlType ----
TEST(TransmitterJoyConCapture, JclKeys_RelevantForJoycon)
{
    // JCL D-pad keys should be visible in JOYCON mode
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    // Should succeed (not treated as irrelevant)
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, XboxDpadKeys_NotRelevantForJoycon)
{
    // keyCode-based D-pad should NOT be relevant in JOYCON mode
    // Default: INPUT_KEY_INDEX_UP → INVALID (was cleared by AdaptToCtrlType)
    // Actually AdaptToCtrlType was not called here (ctrlType stays XBOX by default)
    // Let's set JOYCON type manually
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    // Manually adapt
    tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    // keyCode D-pad was cleared → oldTarget = INVALID
    // But !IsInputKeyRelevantForCtrlType(UP, JOYCON) = true → enters unmapped branch
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressKeyCode(GAMEPAD_BUTTON_UP);
    // Capture succeeds (irrelevant keys are treated as unmapped and get assigned)
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

// ---- Capture correctness for each Joy-Con key type ----
TEST(TransmitterJoyConCapture, CaptureJclUp_MappingCorrect)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_JCL_UP);

    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Old A gets the AdaptToCtrlType default (UP) — swap behavior
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
    STRCMP_EQUAL("A", gLastCaptureN3dsName);
}

TEST(TransmitterJoyConCapture, CaptureL_MappingCorrect)
{
    // LB → N3DS L (new mapping for L bumper to 3DS L)
    tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    pressKeyCode(GAMEPAD_BUTTON_LB);

    CHECK_EQUAL(N3DS_KEY_INDEX_L, tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, CaptureZL_MappingCorrect)
{
    // LT → N3DS ZL (default is already LT→ZL, this is no-op capture)
    // Actually oldTarget == ZL and capture target == ZL → hits "no conflict" branch
    tr->EnterKeyCapture(N3DS_KEY_INDEX_ZL);
    pressKeyCode(GAMEPAD_BUTTON_LT);

    // No conflict: mapping unchanged, capture exited
    CHECK_EQUAL(N3DS_KEY_INDEX_ZL, tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, CaptureL3_MappingCorrect)
{
    // L3 → N3DS L (L3 was unmapped, gets new mapping)
    tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    pressKeyCode(GAMEPAD_BUTTON_L3);

    CHECK_EQUAL(N3DS_KEY_INDEX_L, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old occupant (LB → L) displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

// ---- Full remap scenario: all Joy-Con keys mapped to non-default targets ----
TEST(TransmitterJoyConCapture, FullJoyconRemap_AllPreserved)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);

    // Step 1: JCL_UP → A (auto-switches ctrlType to JOYCON)
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 2: JCL_DOWN → B (clear AdaptToCtrlType default first)
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_INVALID;
    tr->EnterKeyCapture(N3DS_KEY_INDEX_B);
    pressScanCode(GAMEPAD_BUTTON_JCL_DOWN);
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 3: Clear LB→L default, then LB → X (avoid false conflict)
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LB] = N3DS_KEY_INDEX_INVALID;
    tr->EnterKeyCapture(N3DS_KEY_INDEX_X);
    pressKeyCode(GAMEPAD_BUTTON_LB);
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 4: Clear LT→ZL default, then LT → Y
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LT] = N3DS_KEY_INDEX_INVALID;
    tr->EnterKeyCapture(N3DS_KEY_INDEX_Y);
    pressKeyCode(GAMEPAD_BUTTON_LT);
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 5: L3 → L (L3 was unmapped by default, no conflict)
    tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    pressKeyCode(GAMEPAD_BUTTON_L3);
    CHECK_EQUAL(N3DS_KEY_INDEX_L, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    TransmitterTestAccess::ResetCaptureTracking();

    // Verify ALL mappings preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Old A swapped to UP (AdaptToCtrlType default for JCL_UP)
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    CHECK_EQUAL(N3DS_KEY_INDEX_L, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

// ---- Conflict detection with Joy-Con keys ----
TEST(TransmitterJoyConCapture, Conflict_JclKeyAlreadyMapped)
{
    // Setup: JCL_UP already mapped to N3DS B, ctrlType already JOYCON
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_B);

    // Try to capture A with JCL_UP (ctrlType stays JOYCON, no AdaptToCtrlType)
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    pressScanCode(GAMEPAD_BUTTON_JCL_UP);

    // Conflict detected (JCL_UP → B, target is A)
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("B", gLastCaptureConflictN3dsName);
    // Mapping unchanged (waiting for resolution)
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
}

TEST(TransmitterJoyConCapture, ConflictResolve_AcceptSwapsJclMapping)
{
    // JCL_UP → B, capture target = A
    tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_B);
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    TransmitterTestAccess::SetCaptureTarget(tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::SetConflictState(tr, INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_B);

    tr->ResolveKeyConflict(true, 0);

    // JCL_UP → A (new mapping accepted)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Old occupant of A displaced to B
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    // Capture exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
}

// ---- D-pad capture via HandleMotionEvent (HAT axes) ----
TEST(TransmitterJoyConCapture, DpadCapture_HatRightDetected)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);

    // Clear RIGHT→RIGHT default to avoid false conflict
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_RIGHT] = N3DS_KEY_INDEX_INVALID;

    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 1.0f;  // right
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    tr->HandleMotionEvent(&ev);

    // HAT D-pad captured → RIGHT → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_RIGHT));
    // ctrlType stays XBOX (was already XBOX, HAT only changes UNKNOWN→XBOX)
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, DpadCapture_DoesNotSwitchFromJoycon)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    // Setup JCL mappings
    tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_A);

    tr->EnterKeyCapture(N3DS_KEY_INDEX_B);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 1.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    tr->HandleMotionEvent(&ev);

    // HAT captured RIGHT → B
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_RIGHT));
    // ctrlType stays JOYCON (HAT no longer switches from JOYCON→XBOX)
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    // JCL mapping preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
}

// ---- Edge: non-gamepad source ignored during capture ----
TEST(TransmitterJoyConCapture, NonGamepadSource_IgnoredDuringCapture)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_JCL_UP;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = 0; // NOT a gamepad source
    tr->HandleKeyEvent(&ev);

    // Capture still active, no result
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(tr));
    CHECK_EQUAL(0, gCaptureResultCallCount);
}
