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

TEST_GROUP(TransmitterFunctional)
{
    TransmitterTestHarness h;

    void setup() override {
        h.setUp();
        TransmitterTestAccess::ResetCaptureTracking();
    }
    void teardown() override { h.tearDown(); }
};

// ==================== Turbo Semi-Auto Functional ====================

TEST(TransmitterFunctional, SemiAutoTurbo_AlternatesFrameOutput)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    h.tr->SetTurboInterval(1);  // minimal interval for fast testing
    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);

    // Force mLastTurboTime far into the past so elapsed check always passes
    auto longAgo = tp_clock::now() - std::chrono::hours(1);
    TransmitterTestAccess::SetLastTurboTimeAt(h.tr, N3DS_KEY_INDEX_A, longAgo);

    // First frame: turboMark toggles false→true, key NOT applied
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);

    // Second frame (immediate): turboMark still true, key still NOT applied
    h.tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);

    // Force time past interval again: turboMark toggles true→false, key APPLIED
    TransmitterTestAccess::SetLastTurboTimeAt(h.tr, N3DS_KEY_INDEX_A, longAgo);
    h.tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);

    // Next frame (immediate): turboMark still false, key still APPLIED
    h.tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
}

TEST(TransmitterFunctional, SemiAutoTurbo_RestoresNeutralWhenKeyUp)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    h.tr->SetTurboInterval(1);
    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);

    auto longAgo = tp_clock::now() - std::chrono::hours(1);
    // Skip first frame (turboMark toggles true), apply second
    TransmitterTestAccess::SetLastTurboTimeAt(h.tr, N3DS_KEY_INDEX_A, longAgo);
    h.tr->GenerateFrame();
    TransmitterTestAccess::SetLastTurboTimeAt(h.tr, N3DS_KEY_INDEX_A, longAgo);
    h.tr->GenerateFrame();  // key applied

    // Release key
    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_A, KEY_STATE_UP);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // Key up → no turbo invocation → neutral frame
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
}

// ==================== Turbo Full-Auto Functional ====================

TEST(TransmitterFunctional, FullAutoTurbo_TogglesOnKeyPress)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    h.tr->SetTurboMode(N3DS_KEY_INDEX_A, true);  // full-auto

    // Press A: full-auto toggles mTurboActive on
    h.pressKey(GAMEPAD_BUTTON_A, 0);
    CHECK(TransmitterTestAccess::GetTurboActiveAt(h.tr, N3DS_KEY_INDEX_A));

    // Press A again: full-auto toggles mTurboActive off
    h.pressKey(GAMEPAD_BUTTON_A, 0);
    CHECK_FALSE(TransmitterTestAccess::GetTurboActiveAt(h.tr, N3DS_KEY_INDEX_A));
}

TEST(TransmitterFunctional, FullAutoTurbo_GeneratesFramesWhenActive)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    h.tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    h.tr->SetTurboInterval(1);
    TransmitterTestAccess::SetTurboActiveAt(h.tr, N3DS_KEY_INDEX_A, true);

    auto longAgo = tp_clock::now() - std::chrono::hours(1);
    TransmitterTestAccess::SetLastTurboTimeAt(h.tr, N3DS_KEY_INDEX_A, longAgo);

    // First frame: turboMark toggles false→true → key skipped
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);

    // Force time past interval: turboMark toggles true→false → key applied
    TransmitterTestAccess::SetLastTurboTimeAt(h.tr, N3DS_KEY_INDEX_A, longAgo);
    h.tr->GenerateFrame();
    fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
}

TEST(TransmitterFunctional, DisableTurbo_ClearsActiveState)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    h.tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    TransmitterTestAccess::SetTurboActiveAt(h.tr, N3DS_KEY_INDEX_A, true);

    h.tr->SetTurbo(N3DS_KEY_INDEX_A, false);
    CHECK_FALSE(TransmitterTestAccess::GetTurboActiveAt(h.tr, N3DS_KEY_INDEX_A));
}

// ==================== Key Capture via HandleKeyEvent ====================

TEST(TransmitterFunctional, Capture_DirectMappingViaHandleKeyEvent)
{
    // A already maps to A (default). Capture A, press A → no conflict.
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_A;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // Capture should exit
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
    // onCaptureResult called with no conflict
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
    // Mapping unchanged: A → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterFunctional, Capture_UnmappedKeyDisplacesOldMapping)
{
    // L3 is unmapped (N3DS_KEY_INDEX_INVALID). Capture X, press L3.
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_X);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // Capture exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
    // L3 → X
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old occupant of X (A) displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // onCaptureResult: success, no conflict
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterFunctional, Capture_ConflictDetected)
{
    // B maps to N3DS_KEY_INDEX_B. Capture A, press B → conflict.
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_B;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // Capture mode still active (waiting for conflict resolution)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(h.tr));
    // Conflict state recorded
    CHECK_EQUAL(INPUT_KEY_INDEX_B, TransmitterTestAccess::GetConflictInput(h.tr));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, TransmitterTestAccess::GetConflictOldN3ds(h.tr));
    // onCaptureResult: conflict=true
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("B", gLastCaptureConflictN3dsName);
    // Mapping unchanged (waiting for resolve)
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

TEST(TransmitterFunctional, Capture_RepeatCountIgnored)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_B;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    ev.repeatCount = 1;  // repeated event
    h.tr->HandleKeyEvent(&ev);

    // Capture still active, no result called
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(h.tr));
    CHECK_EQUAL(0, gCaptureResultCallCount);
}

TEST(TransmitterFunctional, Capture_NongamepadSourceIgnored)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_A;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = 0;  // not a gamepad
    h.tr->HandleKeyEvent(&ev);

    // Capture still active, no result
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(h.tr));
    CHECK_EQUAL(0, gCaptureResultCallCount);
}

// ==================== InvertAB / InvertXY Effect on Frame ====================

