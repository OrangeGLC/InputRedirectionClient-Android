#include "TestHelpers.h"
#include "CppUTest/TestHarness.h"
#include "Config.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

static struct android_app gStubApp;

TEST_GROUP(TransmitterSetters)
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

// ---- SetCfgIP / GetCfgIP ----
TEST(TransmitterSetters, SetAndGetIP)
{
    tr->SetCfgIP("10.0.0.1");
    char buf[64] = {};
    CHECK_EQUAL(Transmitter::OK, tr->GetCfgIP(buf, sizeof(buf)));
    STRCMP_EQUAL("10.0.0.1", buf);
}

TEST(TransmitterSetters, GetCfgIP_NullBuffer)
{
    CHECK_EQUAL(Transmitter::NOK, tr->GetCfgIP(nullptr, 64));
}

TEST(TransmitterSetters, GetCfgIP_ZeroSize)
{
    char buf[64];
    CHECK_EQUAL(Transmitter::NOK, tr->GetCfgIP(buf, 0));
}

TEST(TransmitterSetters, GetCfgIP_BufferTooSmall)
{
    tr->SetCfgIP("192.168.100.100");
    char buf[4];
    CHECK_EQUAL(Transmitter::NOK, tr->GetCfgIP(buf, sizeof(buf)));
}

TEST(TransmitterSetters, SetCfgIP_TruncatesLongString)
{
    char longIP[128];
    memset(longIP, 'x', sizeof(longIP));
    longIP[CONFIG_IP_MAX_LEN - 1] = '\0';
    longIP[CONFIG_IP_MAX_LEN - 2] = 'z';
    tr->SetCfgIP(longIP);
    char buf[CONFIG_IP_MAX_LEN];
    CHECK_EQUAL(Transmitter::OK, tr->GetCfgIP(buf, sizeof(buf)));
    CHECK_EQUAL('z', buf[CONFIG_IP_MAX_LEN - 2]);
    CHECK_EQUAL('\0', buf[CONFIG_IP_MAX_LEN - 1]);
}

// ---- SetInvertAB / GetInvertAB ----
TEST(TransmitterSetters, InvertAB_Enable)
{
    tr->SetInvertAB(true);
    CHECK(tr->GetInvertAB());
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

TEST(TransmitterSetters, InvertAB_Disable)
{
    tr->SetInvertAB(true);
    tr->SetInvertAB(false);
    CHECK_FALSE(tr->GetInvertAB());
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

// ---- SetInvertXY / GetInvertXY ----
TEST(TransmitterSetters, InvertXY_Enable)
{
    tr->SetInvertXY(true);
    CHECK(tr->GetInvertXY());
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_Y));
}

TEST(TransmitterSetters, InvertXY_Disable)
{
    tr->SetInvertXY(true);
    tr->SetInvertXY(false);
    CHECK_FALSE(tr->GetInvertXY());
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, tr->GetKeyMapping(INPUT_KEY_INDEX_Y));
}

// ---- SetSwapJoysticks / GetSwapJoysticks ----
TEST(TransmitterSetters, SwapJoysticks_Enable)
{
    tr->SetSwapJoysticks(true);
    CHECK(tr->GetSwapJoysticks());
}

TEST(TransmitterSetters, SwapJoysticks_Disable)
{
    tr->SetSwapJoysticks(true);
    tr->SetSwapJoysticks(false);
    CHECK_FALSE(tr->GetSwapJoysticks());
}

// ---- SetKeyMapMode / GetKeyMapMode ----
TEST(TransmitterSetters, KeyMapMode_SetSimple)
{
    tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    CHECK_EQUAL(KEYMAP_MODE_SIMPLE, tr->GetKeyMapMode());
}

TEST(TransmitterSetters, KeyMapMode_InvalidIgnored)
{
    tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    tr->SetKeyMapMode(99);
    CHECK_EQUAL(KEYMAP_MODE_SIMPLE, tr->GetKeyMapMode());
}

