#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

TEST_GROUP(TransmitterFrame)
{
    TransmitterTestHarness h;

    void setup() override { h.setUp(); }
    void teardown() override { h.tearDown(); }
};

// ---- GenerateFrame neutral ----
TEST(TransmitterFrame, GenerateFrame_Neutral)
{
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
    CHECK_EQUAL(CIRCLE_PAD_NEUTRAL, fd->circlePadState);
    CHECK_EQUAL(CPP_STATE_NEUTRAL, fd->cppState);
    CHECK_EQUAL(0, fd->irButtonsState);
    CHECK_EQUAL(0, fd->interfaceButtons);
    CHECK_EQUAL(TOUCH_SCREEN_NEUTRAL, fd->touchScreenState);
}

// ---- KeyEventToFrameData: A press ----
TEST(TransmitterFrame, KeyEventToFrameData_APress)
{
    h.pressKey(GAMEPAD_BUTTON_A, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // N3DS BUTTON A = offset 0, FIRST obj -> hidPad bit 0 cleared
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
}

// ---- KeyEventToFrameData: B press ----
TEST(TransmitterFrame, KeyEventToFrameData_BPress)
{
    h.pressKey(GAMEPAD_BUTTON_B, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_B)) == 0);
}

// ---- KeyEventToFrameData: key release ----
TEST(TransmitterFrame, KeyEventToFrameData_Release)
{
    h.pressKey(GAMEPAD_BUTTON_A, 0);
    h.releaseKey(GAMEPAD_BUTTON_A, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
}

// ---- KeyEventToFrameData: ZL (FOURTH obj) ----
TEST(TransmitterFrame, KeyEventToFrameData_ZLPress)
{
    h.pressKey(GAMEPAD_BUTTON_LT, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // ZL offset=2, FOURTH obj -> irButtonsState bit 2 set
    CHECK((fd->irButtonsState & (1 << N3DS_BUTTON_ZL)) != 0);
}

// ---- KeyEventToFrameData: ZR (FOURTH obj) ----
TEST(TransmitterFrame, KeyEventToFrameData_ZRPress)
{
    h.pressKey(GAMEPAD_BUTTON_RT, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->irButtonsState & (1 << N3DS_BUTTON_ZR)) != 0);
}

// ---- KeyEventToFrameData: HOME (FIFTH obj, when mapped) ----
TEST(TransmitterFrame, KeyEventToFrameData_HomePress)
{
    h.tr->SetHomeMap(true);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);
    h.pressKey(GAMEPAD_BUTTON_HOME, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK((fd->interfaceButtons & (1 << N3DS_BUTTON_HOME)) != 0);
}

// ---- KeyEventToFrameData: HOME not mapped ----
TEST(TransmitterFrame, KeyEventToFrameData_HomeNotMapped)
{
    h.tr->SetHomeMap(false);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);
    h.pressKey(GAMEPAD_BUTTON_HOME, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(0, fd->interfaceButtons);
}

// ---- KeyEventToFrameData: un-mapped key (N3DS_KEY_INDEX_INVALID) ----
TEST(TransmitterFrame, KeyEventToFrameData_UnmappedKey)
{
    // L3 is unmapped by default (N3DS_KEY_INDEX_INVALID)
    h.pressKey(GAMEPAD_BUTTON_L3, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
}

// ---- KeyEventToFrameData: L3+R3 combo = shutdown ----
TEST(TransmitterFrame, KeyEventToFrameData_PowerOffChord)
{
    h.tr->SetPowerOffMap(true);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_L3, N3DS_KEY_INDEX_SHUTDOWN);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_R3, N3DS_KEY_INDEX_SHUTDOWN);

    h.pressKey(GAMEPAD_BUTTON_L3, 0);
    h.pressKey(GAMEPAD_BUTTON_R3, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // SHUTDOWN offset=2, FIFTH obj -> interfaceButtons bit 2 set
    CHECK((fd->interfaceButtons & (1 << N3DS_BUTTON_SHUTDOWN)) != 0);
}

// ---- KeyEventToFrameData: Multiple keys ----
TEST(TransmitterFrame, KeyEventToFrameData_MultipleKeys)
{
    h.pressKey(GAMEPAD_BUTTON_A, 0);
    h.pressKey(GAMEPAD_BUTTON_X, 0);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // Both A (bit 0) and X (bit 10) cleared in hidPad
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_X)) == 0);
}

// ---- MotionEventToFrameData: Left stick ----
TEST(TransmitterFrame, MotionEventToFrameData_LeftStick)
{
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 0.5f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Y] = -0.3f;
    h.tr->HandleMotionEvent(&ev);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // circlePadState should NOT be neutral when stick is moved
    CHECK(fd->circlePadState != CIRCLE_PAD_NEUTRAL);
}