TEST(TransmitterFunctional, InvertAB_SwapsABInFrame)
{
    h.tr->SetInvertAB(true);

    // Press A physical → should produce N3DS B (bit 1 cleared, not bit 0)
    h.pressKey(GAMEPAD_BUTTON_A, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // A bit (0) should remain set, B bit (1) should be cleared
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) != 0);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_B)) == 0);
}

TEST(TransmitterFunctional, InvertXY_SwapsXYInFrame)
{
    h.tr->SetInvertXY(true);

    // Press X physical → should produce N3DS Y (bit 11 cleared, not bit 10)
    h.pressKey(GAMEPAD_BUTTON_X, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // X bit (10) should remain set, Y bit (11) should be cleared
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_X)) != 0);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_Y)) == 0);
}

// ==================== Swap Joysticks Effect on Frame ====================

TEST(TransmitterFunctional, SwapJoysticks_SwapsStickOutput)
{
    h.tr->SetSwapJoysticks(true);

    // Left stick (X,Y) = (0.5, 0.3), right stick (Z,RZ,RX,RY) = (0,0,0,0)
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 0.5f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Y] = -0.3f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Z] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RZ] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RX] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RY] = 0.0f;
    h.tr->HandleMotionEvent(&ev);
    h.tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // With swap: left stick input → JOYSTICK_R → cppState (non-neutral)
    // and JOYSTICK_L is (0,0) → circlePadState neutral
    CHECK_EQUAL(CIRCLE_PAD_NEUTRAL, fd->circlePadState);
    CHECK(fd->cppState != CPP_STATE_NEUTRAL);
}

// ==================== Shutdown Chord ====================

TEST(TransmitterFunctional, ShutdownChord_Disabled)
{
    h.tr->SetPowerOffMap(false);
    // L3 and R3 are unmapped by default (N3DS_KEY_INDEX_INVALID)
    // Only the chord path can produce SHUTDOWN when both are pressed

    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_L3, KEY_STATE_DOWN);
    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_R3, KEY_STATE_DOWN);
    h.tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // mapShut=false → chord guard blocks SHUTDOWN output
    CHECK_EQUAL(0, fd->interfaceButtons & (1 << N3DS_BUTTON_SHUTDOWN));
}

// ==================== Combined Multi-Section Keys ====================

TEST(TransmitterFunctional, CombinedKeys_ThreeFrameSections)
{
    // A (FIRST), ZL (FOURTH), HOME (FIFTH) simultaneously
    h.tr->SetHomeMap(true);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);

    h.pressKey(GAMEPAD_BUTTON_A, 0);
    h.pressKey(GAMEPAD_BUTTON_LT, 0);   // LT → ZL (FOURTH)
    h.pressKey(GAMEPAD_BUTTON_HOME, 0);  // HOME (FIFTH)
    h.tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
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
    h.tr->HandleMotionEvent(&ev);
    h.tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);

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
    h.tr->SetTurbo(N3DS_KEY_INDEX_ZL, true);
    h.tr->SetTurboMode(N3DS_KEY_INDEX_ZL, true);

    // Rising edge: BRAKE goes from 0 to 1.0 → toggle mTurboActive ON
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 1.0f;
    h.tr->HandleMotionEvent(&ev);

    CHECK(TransmitterTestAccess::GetTurboActiveAt(h.tr, N3DS_KEY_INDEX_ZL));

    // Release trigger (falling edge: BRAKE = 0)
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 0.0f;
    h.tr->HandleMotionEvent(&ev);
    // Active state unchanged by falling edge
    CHECK(TransmitterTestAccess::GetTurboActiveAt(h.tr, N3DS_KEY_INDEX_ZL));

    // Second rising edge → toggle OFF
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 1.0f;
    h.tr->HandleMotionEvent(&ev);
    CHECK_FALSE(TransmitterTestAccess::GetTurboActiveAt(h.tr, N3DS_KEY_INDEX_ZL));
}

// ==================== Full Config Round-Trip ====================

TEST(TransmitterFunctional, SaveLoad_FullConfigRoundTrip)
{
    h.tr->SetCfgIP("10.20.30.40");
    h.tr->SetInvertAB(true);
    h.tr->SetInvertXY(true);
    h.tr->SetSwapJoysticks(true);
    h.tr->SetHomeMap(true);
    h.tr->SetPowerMap(true);
    h.tr->SetPowerOffMap(true);
    h.tr->SetTurboInterval(120);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    h.tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_B);

    CHECK_EQUAL(Transmitter::OK, h.tr->SaveConfig());

    // Recreate instance (LoadConfig called in constructor)
    h.tr->DestroyInstance();
    TransmitterTestAccess::ResetInstance();
    Transmitter::CreateInstance(&h.stubApp);
    h.tr = Transmitter::GetInstance();

    // Verify all settings restored
    char buf[64] = {};
    h.tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("10.20.30.40", buf);
    CHECK(h.tr->GetInvertAB());
    CHECK(h.tr->GetInvertXY());
    CHECK(h.tr->GetSwapJoysticks());
    CHECK(h.tr->GetHomeMap());
    CHECK(h.tr->GetPowerMap());
    CHECK(h.tr->GetPowerOffMap());
    CHECK_EQUAL(120, h.tr->GetTurboInterval());
    CHECK_EQUAL(KEYMAP_MODE_CUSTOM, h.tr->GetKeyMapMode());
    CHECK(h.tr->GetTurbo(N3DS_KEY_INDEX_A));
    CHECK(h.tr->GetTurboMode(N3DS_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

// ==================== Key State Tracking ====================

TEST(TransmitterFunctional, KeyDownThenUp_TracksState)
{
    h.pressKey(GAMEPAD_BUTTON_A, 0);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_A]);

    h.releaseKey(GAMEPAD_BUTTON_A, 0);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_A]);
}

