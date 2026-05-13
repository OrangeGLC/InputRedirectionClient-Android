#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

static struct android_app gStubApp;

TEST_GROUP(TransmitterFrame)
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

// ---- GenerateFrame neutral ----
TEST(TransmitterFrame, GenerateFrame_Neutral)
{
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
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
    pressKey(GAMEPAD_BUTTON_A, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // N3DS BUTTON A = offset 0, FIRST obj -> hidPad bit 0 cleared
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_A)) == 0);
}

// ---- KeyEventToFrameData: B press ----
TEST(TransmitterFrame, KeyEventToFrameData_BPress)
{
    pressKey(GAMEPAD_BUTTON_B, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK((fd->hidPad & (1 << N3DS_BUTTON_B)) == 0);
}

// ---- KeyEventToFrameData: key release ----
TEST(TransmitterFrame, KeyEventToFrameData_Release)
{
    pressKey(GAMEPAD_BUTTON_A, 0);
    releaseKey(GAMEPAD_BUTTON_A, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
}

// ---- KeyEventToFrameData: ZL (FOURTH obj) ----
TEST(TransmitterFrame, KeyEventToFrameData_ZLPress)
{
    pressKey(GAMEPAD_BUTTON_LT, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // ZL offset=2, FOURTH obj -> irButtonsState bit 2 set
    CHECK((fd->irButtonsState & (1 << N3DS_BUTTON_ZL)) != 0);
}

// ---- KeyEventToFrameData: ZR (FOURTH obj) ----
TEST(TransmitterFrame, KeyEventToFrameData_ZRPress)
{
    pressKey(GAMEPAD_BUTTON_RT, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK((fd->irButtonsState & (1 << N3DS_BUTTON_ZR)) != 0);
}

// ---- KeyEventToFrameData: HOME (FIFTH obj, when mapped) ----
TEST(TransmitterFrame, KeyEventToFrameData_HomePress)
{
    tr->SetHomeMap(true);
    tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);
    pressKey(GAMEPAD_BUTTON_HOME, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK((fd->interfaceButtons & (1 << N3DS_BUTTON_HOME)) != 0);
}

// ---- KeyEventToFrameData: HOME not mapped ----
TEST(TransmitterFrame, KeyEventToFrameData_HomeNotMapped)
{
    tr->SetHomeMap(false);
    tr->SetKeyMapping(INPUT_KEY_INDEX_HOME, N3DS_KEY_INDEX_HOME);
    pressKey(GAMEPAD_BUTTON_HOME, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(0, fd->interfaceButtons);
}

// ---- KeyEventToFrameData: un-mapped key (N3DS_KEY_INDEX_INVALID) ----
TEST(TransmitterFrame, KeyEventToFrameData_UnmappedKey)
{
    // L3 is unmapped by default (N3DS_KEY_INDEX_INVALID)
    pressKey(GAMEPAD_BUTTON_L3, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    CHECK_EQUAL(HID_PAD_NEUTRAL, fd->hidPad);
}

// ---- KeyEventToFrameData: L3+R3 combo = shutdown ----
TEST(TransmitterFrame, KeyEventToFrameData_PowerOffChord)
{
    tr->SetPowerOffMap(true);
    tr->SetKeyMapping(INPUT_KEY_INDEX_L3, N3DS_KEY_INDEX_SHUTDOWN);
    tr->SetKeyMapping(INPUT_KEY_INDEX_R3, N3DS_KEY_INDEX_SHUTDOWN);

    pressKey(GAMEPAD_BUTTON_L3, 0);
    pressKey(GAMEPAD_BUTTON_R3, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    // SHUTDOWN offset=2, FIFTH obj -> interfaceButtons bit 2 set
    CHECK((fd->interfaceButtons & (1 << N3DS_BUTTON_SHUTDOWN)) != 0);
}

// ---- KeyEventToFrameData: Multiple keys ----
TEST(TransmitterFrame, KeyEventToFrameData_MultipleKeys)
{
    pressKey(GAMEPAD_BUTTON_A, 0);
    pressKey(GAMEPAD_BUTTON_X, 0);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
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
    tr->HandleMotionEvent(&ev);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
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
    tr->HandleMotionEvent(&ev);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
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
    tr->HandleMotionEvent(&ev);
    tr->GenerateFrame();
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
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
    tr->HandleMotionEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_RIGHT]);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_LEFT]);
}

TEST(TransmitterFrame, MotionEventToFrameData_DPad_Up)
{
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_JOYSTICK;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 0.0f;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = -1.0f;
    tr->HandleMotionEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_UP]);
}

TEST(TransmitterFrame, MotionEventToFrameData_DPad_Centered)
{
    // First press a direction
    GameActivityMotionEvent evOn = {};
    evOn.source = AINPUT_SOURCE_JOYSTICK;
    evOn.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 1.0f;
    evOn.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    tr->HandleMotionEvent(&evOn);

    // Then release
    GameActivityMotionEvent evOff = {};
    evOff.source = AINPUT_SOURCE_JOYSTICK;
    evOff.pointers[0].values[AMOTION_EVENT_AXIS_HAT_X] = 0.0f;
    evOff.pointers[0].values[AMOTION_EVENT_AXIS_HAT_Y] = 0.0f;
    tr->HandleMotionEvent(&evOff);

    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_RIGHT]);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_LEFT]);
}

