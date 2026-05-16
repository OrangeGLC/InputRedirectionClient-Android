#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

static struct android_app gStubApp;

TEST_GROUP(TransmitterKeyMapping)
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
    }

    void teardown() override {
        tr->DestroyInstance();
        TransmitterTestAccess::ResetInstance();
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", tempDir);
        system(cmd);
    }
};

// ---- GetOutputKeyIndex ----
TEST(TransmitterKeyMapping, GetOutputKeyIndex_Default_A)
{
    // Uses the public inline method via HandleKeyEvent path
    tr->SetDefaultConfigValue();
    tr->SetDefaultKeyMapValue();
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterKeyMapping, GetOutputKeyIndex_AfterSetKeyMapping)
{
    tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_X);
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

// ---- GetInputKeyIndex ----
TEST(TransmitterKeyMapping, GetInputKeyIndex_ByKeyCode_A)
{
    GameActivityKeyEvent ev = {};
    ev.keyCode = GAMEPAD_BUTTON_A;
    ev.scanCode = 0;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;

    // We can't call private GetInputKeyIndex directly, but HandleKeyEvent uses it.
    // Verify indirectly: after HandleKeyEvent, key state for A should be DOWN.
    tr->HandleKeyEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_A]);
}

TEST(TransmitterKeyMapping, GetInputKeyIndex_ByScanCode_ScrShot)
{
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_SCRSHOT;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;

    tr->HandleKeyEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_SCRSHOT]);
}

TEST(TransmitterKeyMapping, GetInputKeyIndex_UnknownKey)
{
    GameActivityKeyEvent ev = {};
    ev.keyCode = 9999;
    ev.scanCode = 9999;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;

    tr->HandleKeyEvent(&ev);
    // No key state should have changed for any mapped key
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(tr);
    for (int i = 0; i < MAX_INPUT_KEY_INDEX; ++i) {
        CHECK_EQUAL(KEY_STATE_UP, keys[i]);
    }
}

// ---- EnterKeyCapture / ExitKeyCapture ----
TEST(TransmitterKeyMapping, EnterKeyCapture_ValidIndex)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(tr));
}

TEST(TransmitterKeyMapping, EnterKeyCapture_InvalidIndex)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    tr->EnterKeyCapture(MAX_N3DS_KEY_INDEX + 5); // out of range
    // Should retain previous value (out-of-range ignored)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(tr));
}

TEST(TransmitterKeyMapping, EnterKeyCapture_NegativeIndex)
{
    tr->EnterKeyCapture(-1);
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
}

TEST(TransmitterKeyMapping, ExitKeyCapture_ClearsState)
{
    tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    tr->ExitKeyCapture();
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
    CHECK_EQUAL(INPUT_KEY_INDEX_INVALID, TransmitterTestAccess::GetConflictInput(tr));
}

// ---- ResolveKeyConflict ----
TEST(TransmitterKeyMapping, ResolveConflict_Accept_SwapsMappings)
{
    // Set up: capture target = A, conflict: physical X already mapped to B
    tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    TransmitterTestAccess::SetCaptureTarget(tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::SetConflictState(tr, INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    tr->ResolveKeyConflict(true, 0);

    // After accept: X -> A, the old occupant of A gets B
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // Capture mode exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
}

TEST(TransmitterKeyMapping, ResolveConflict_Reject_NoMappingChange)
{
    tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    TransmitterTestAccess::SetCaptureTarget(tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::SetConflictState(tr, INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    tr->ResolveKeyConflict(false, 0);

    // Mapping should NOT change
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // Capture mode exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(tr));
}

TEST(TransmitterKeyMapping, ResolveConflict_NoOpWhenNotCapturing)
{
    tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);
    int before = tr->GetKeyMapping(INPUT_KEY_INDEX_X);
    // No capture target set
    TransmitterTestAccess::SetConflictState(tr, INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);
    tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(before, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
}

TEST(TransmitterKeyMapping, ResolveConflict_NoOpWhenNoConflict)
{
    tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);
    int before = tr->GetKeyMapping(INPUT_KEY_INDEX_X);
    // Capture target set but no conflict
    TransmitterTestAccess::SetCaptureTarget(tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::ClearConflictState(tr);
    tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(before, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
}