// ==================== Frame Buffer Content ====================

TEST(TransmitterFunctional, FrameBuffer_AllFieldsEncoded)
{
    h.tr->SetHomeMap(true);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);

    h.pressKey(GAMEPAD_BUTTON_A, 0);
    h.pressKey(GAMEPAD_BUTTON_HOME, 0);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 0.5f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Y] = -0.3f;
    h.tr->HandleMotionEvent(&ev);

    h.tr->GenerateFrame();

    char* buf = TransmitterTestAccess::GetFrameBuffer(h.tr);
    u32 bufHidPad, bufTouch, bufCirclePad, bufCpp, bufIfButtons;
    memcpy(&bufHidPad, buf, 4);
    memcpy(&bufTouch, buf + 4, 4);
    memcpy(&bufCirclePad, buf + 8, 4);
    memcpy(&bufCpp, buf + 12, 4);
    memcpy(&bufIfButtons, buf + 16, 4);

    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
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
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
    CHECK_EQUAL(TOUCH_SCREEN_NEUTRAL, fd->touchScreenState);
    CHECK_EQUAL(CIRCLE_PAD_NEUTRAL, fd->circlePadState);
    CHECK_EQUAL(CPP_STATE_NEUTRAL, fd->cppState);
    CHECK_EQUAL(0, fd->interfaceButtons);
}

// ==================== Edge: HOME/POWER Guard ====================

TEST(TransmitterFunctional, HomeKey_NotOutputWhenMapDisabled)
{
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);
    h.tr->SetHomeMap(false);

    h.pressKey(GAMEPAD_BUTTON_HOME, 0);
    h.tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(0, fd->interfaceButtons & (1 << N3DS_BUTTON_HOME));
}

TEST(TransmitterFunctional, PowerKey_NotOutputWhenMapDisabled)
{
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_SHARE, N3DS_KEY_INDEX_POWER);
    h.tr->SetPowerMap(false);

    h.pressKey(GAMEPAD_BUTTON_SHARE, 0);
    h.tr->GenerateFrame();

    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(0, fd->interfaceButtons & (1 << N3DS_BUTTON_POWER));
}

// ==================== L3 capture regression tests ====================

TEST(TransmitterFunctional, CaptureL3_PreservesOtherMappings)
{
    // Set up: A→B, B→A (non-default mappings to verify preservation)
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_B);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_B, N3DS_KEY_INDEX_A);
    int xBefore = h.tr->GetKeyMapping(INPUT_KEY_INDEX_X);

    // Capture X with L3
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_X);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // L3 → X
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old occupant of X displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // Other mappings preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
    // onCaptureResult called
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterFunctional, CaptureL3_UnmappedKeyGetsMapping)
{
    // L3 is unmapped by default. Capture A with L3.
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // L3 → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old A occupant displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterFunctional, CaptureL3_WithJoyconType)
{
    // Simulate JOYCON type (L3 should still be capturable)
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterFunctional, CaptureL3_DoesNotResetJoyConMappings)
{
    // Reproduce: ABXY mapped to JCL D-pad, then capture L3 for L
    // Setup Joy-Con mappings
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_A);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN, N3DS_KEY_INDEX_B);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT, N3DS_KEY_INDEX_X);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT, N3DS_KEY_INDEX_Y);
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    // Capture L with L3
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_L3;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // L3 → L
    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Joy-Con D-pad mappings preserved (critical!)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));
    // ctrlType unchanged
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

// ==================== Joy-Con Capture Functional Tests ====================

TEST_GROUP(TransmitterJoyConCapture)
{
    TransmitterTestHarness h;

    void setup() override {
        h.setUp();
        TransmitterTestAccess::ResetCaptureTracking();
    }
    void teardown() override { h.tearDown(); }
};

// ---- IsCapturableKey: all Joy-Con keys pass whitelist ----
TEST(TransmitterJoyConCapture, JoyConJclUp_IsCapturable)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    CHECK_EQUAL(1, gCaptureResultCallCount); // captured, not ignored
}

TEST(TransmitterJoyConCapture, JoyConL_IsCapturable)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressKeyCode(GAMEPAD_BUTTON_LB);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

TEST(TransmitterJoyConCapture, JoyConZL_IsCapturable)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressKeyCode(GAMEPAD_BUTTON_LT);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

TEST(TransmitterJoyConCapture, JoyConL3_IsCapturable)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressKeyCode(GAMEPAD_BUTTON_L3);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

TEST(TransmitterJoyConCapture, JoyConScrShot_IsCapturable)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_SCRSHOT);
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

// ---- DetectCtrlTypeFromKey: JCL D-pad → JOYCON, others keep current ----
TEST(TransmitterJoyConCapture, JclUp_SetsCtrlTypeToJoycon)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType); // default

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);

    // ctrlType auto-switched to JOYCON, AdaptToCtrlType set JCL_UP→UP
    // oldTarget=UP (read AFTER AdaptToCtrlType) → conflict with capture target A
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("+↑", gLastCaptureConflictN3dsName);

    // Accept the conflict (swap: JCL_UP→A, old A→UP)
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
}

TEST(TransmitterJoyConCapture, JclAllFour_SetCtrlTypeOnce)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    // Simulate: ctrlType already JOYCON, AdaptToCtrlType already applied,
    // then user wants to remap JCL_UP→A and JCL_DOWN→B
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    // Clear AdaptToCtrlType defaults for keys being remapped
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_INVALID;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_INVALID;

    // First capture: JCL_UP → A
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));

    TransmitterTestAccess::ResetCaptureTracking();

    // Second capture: JCL_DOWN → B (ctrlType still JOYCON, no AdaptToCtrlType)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_B);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_DOWN);
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    // First mapping preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Second mapping applied
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
}