// ---- SetKeyMapping / GetKeyMapping ----
TEST(TransmitterSetters, KeyMapping_ValidRoundTrip)
{
    tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_X);
    CHECK_EQUAL(N3DS_KEY_INDEX_X, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, KeyMapping_InvalidInputIgnored)
{
    int before = tr->GetKeyMapping(INPUT_KEY_INDEX_A);
    tr->SetKeyMapping(MAX_INPUT_KEY_INDEX + 5, N3DS_KEY_INDEX_A);
    CHECK_EQUAL(before, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, KeyMapping_NegativeInputIgnored)
{
    int before = tr->GetKeyMapping(INPUT_KEY_INDEX_A);
    tr->SetKeyMapping(-1, N3DS_KEY_INDEX_A);
    CHECK_EQUAL(before, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, KeyMapping_InvalidTargetIgnored)
{
    int before = tr->GetKeyMapping(INPUT_KEY_INDEX_A);
    tr->SetKeyMapping(INPUT_KEY_INDEX_A, MAX_N3DS_KEY_INDEX + 5);
    CHECK_EQUAL(before, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, GetKeyMapping_InvalidInputReturnsInvalid)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(MAX_INPUT_KEY_INDEX + 5));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(-1));
}

// ---- SetHomeMap / GetHomeMap ----
TEST(TransmitterSetters, HomeMap_Enable)
{
    tr->SetHomeMap(true);
    CHECK(tr->GetHomeMap());
}

TEST(TransmitterSetters, HomeMap_Disable)
{
    tr->SetHomeMap(true);
    tr->SetHomeMap(false);
    CHECK_FALSE(tr->GetHomeMap());
}

// ---- SetPowerMap / GetPowerMap ----
TEST(TransmitterSetters, PowerMap_Enable)
{
    tr->SetPowerMap(true);
    CHECK(tr->GetPowerMap());
}

TEST(TransmitterSetters, PowerMap_Disable)
{
    tr->SetPowerMap(true);
    tr->SetPowerMap(false);
    CHECK_FALSE(tr->GetPowerMap());
}

// ---- SetPowerOffMap / GetPowerOffMap ----
TEST(TransmitterSetters, PowerOffMap_Enable)
{
    tr->SetPowerOffMap(true);
    CHECK(tr->GetPowerOffMap());
}

TEST(TransmitterSetters, PowerOffMap_Disable)
{
    tr->SetPowerOffMap(true);
    tr->SetPowerOffMap(false);
    CHECK_FALSE(tr->GetPowerOffMap());
}

// ---- SetTurbo / GetTurbo (for supported key) ----
TEST(TransmitterSetters, Turbo_EnableDisable)
{
    tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    CHECK(tr->GetTurbo(N3DS_KEY_INDEX_A));
    tr->SetTurbo(N3DS_KEY_INDEX_A, false);
    CHECK_FALSE(tr->GetTurbo(N3DS_KEY_INDEX_A));
}

// ---- SetTurboInterval / GetTurboInterval ----
TEST(TransmitterSetters, TurboInterval_RoundTrip)
{
    tr->SetTurboInterval(100);
    CHECK_EQUAL(100, tr->GetTurboInterval());
    tr->SetTurboInterval(30);
    CHECK_EQUAL(30, tr->GetTurboInterval());
}

// ---- SetTurboMode / GetTurboMode ----
TEST(TransmitterSetters, TurboMode_SemiFull)
{
    tr->SetTurboMode(N3DS_KEY_INDEX_A, false);
    CHECK_FALSE(tr->GetTurboMode(N3DS_KEY_INDEX_A));
    tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    CHECK(tr->GetTurboMode(N3DS_KEY_INDEX_A));
}

TEST(TransmitterSetters, TurboMode_InvalidIndex)
{
    CHECK_FALSE(tr->GetTurboMode(-1));
    CHECK_FALSE(tr->GetTurboMode(MAX_N3DS_KEY_TURBO_INDEX));
}