// ---- MotionEventToFrameData: Right stick (C-stick with 45-degree rotation) ----
TEST(TransmitterFrame, MotionEventToFrameData_RightStick)
{
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    // Zero out Z/RZ (left stick defaults for Xbox), use RX/RY for right
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Z] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RZ] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RX] = 1.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RY] = 0.0f;
    h.tr->HandleMotionEvent(&ev);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // cppState should NOT be neutral when C-stick is moved
    CHECK(fd->cppState != CPP_STATE_NEUTRAL);
}

// ---- MotionEventToFrameData: Dead zone ----
TEST(TransmitterFrame, MotionEventToFrameData_DeadZone)
{
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    // Values below dead zone threshold (0.05f)
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 0.02f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Y] = -0.02f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_Z] = 0.03f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_RZ] = -0.03f;
    h.tr->HandleMotionEvent(&ev);
    h.tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    // Both sticks should be neutral (dead zone applied)
    CHECK_EQUAL(CIRCLE_PAD_NEUTRAL, fd->circlePadState);
    CHECK_EQUAL(CPP_STATE_NEUTRAL, fd->cppState);
}

// ---- MotionEventToFrameData: D-pad HAT ----
TEST(TransmitterFrame, MotionEventToFrameData_DPad_Right)
{
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 1.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    h.tr->HandleMotionEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_RIGHT]);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_LEFT]);
}

TEST(TransmitterFrame, MotionEventToFrameData_DPad_Up)
{
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = -1.0f;
    h.tr->HandleMotionEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_UP]);
}

TEST(TransmitterFrame, MotionEventToFrameData_DPad_Centered)
{
    // First press a direction
    GameActivityMotionEvent evOn = {};
    evOn.source = AINPUT_SOURCE_JOYSTICK;
    evOn.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 1.0f;
    evOn.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    h.tr->HandleMotionEvent(&evOn);

    // Then release
    GameActivityMotionEvent evOff = {};
    evOff.source = AINPUT_SOURCE_JOYSTICK;
    evOff.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 0.0f;
    evOff.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    h.tr->HandleMotionEvent(&evOff);

    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_RIGHT]);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_LEFT]);
}

// ---- HandleKeyEvent: null event ----
TEST(TransmitterFrame, HandleKeyEvent_NullEvent)
{
    h.tr->HandleKeyEvent(nullptr);
    // Should not crash; no assertion needed
}

// ---- HandleKeyEvent: non-gamepad source ----
TEST(TransmitterFrame, HandleKeyEvent_NonGamepadSource)
{
    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_A;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = 0; // Not a gamepad
    h.tr->HandleKeyEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_A]);
}

// ---- HandleMotionEvent: null event ----
TEST(TransmitterFrame, HandleMotionEvent_NullEvent)
{
    h.tr->HandleMotionEvent(nullptr);
    // Should not crash
}

// ---- HandleMotionEvent: non-joystick source ----
TEST(TransmitterFrame, HandleMotionEvent_NonJoystickSource)
{
    GameActivityMotionEvent ev = {};
    ev.source = 0;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 1.0f;
    h.tr->HandleMotionEvent(&ev);
    // Joystick values should remain at neutral
    AxisValue* joy = TransmitterTestAccess::GetJoystick(h.tr);
    DOUBLES_EQUAL(0.0f, joy[JOYSTICK_L].x, 0.001f);
}

// ---- NeedTurbo ----
TEST(TransmitterFrame, NeedTurbo_NoKeysPressed)
{
    CHECK_FALSE(h.tr->NeedTurbo());
}

TEST(TransmitterFrame, NeedTurbo_WithTurboKeyDown)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    // Simulate A key pressed
    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);
    CHECK(h.tr->NeedTurbo());
}

TEST(TransmitterFrame, NeedTurbo_TurboDisabled_KeyDown)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, false);
    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);
    CHECK_FALSE(h.tr->NeedTurbo());
}

TEST(TransmitterFrame, NeedTurbo_NonTurboKeyDown)
{
    // SELECT is not in turbo range (MAX_N3DS_KEY_TURBO_INDEX = 8)
    TransmitterTestAccess::SetKeyState(h.tr, INPUT_KEY_INDEX_SELECT, KEY_STATE_DOWN);
    CHECK_FALSE(h.tr->NeedTurbo());
}

// ---- GenerateFrame buffer content ----
TEST(TransmitterFrame, GenerateFrame_BufferSize)
{
    h.tr->GenerateFrame();
    char* buf = TransmitterTestAccess::GetFrameBuffer(h.tr);
    // First 4 bytes = hidPad (little-endian)
    FrameData* fd = TransmitterTestAccess::GetFrameData(h.tr);
    u32 bufHidPad;
    memcpy(&bufHidPad, buf, 4);
    CHECK_EQUAL(fd->hidPad, bufHidPad);
}