TEST(TransmitterJoyConCapture, L3_DoesNotChangeCtrlType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    h.pressKeyCode(GAMEPAD_BUTTON_L3);

    // ctrlType stays JOYCON (L3 is not a D-pad key)
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

TEST(TransmitterJoyConCapture, L_DoesNotChangeCtrlType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressKeyCode(GAMEPAD_BUTTON_LB);

    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

TEST(TransmitterJoyConCapture, ZL_DoesNotChangeCtrlType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressKeyCode(GAMEPAD_BUTTON_LT);

    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

// ---- IsInputKeyRelevantForCtrlType ----
TEST(TransmitterJoyConCapture, JclKeys_RelevantForJoycon)
{
    // JCL D-pad keys should be visible in JOYCON mode
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);
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
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    // Manually adapt
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    // keyCode D-pad was cleared → oldTarget = INVALID
    // But !IsInputKeyRelevantForCtrlType(UP, JOYCON) = true → enters unmapped branch
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressKeyCode(GAMEPAD_BUTTON_UP);
    // Capture succeeds (irrelevant keys are treated as unmapped and get assigned)
    CHECK_EQUAL(1, gCaptureResultCallCount);
}

// ---- Capture correctness for each Joy-Con key type ----
TEST(TransmitterJoyConCapture, CaptureJclUp_MappingCorrect)
{
    // First JCL capture: AdaptToCtrlType sets JCL_UP→UP → conflict
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);

    // Conflict detected (JCL_UP already mapped to UP by AdaptToCtrlType)
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("+↑", gLastCaptureConflictN3dsName);

    // Accept conflict → swap
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterJoyConCapture, CaptureL_MappingCorrect)
{
    // LB → N3DS L (new mapping for L bumper to 3DS L)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    h.pressKeyCode(GAMEPAD_BUTTON_LB);

    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, CaptureZL_MappingCorrect)
{
    // LT → N3DS ZL (default is already LT→ZL, this is no-op capture)
    // Actually oldTarget == ZL and capture target == ZL → hits "no conflict" branch
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_ZL);
    h.pressKeyCode(GAMEPAD_BUTTON_LT);

    // No conflict: mapping unchanged, capture exited
    CHECK_EQUAL(N3DS_KEY_INDEX_ZL, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, CaptureL3_MappingCorrect)
{
    // L3 → N3DS L (L3 was unmapped, gets new mapping)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    h.pressKeyCode(GAMEPAD_BUTTON_L3);

    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    // Old occupant (LB → L) displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

// ---- Full remap scenario: all Joy-Con keys mapped to non-default targets ----
TEST(TransmitterJoyConCapture, FullJoyconRemap_AllPreserved)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);

    // Step 1: JCL_UP → A (auto-switches ctrlType to JOYCON, triggers conflict)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    CHECK(gLastCaptureConflict);
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 2: JCL_DOWN → B (clear AdaptToCtrlType default first)
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_INVALID;
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_B);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_DOWN);
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 3: Clear LB→L default, then LB → X (avoid false conflict)
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LB] = N3DS_KEY_INDEX_INVALID;
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_X);
    h.pressKeyCode(GAMEPAD_BUTTON_LB);
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 4: Clear LT→ZL default, then LT → Y
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LT] = N3DS_KEY_INDEX_INVALID;
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_Y);
    h.pressKeyCode(GAMEPAD_BUTTON_LT);
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 5: L3 → L (L3 was unmapped by default, no conflict)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    h.pressKeyCode(GAMEPAD_BUTTON_L3);
    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    TransmitterTestAccess::ResetCaptureTracking();

    // Verify ALL mappings preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Old A swapped to UP (AdaptToCtrlType default for JCL_UP)
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
}

// ---- Conflict detection with Joy-Con keys ----
TEST(TransmitterJoyConCapture, Conflict_JclKeyAlreadyMapped)
{
    // Setup: JCL_UP already mapped to N3DS B, ctrlType already JOYCON
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_B);

    // Try to capture A with JCL_UP (ctrlType stays JOYCON, no AdaptToCtrlType)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);

    // Conflict detected (JCL_UP → B, target is A)
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("B", gLastCaptureConflictN3dsName);
    // Mapping unchanged (waiting for resolution)
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
}

TEST(TransmitterJoyConCapture, ConflictResolve_AcceptSwapsJclMapping)
{
    // JCL_UP → B, capture target = A
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_B);
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;

    TransmitterTestAccess::SetCaptureTarget(h.tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::SetConflictState(h.tr, INPUT_KEY_INDEX_JCL_UP, N3DS_KEY_INDEX_B);

    h.tr->ResolveKeyConflict(true, 0);

    // JCL_UP → A (new mapping accepted)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Old occupant of A displaced to B
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    // Capture exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
}

// ---- D-pad capture via HandleMotionEvent (HAT axes) ----
TEST(TransmitterJoyConCapture, DpadCapture_HatRightDetected)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);

    // Clear RIGHT→RIGHT default to avoid false conflict
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_RIGHT] = N3DS_KEY_INDEX_INVALID;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 1.0f;  // right
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    h.tr->HandleMotionEvent(&ev);

    // HAT D-pad captured → RIGHT → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RIGHT));
    // ctrlType stays XBOX (was already XBOX, HAT only changes UNKNOWN→XBOX)
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, DpadCapture_SwitchesFromJoyconToXbox)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_B);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 1.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    h.tr->HandleMotionEvent(&ev);

    // HAT D-pad detected → auto-switches ctrlType JOYCON→XBOX
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);
    // HAT captured RIGHT → B
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RIGHT));
    // JCL keys cleared by AdaptToCtrlType(XBOX)
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Xbox D-pad defaults restored (except RIGHT which was captured)
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));
}

// ---- Edge: non-gamepad source ignored during capture ----
TEST(TransmitterJoyConCapture, NonGamepadSource_IgnoredDuringCapture)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_JCL_UP;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = 0; // NOT a gamepad source
    h.tr->HandleKeyEvent(&ev);

    // Capture still active, no result
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(h.tr));
    CHECK_EQUAL(0, gCaptureResultCallCount);
}

