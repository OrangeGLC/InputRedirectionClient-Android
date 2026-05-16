#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

TEST_GROUP(TransmitterKeyMapping)
{
    TransmitterTestHarness h;

    void setup() override { h.setUp(false); }
    void teardown() override { h.tearDown(); }
};

// ---- GetOutputKeyIndex ----
TEST(TransmitterKeyMapping, GetOutputKeyIndex_Default_A)
{
    // Uses the public inline method via HandleKeyEvent path
    h.tr->SetDefaultConfigValue();
    h.tr->SetDefaultKeyMapValue();
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterKeyMapping, GetOutputKeyIndex_AfterSetKeyMapping)
{
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_X);
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
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
    h.tr->HandleKeyEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_A]);
}

TEST(TransmitterKeyMapping, GetInputKeyIndex_ByScanCode_ScrShot)
{
    GameActivityKeyEvent ev = {};
    ev.keyCode = 0;
    ev.scanCode = GAMEPAD_BUTTON_SCRSHOT;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;

    h.tr->HandleKeyEvent(&ev);
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    CHECK_EQUAL(KEY_STATE_DOWN, keys[INPUT_KEY_INDEX_SCRSHOT]);
}

TEST(TransmitterKeyMapping, GetInputKeyIndex_UnknownKey)
{
    GameActivityKeyEvent ev = {};
    ev.keyCode = 9999;
    ev.scanCode = 9999;
    ev.action = AKEY_EVENT_ACTION_DOWN;
    ev.source = AINPUT_SOURCE_GAMEPAD;

    h.tr->HandleKeyEvent(&ev);
    // No key state should have changed for any mapped key
    KEY_STATE* keys = TransmitterTestAccess::GetKeysState(h.tr);
    for (int i = 0; i < MAX_INPUT_KEY_INDEX; ++i) {
        CHECK_EQUAL(KEY_STATE_UP, keys[i]);
    }
}

// ---- EnterKeyCapture / ExitKeyCapture ----
TEST(TransmitterKeyMapping, EnterKeyCapture_ValidIndex)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(h.tr));
}

TEST(TransmitterKeyMapping, EnterKeyCapture_InvalidIndex)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.tr->EnterKeyCapture(MAX_N3DS_KEY_INDEX + 5); // out of range
    // Should retain previous value (out-of-range ignored)
    CHECK_EQUAL(N3DS_KEY_INDEX_A, TransmitterTestAccess::GetCaptureTarget(h.tr));
}

TEST(TransmitterKeyMapping, EnterKeyCapture_NegativeIndex)
{
    h.tr->EnterKeyCapture(-1);
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
}

TEST(TransmitterKeyMapping, ExitKeyCapture_ClearsState)
{
    h.tr->EnterKeyCapture(N3DS_KEY_INDEX_A);
    h.tr->ExitKeyCapture();
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
    CHECK_EQUAL(INPUT_KEY_INDEX_INVALID, TransmitterTestAccess::GetConflictInput(h.tr));
}

// ---- ResolveKeyConflict ----
TEST(TransmitterKeyMapping, ResolveConflict_Accept_SwapsMappings)
{
    // Set up: capture target = A, conflict: physical X already mapped to B
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    TransmitterTestAccess::SetCaptureTarget(h.tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::SetConflictState(h.tr, INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    h.tr->ResolveKeyConflict(true, 0);

    // After accept: X -> A, the old occupant of A gets B
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // Capture mode exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
}

TEST(TransmitterKeyMapping, ResolveConflict_Reject_NoMappingChange)
{
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    TransmitterTestAccess::SetCaptureTarget(h.tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::SetConflictState(h.tr, INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);

    h.tr->ResolveKeyConflict(false, 0);

    // Mapping should NOT change
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    // Capture mode exited
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, TransmitterTestAccess::GetCaptureTarget(h.tr));
}

TEST(TransmitterKeyMapping, ResolveConflict_NoOpWhenNotCapturing)
{
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);
    int before = h.tr->GetKeyMapping(INPUT_KEY_INDEX_X);
    // No capture target set
    TransmitterTestAccess::SetConflictState(h.tr, INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(before, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
}

TEST(TransmitterKeyMapping, ResolveConflict_NoOpWhenNoConflict)
{
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_X, N3DS_KEY_INDEX_B);
    int before = h.tr->GetKeyMapping(INPUT_KEY_INDEX_X);
    // Capture target set but no conflict
    TransmitterTestAccess::SetCaptureTarget(h.tr, N3DS_KEY_INDEX_A);
    TransmitterTestAccess::ClearConflictState(h.tr);
    h.tr->ResolveKeyConflict(true, 0);
    CHECK_EQUAL(before, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
}