// ---- HandleKeyEvent: null event ----
TEST(TransmitterFrame, HandleKeyEvent_NullEvent)
{
    tr->HandleKeyEvent(nullptr);
    // Should not crash; no assertion needed
}

// ---- HandleKeyEvent: non-gamepad source ----
TEST(TransmitterFrame, HandleKeyEvent_NonGamepadSource)
{
    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_A;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = 0; // Not a gamepad
    tr->HandleKeyEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    CHECK_EQUAL(KEY_STATE_UP, keys[INPUT_KEY_INDEX_A]);
}

// ---- HandleMotionEvent: null event ----
TEST(TransmitterFrame, HandleMotionEvent_NullEvent)
{
    tr->HandleMotionEvent(nullptr);
    // Should not crash
}

// ---- HandleMotionEvent: non-joystick source ----
TEST(TransmitterFrame, HandleMotionEvent_NonJoystickSource)
{
    GameActivityMotionEvent ev = {};
    ev.source = 0;
    ev.pointers[0].values[AMOTION_EVENT_AXIS_X] = 1.0f;
    tr->HandleMotionEvent(&ev);
    // Joystick values should remain at neutral
    AxisValue* joy = TransmitterTestAccess::GetJoystick(tr);
    DOUBLES_EQUAL(0.0f, joy[JOYSTICK_L].x, 0.001f);
}

// ---- NeedTurbo ----
TEST(TransmitterFrame, NeedTurbo_NoKeysPressed)
{
    CHECK_FALSE(tr->NeedTurbo());
}

TEST(TransmitterFrame, NeedTurbo_WithTurboKeyDown)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    // Simulate A key pressed
    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);
    CHECK(tr->NeedTurbo());
}

TEST(TransmitterFrame, NeedTurbo_TurboDisabled_KeyDown)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, false);
    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_A, KEY_STATE_DOWN);
    CHECK_FALSE(tr->NeedTurbo());
}

TEST(TransmitterFrame, NeedTurbo_NonTurboKeyDown)
{
    // SELECT is not in turbo range (MAX_N3DS_KEY_TURBO_INDEX = 8)
    TransmitterTestAccess::SetKeyState(tr, INPUT_KEY_INDEX_SELECT, KEY_STATE_DOWN);
    CHECK_FALSE(tr->NeedTurbo());
}

// ---- GenerateFrame buffer content ----
TEST(TransmitterFrame, GenerateFrame_BufferSize)
{
    tr->GenerateFrame();
    char* buf = TransmitterTestAccess::GetFrameBuffer(tr);
    // First 4 bytes = hidPad (little-endian)
    FrameData* fd = TransmitterTestAccess::GetFrameData(tr);
    u32 bufHidPad;
    memcpy(&bufHidPad, buf, 4);
    CHECK_EQUAL(fd->hidPad, bufHidPad);
}