// ---- Analog trigger capture via HandleMotionEvent ----

TEST(TransmitterJoyConCapture, TriggerLT_Captured)
{
    // Clear LT→ZL default to avoid conflict
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LT] = N3DS_KEY_INDEX_INVALID;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 1.0f;
    h.tr->HandleMotionEvent(&ev);

    // LT captured → A
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    // Old A displaced
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, TriggerRT_Captured)
{
    // Clear RT→ZR default to avoid conflict
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_RT] = N3DS_KEY_INDEX_INVALID;

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_B);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_GAS] = 1.0f;
    h.tr->HandleMotionEvent(&ev);

    // RT captured → B
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RT));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, TriggerLT_NotCapturedWhenReleased)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    // Trigger at 0 (released) → no rising edge
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 0.0f;
    h.tr->HandleMotionEvent(&ev);

    // No capture occurred
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(h.tr));
    CHECK_EQUAL(0, gCaptureResultCallCount);
}

TEST(TransmitterJoyConCapture, TriggerLT_ConflictDetected)
{
    // LT already mapped to ZL by default
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 1.0f;
    h.tr->HandleMotionEvent(&ev);

    // Conflict: LT → ZL, target is A
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("ZL", gLastCaptureConflictN3dsName);
}

TEST(TransmitterJoyConCapture, TriggerLT_SuppressedWhenConsumed)
{
    // Clear LT→ZL default to avoid conflict
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LT] = N3DS_KEY_INDEX_INVALID;

    // First motion event: trigger rising edge → captured
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_BRAKE] = 1.0f;
    h.tr->HandleMotionEvent(&ev);
    CHECK_EQUAL(1, gCaptureResultCallCount);

    // Trigger state should be UP (event consumed by capture, lt set to 0)
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_LT]);
}

// ---- Mode switch persistence: CUSTOM → SIMPLE → CUSTOM ----

TEST(TransmitterJoyConCapture, ModeSwitch_CustomToSimpleToCustom_PreservesMappings)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);

    // Step 1: Switch to CUSTOM mode and set up mappings (simulate capture: clear defaults)
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_B;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_B] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_X] = N3DS_KEY_INDEX_Y;
    // Clear displaced old occupants (as capture would)
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_Y] = N3DS_KEY_INDEX_INVALID;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_UP;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_DOWN;
    // Clear displaced defaults to avoid duplicate N3DS key mappings
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_UP] = N3DS_KEY_INDEX_INVALID;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_DOWN] = N3DS_KEY_INDEX_INVALID;

    // Step 2: Switch to SIMPLE mode (triggers SaveConfig before reset)
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);

    // Verify defaults are active in SIMPLE mode
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));

    // Step 3: Switch back to CUSTOM mode (reloads from JSON)
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // Verify custom mappings are restored
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
}

TEST(TransmitterJoyConCapture, ModeSwitch_EmptyCustom_PreservesDefaults)
{
    // Switch to CUSTOM with no modifications, then back to SIMPLE
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);

    // Defaults should be intact
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));
}

TEST(TransmitterJoyConCapture, ModeSwitch_PreservesCtrlType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);

    // Set JOYCON type and custom mappings (simulate capture)
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LB] = N3DS_KEY_INDEX_B;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_B] = N3DS_KEY_INDEX_INVALID;

    // Round trip
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // ctrlType and mappings preserved
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
}

// ---- Cross-controller mapping: Xbox setup → Joy-Con modify ----

TEST(TransmitterJoyConCapture, CrossController_XboxSetupThenJoyconModify)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);

    // === Phase 1: Set up with Xbox controller (HAT D-pad captures) ===
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_XBOX;

    // Simulate: capture Xbox D-pad UP for N3DS A
    // Clear existing defaults to avoid duplicates
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;

    // Simulate: capture Xbox D-pad DOWN for N3DS B
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_DOWN] = N3DS_KEY_INDEX_B;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_B] = N3DS_KEY_INDEX_INVALID;

    // Simulate: capture LB for N3DS X
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LB] = N3DS_KEY_INDEX_X;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_X] = N3DS_KEY_INDEX_INVALID;

    // Simulate: capture LT for N3DS Y
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LT] = N3DS_KEY_INDEX_Y;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_Y] = N3DS_KEY_INDEX_INVALID;

    // Persist via mode switch
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // Verify Xbox mappings restored after round trip
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);

    // Verify D-pad display names use +arrow format (via gInputKeyTab)
    STRCMP_EQUAL("+↑", gInputKeyTab[INPUT_KEY_INDEX_UP].name);
    STRCMP_EQUAL("+↓", gInputKeyTab[INPUT_KEY_INDEX_DOWN].name);
    STRCMP_EQUAL("+←", gInputKeyTab[INPUT_KEY_INDEX_LEFT].name);
    STRCMP_EQUAL("+→", gInputKeyTab[INPUT_KEY_INDEX_RIGHT].name);
    STRCMP_EQUAL("+↑", gInputKeyTab[INPUT_KEY_INDEX_JCL_UP].name);
    STRCMP_EQUAL("+↓", gInputKeyTab[INPUT_KEY_INDEX_JCL_DOWN].name);
    STRCMP_EQUAL("+←", gInputKeyTab[INPUT_KEY_INDEX_JCL_LEFT].name);
    STRCMP_EQUAL("+→", gInputKeyTab[INPUT_KEY_INDEX_JCL_RIGHT].name);

    // === Phase 2: Modify with Joy-Con controller ===
    TransmitterTestAccess::ResetCaptureTracking();

    // Capture JCL_UP for N3DS L (auto-switches ctrlType to JOYCON, triggers conflict)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_JCL_UP;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // ctrlType switched to JOYCON, AdaptToCtrlType set JCL_UP→UP → conflict
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("+↑", gLastCaptureConflictN3dsName);

    // Accept conflict: JCL_UP → L, old L occupant gets UP
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));

    // Verify non-D-pad Xbox mappings preserved after Joy-Con AdaptToCtrlType
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));

    // Verify Xbox D-pad keys were cleared by AdaptToCtrlType(JOYCON)
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));

    // JCL D-pad defaults set by AdaptToCtrlType (except JCL_UP which was captured)
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_LEFT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_RIGHT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));
}

TEST(TransmitterJoyConCapture, CrossController_NoConflictBetweenTypes)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_XBOX;

    // Xbox D-pad UP → A
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;

    // Switch to JOYCON via JCL capture (triggers conflict: AdaptToCtrlType sets JCL_UP→UP)
    TransmitterTestAccess::ResetCaptureTracking();
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_B);
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_JCL_UP;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    // Conflict detected: JCL_UP already mapped to UP by AdaptToCtrlType
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("+↑", gLastCaptureConflictN3dsName);

    // Accept conflict: JCL_UP→B, old B occupant gets UP
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

// ---- LoadConfig / SetKeyMapMode applies D-pad defaults per ctrlType ----

TEST(TransmitterJoyConCapture, LoadConfig_AppliesJoyconDpadDefaults)
{
    // Simulate: capture JCL_UP→A in JOYCON mode (ctrlType detected, AdaptToCtrlType ran)
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);  // sets JCL defaults, clears Xbox D-pad
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;

    // Persist
    h.tr->SaveConfig();

    // Reload via SetKeyMapMode round trip
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // Custom JCL_UP→A preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Other JCL D-pad defaults applied by AdaptToCtrlType → UI shows +↓ +← +→
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_LEFT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_RIGHT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));
    // Xbox D-pad keys cleared by AdaptToCtrlType(JoyCon)
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));
    // D-pad display names are +arrow format (not N3DS key names)
    STRCMP_EQUAL("+↑", gInputKeyTab[INPUT_KEY_INDEX_JCL_UP].name);
    STRCMP_EQUAL("+↓", gInputKeyTab[INPUT_KEY_INDEX_JCL_DOWN].name);
}

TEST(TransmitterJoyConCapture, LoadConfig_AppliesXboxDpadDefaults)
{
    // Simulate: capture keyCode UP→A in XBOX mode
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_XBOX;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_XBOX);  // sets Xbox defaults, clears JCL
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;

    h.tr->SaveConfig();
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // Custom UP→A preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    // Other Xbox D-pad defaults applied by AdaptToCtrlType
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_LEFT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_RIGHT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RIGHT));
    // JCL keys cleared by AdaptToCtrlType(XBOX)
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    // Xbox D-pad display names
    STRCMP_EQUAL("+↑", gInputKeyTab[INPUT_KEY_INDEX_UP].name);
    STRCMP_EQUAL("+↓", gInputKeyTab[INPUT_KEY_INDEX_DOWN].name);
}

TEST(TransmitterJoyConCapture, LoadConfig_DestroyRecreate_PreservesDpadDefaults)
{
    // Save JOYCON config (AdaptToCtrlType already ran during capture)
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;
    h.tr->SaveConfig();

    // Destroy + recreate (LoadConfig path → ParseJsonConfig calls AdaptToCtrlType)
    h.tr->DestroyInstance();
    TransmitterTestAccess::ResetInstance();
    Transmitter::CreateInstance(&h.stubApp);
    h.tr = Transmitter::GetInstance();
    cfg = TransmitterTestAccess::GetConfig(h.tr);

    // ctrlType restored from JSON
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    // JCL D-pad defaults applied (AdaptToCtrlType was called by ParseJsonConfig)
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_LEFT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_RIGHT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));
    // Custom JCL_UP→A overrides default (keyMappings applied after AdaptToCtrlType)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
}

// ---- Cross-controller: Joy-Con setup → Xbox capture → duplicate +↓ display ----

TEST(TransmitterJoyConCapture, JoyconSetup_ThenXboxDpadCapture_DuplicateDisplay)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // === Step 1: Joy-Con setup (JCL_UP→A, JCL_DOWN→B) ===
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);
    // Capture JCL_UP for A
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_A;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_INVALID;
    // Capture JCL_DOWN for B
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_B;
    cfg->gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_B] = N3DS_KEY_INDEX_INVALID;

    // Verify Joy-Con setup
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);

    // === Step 2: Capture Xbox D-pad DOWN via HandleMotionEvent for N3DS A ===
    TransmitterTestAccess::ResetCaptureTracking();
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);

    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 1.0f;  // DOWN
    h.tr->HandleMotionEvent(&ev);

    // After capture: HAT D-pad DOWN auto-switches ctrlType JOYCON→XBOX
    // AdaptToCtrlType(XBOX) clears JCL keys, sets Xbox D-pad defaults
    // Then capture: INPUT_KEY_INDEX_DOWN → A

    // === Verify fixed behavior ===
    // ctrlType switched to XBOX (HAT D-pad detected during capture)
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);

    // Xbox D-pad DOWN → A (only one "+↓" now, JCL_DOWN was cleared by AdaptToCtrlType)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));

    // JCL keys cleared by AdaptToCtrlType(XBOX)
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));

    // Xbox D-pad defaults restored (except DOWN which was captured)
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_LEFT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_RIGHT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RIGHT));

    // No duplicate "+↓": JCL_DOWN→INVALID, only INPUT_KEY_INDEX_DOWN→A shows "+↓"
}

// ---- Sequential Joy-Con capture: L3→L, LB→ZL, LT→L conflict ----

TEST(TransmitterJoyConCapture, SequentialCapture_L3L_LBZL_LTL_Conflict)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);
    // Assume ctrlType=XBOX (default) for Joy-Con non-D-pad keys

    // Step 1: Capture L3 for N3DS L (L3→L, displaces LB→L)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    h.pressKeyCode(GAMEPAD_BUTTON_L3);
    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_FALSE(gLastCaptureConflict);
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 2: Capture LB for N3DS ZL (LB→ZL, displaces LT→ZL)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_ZL);
    h.pressKeyCode(GAMEPAD_BUTTON_LB);
    CHECK_EQUAL(N3DS_KEY_INDEX_ZL, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    CHECK_FALSE(gLastCaptureConflict);
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 3: Capture LT for N3DS L — LT was displaced to INVALID in step 2
    // Should succeed without conflict, NOT show "ZL already mapped"
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_L);
    h.pressKeyCode(GAMEPAD_BUTTON_LT);
    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
    // Old occupant (L3→L) displaced to INVALID
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK_FALSE(gLastCaptureConflict);
}

TEST(TransmitterJoyConCapture, SequentialCapture_R3R_RBZR_RTR_Conflict)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // Step 1: Capture R3 for N3DS R (R3→R, displaces RB→R)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_R);
    h.pressKeyCode(GAMEPAD_BUTTON_R3);
    CHECK_EQUAL(N3DS_KEY_INDEX_R, h.tr->GetKeyMapping(INPUT_KEY_INDEX_R3));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RB));
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 2: Capture RB for N3DS ZR (RB→ZR, displaces RT→ZR)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_ZR);
    h.pressKeyCode(GAMEPAD_BUTTON_RB);
    CHECK_EQUAL(N3DS_KEY_INDEX_ZR, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RB));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RT));
    TransmitterTestAccess::ResetCaptureTracking();

    // Step 3: Capture RT for N3DS R — RT was displaced to INVALID in step 2
    // Should succeed without conflict, NOT show "ZR already mapped"
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_R);
    h.pressKeyCode(GAMEPAD_BUTTON_RT);
    CHECK_EQUAL(N3DS_KEY_INDEX_R, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RT));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_R3));
    CHECK_FALSE(gLastCaptureConflict);
}

// ---- Conflict dialog shows D-pad names as +↑ not UP ----

TEST(TransmitterJoyConCapture, ConflictDialog_UsesArrowNamesForDpad)
{
    // JCL_UP mapped to N3DS UP via AdaptToCtrlType, capture A → conflict
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_JCL_UP;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    CHECK(gLastCaptureConflict);
    // Bug: was "UP", should be "+↑"
    STRCMP_EQUAL("+↑", gLastCaptureConflictN3dsName);
}

TEST(TransmitterJoyConCapture, ConflictDialog_UsesArrowNamesForXboxDpad)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_XBOX;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_XBOX);

    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_DOWN;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    CHECK(gLastCaptureConflict);
    // Bug: was "DOWN", should be "+↓"
    STRCMP_EQUAL("+↓", gLastCaptureConflictN3dsName);
}

// ---- Special button capture: HOME → physical HOME key ----

TEST(TransmitterJoyConCapture, CaptureHome_ShowsCorrectPhysKeyName)
{
    // Capture N3DS HOME with Xbox Home button (keyCode 110)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_HOME);
    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_HOME;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // No conflict: HOME→HOME is default
    CHECK_FALSE(gLastCaptureConflict);
    // The phys key name should be "HOME" (gInputKeyTab[INPUT_KEY_INDEX_HOME])
    STRCMP_EQUAL("HOME", gInputKeyTab[INPUT_KEY_INDEX_HOME].name);
    // getKeyMapping(INPUT_KEY_INDEX_HOME) should return N3DS_KEY_INDEX_HOME
    CHECK_EQUAL(N3DS_KEY_INDEX_HOME, h.tr->GetKeyMapping(INPUT_KEY_INDEX_HOME));
}

TEST(TransmitterJoyConCapture, CapturePower_ShowsCorrectPhysKeyName)
{
    // Capture N3DS POWER with Xbox Share button (keyCode 130)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_POWER);
    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_SHARE;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    CHECK_FALSE(gLastCaptureConflict);
    STRCMP_EQUAL("SHARE", gInputKeyTab[INPUT_KEY_INDEX_SHARE].name);
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SHARE));
}

// ---- Joy-Con D-pad no response on default config (fresh install) ----

TEST(TransmitterJoyConCapture, DpadAutoDetect_OnKeyPress)
{
    // Default config after fresh install: ctrlType=XBOX, JCL D-pad → INVALID
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));

    // Press Joy-Con D-pad UP via scanCode 544 — should auto-detect JOYCON type
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_JCL_UP;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // ctrlType auto-switched to JOYCON, JCL D-pad defaults applied
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));

    // Frame should show UP output (N3DS BUTTON UP = offset 6, hidPad bit 6 cleared)
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_UP)) == 0);
}

TEST(TransmitterJoyConCapture, DpadAutoDetect_AllFourDirections)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);

    // Press JCL_UP → auto-detect JOYCON
    h.pressScanCode(GAMEPAD_BUTTON_JCL_UP);
    CHECK_EQUAL(CONTROLLER_TYPE_JOYCON, cfg->gamepadCfg.ctrlType);

    // All four JCL D-pad keys should now have correct N3DS direction mappings
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_DOWN, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_LEFT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_RIGHT, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));

    // Xbox D-pad keys cleared by AdaptToCtrlType(JOYCON)
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RIGHT));
}

TEST(TransmitterJoyConCapture, DpadAutoDetect_DownTriggersOutput)
{
    // Press JCL_DOWN on default config → auto-detect JOYCON, N3DS DOWN output
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_JCL_DOWN;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_DOWN)) == 0);
}

TEST(TransmitterJoyConCapture, DpadAutoDetect_NonDpadKeyDoesNotChangeType)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    // Default XBOX type
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);

    // Press L (LB keyCode) — not a D-pad key, ctrlType stays XBOX
    h.pressKeyCode(GAMEPAD_BUTTON_LB);
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);
}

TEST(TransmitterJoyConCapture, DpadAutoDetect_XboxDpadStaysXbox)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);

    // Press Xbox D-pad UP (keyCode=19) — ctrlType stays XBOX
    h.pressKeyCode(GAMEPAD_BUTTON_UP);
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);
    // Xbox D-pad UP still maps to N3DS UP
    CHECK_EQUAL(N3DS_KEY_INDEX_UP, h.tr->GetKeyMapping(INPUT_KEY_INDEX_UP));
}

// ---- SCRSHOT (Joy-Con Capture/Share) should conflict with POWER (special) ----

TEST(TransmitterJoyConCapture, ScrShotCapture_NoConflictInJoyconMode)
{
    // Bug: SCRSHOT defaults to INVALID even in JOYCON mode → no conflict on capture
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    // SCRSHOT now defaults to POWER for Joy-Con (like SHARE→POWER for Xbox)
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));

    // Capture A with SCRSHOT → should conflict with POWER (special key)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_SCRSHOT;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    // Conflict detected: SCRSHOT already mapped to POWER (special)
    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("POWER", gLastCaptureConflictN3dsName);
    // Mapping unchanged (waiting for conflict resolution)
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));
}

TEST(TransmitterJoyConCapture, ScrShotCapture_ConflictBlockedForSpecialKey)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    // Set up conflict state (simulating middleware between capture and resolve)
    TransmitterTestAccess::SetCaptureTarget(h.tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::SetConflictState(h.tr, INPUT_KEY_INDEX_SCRSHOT, N3DS_KEY_INDEX_POWER);

    // "Continue" is blocked for special keys — but ResolveKeyConflict still swaps
    h.tr->ResolveKeyConflict(true, 0);

    // SCRSHOT → A, old A occupant gets POWER
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterJoyConCapture, ScrShotCapture_ConflictInXboxMode)
{
    // XBOX mode: both SHARE and SCRSHOT map to POWER (special key)
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_XBOX;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_XBOX);

    // SCRSHOT stays POWER from SetDefaultKeyMapValue (AdaptToCtrlType no longer clears it)
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SHARE));

    // Capture A with SCRSHOT → conflict with POWER (special key)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_SCRSHOT;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;
    h.tr->HandleKeyEvent(&ev);

    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("POWER", gLastCaptureConflictN3dsName);
    // Mapping unchanged (waiting for conflict resolution)
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));
}

TEST(TransmitterJoyConCapture, ScrShotCapture_DefaultConfigPowerMapping)
{
    // Default config: SetDefaultKeyMapValue sets SCRSHOT→POWER regardless of ctrlType
    // AdaptToCtrlType no longer touches SCRSHOT, so POWER mapping persists
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    h.tr->SetDefaultConfigValue();

    // Simulate Joy-Con D-pad auto-detect
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));

    // Capture with SCRSHOT → conflict with POWER
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_B);
    h.pressScanCode(GAMEPAD_BUTTON_SCRSHOT);

    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("POWER", gLastCaptureConflictN3dsName);
}

// ---- Bug 1: Clear data → Custom mode → SCRSHOT still mappable ----

TEST(TransmitterJoyConCapture, ScrShotCapture_AfterClearDataThenCustomMode)
{
    // Simulate: clear data → fresh defaults (XBOX) → enter Custom mode
    h.tr->SetDefaultConfigValue();
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    CHECK_EQUAL(CONTROLLER_TYPE_XBOX, cfg->gamepadCfg.ctrlType);

    // SetDefaultKeyMapValue sets SCRSHOT→POWER
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));

    // Enter Custom mode: SetKeyMapMode(CUSTOM) calls AdaptToCtrlType(XBOX)
    // BUG: AdaptToCtrlType(XBOX) sets SCRSHOT→INVALID, clearing the POWER mapping
    h.tr->SetKeyMapMode(KEYMAP_MODE_CUSTOM);

    // After SetKeyMapMode(CUSTOM): SCRSHOT→POWER should be preserved
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));

    // Capture A with SCRSHOT → should conflict with POWER (special key)
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_SCRSHOT);

    CHECK_EQUAL(1, gCaptureResultCallCount);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("POWER", gLastCaptureConflictN3dsName);
}

// ---- Bug 2: After first capture maps SCRSHOT→A, second capture shows
//             conflict with A (non-special) instead of POWER (special) ----

TEST(TransmitterJoyConCapture, ScrShotCapture_SecondCaptureKeepsSpecialConflict)
{
    // After Bug 1 allowed first capture, SCRSHOT was mapped to A
    // Second capture should still conflict with POWER (not A)
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    cfg->gamepadCfg.ctrlType = CONTROLLER_TYPE_JOYCON;
    h.tr->AdaptToCtrlType(CONTROLLER_TYPE_JOYCON);

    // Verify SCRSHOT→POWER (special key)
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));

    // Capture A with SCRSHOT → conflict with POWER
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.pressScanCode(GAMEPAD_BUTTON_SCRSHOT);
    CHECK(gLastCaptureConflict);
    STRCMP_EQUAL("POWER", gLastCaptureConflictN3dsName);

    // Accept: SCRSHOT→A, A→POWER
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SCRSHOT));

    TransmitterTestAccess::ResetCaptureTracking();

    // Second capture: B with SCRSHOT → conflict with A (not special!)
    // BUG: should still conflict with POWER, but POWER is now at INPUT_KEY_INDEX_A
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_B);
    h.pressScanCode(GAMEPAD_BUTTON_SCRSHOT);
    CHECK(gLastCaptureConflict);
    // conflictN3dsName should be A (the key SCRSHOT was remapped to), not POWER
    STRCMP_EQUAL("A", gLastCaptureConflictN3dsName);
}
