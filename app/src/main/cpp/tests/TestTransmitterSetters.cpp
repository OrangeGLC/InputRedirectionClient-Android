#include "TestHelpers.h"
#include "CppUTest/TestHarness.h"
#include "Config.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

TEST_GROUP(TransmitterSetters)
{
    TransmitterTestHarness h;

    void setup() override { h.setUp(false); }
    void teardown() override { h.tearDown(); }
};

// ---- SetCfgIP / GetCfgIP ----
TEST(TransmitterSetters, SetAndGetIP)
{
    h.tr->SetCfgIP("10.0.0.1");
    char buf[64] = {};
    CHECK_EQUAL(Transmitter::OK, h.tr->GetCfgIP(buf, sizeof(buf)));
    STRCMP_EQUAL("10.0.0.1", buf);
}

TEST(TransmitterSetters, GetCfgIP_NullBuffer)
{
    CHECK_EQUAL(Transmitter::NOK, h.tr->GetCfgIP(nullptr, 64));
}

TEST(TransmitterSetters, GetCfgIP_ZeroSize)
{
    char buf[64];
    CHECK_EQUAL(Transmitter::NOK, h.tr->GetCfgIP(buf, 0));
}

TEST(TransmitterSetters, GetCfgIP_BufferTooSmall)
{
    h.tr->SetCfgIP("192.168.100.100");
    char buf[4];
    CHECK_EQUAL(Transmitter::NOK, h.tr->GetCfgIP(buf, sizeof(buf)));
}

TEST(TransmitterSetters, SetCfgIP_TruncatesLongString)
{
    char longIP[128];
    memset(longIP, 'x', sizeof(longIP));
    longIP[CONFIG_IP_MAX_LEN - 1] = '\0';
    longIP[CONFIG_IP_MAX_LEN - 2] = 'z';
    h.tr->SetCfgIP(longIP);
    char buf[CONFIG_IP_MAX_LEN];
    CHECK_EQUAL(Transmitter::OK, h.tr->GetCfgIP(buf, sizeof(buf)));
    CHECK_EQUAL('z', buf[CONFIG_IP_MAX_LEN - 2]);
    CHECK_EQUAL('\0', buf[CONFIG_IP_MAX_LEN - 1]);
}

// ---- SetInvertAB / GetInvertAB ----
TEST(TransmitterSetters, InvertAB_Enable)
{
    h.tr->SetInvertAB(true);
    CHECK(h.tr->GetInvertAB());
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

TEST(TransmitterSetters, InvertAB_Disable)
{
    h.tr->SetInvertAB(true);
    h.tr->SetInvertAB(false);
    CHECK_FALSE(h.tr->GetInvertAB());
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

// ---- SetInvertXY / GetInvertXY ----
TEST(TransmitterSetters, InvertXY_Enable)
{
    h.tr->SetInvertXY(true);
    CHECK(h.tr->GetInvertXY());
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_Y));
}

TEST(TransmitterSetters, InvertXY_Disable)
{
    h.tr->SetInvertXY(true);
    h.tr->SetInvertXY(false);
    CHECK_FALSE(h.tr->GetInvertXY());
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_X));
    CHECK_EQUAL(N3DS_KEY_INDEX_Y, h.tr->GetKeyMapping(INPUT_KEY_INDEX_Y));
}

// ---- SetSwapJoysticks / GetSwapJoysticks ----
TEST(TransmitterSetters, SwapJoysticks_Enable)
{
    h.tr->SetSwapJoysticks(true);
    CHECK(h.tr->GetSwapJoysticks());
}

TEST(TransmitterSetters, SwapJoysticks_Disable)
{
    h.tr->SetSwapJoysticks(true);
    h.tr->SetSwapJoysticks(false);
    CHECK_FALSE(h.tr->GetSwapJoysticks());
}

// ---- SetKeyMapMode / GetKeyMapMode ----
TEST(TransmitterSetters, KeyMapMode_SetSimple)
{
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    CHECK_EQUAL(KEYMAP_MODE_SIMPLE, h.tr->GetKeyMapMode());
}

TEST(TransmitterSetters, KeyMapMode_InvalidIgnored)
{
    h.tr->SetKeyMapMode(KEYMAP_MODE_SIMPLE);
    h.tr->SetKeyMapMode(99);
    CHECK_EQUAL(KEYMAP_MODE_SIMPLE, h.tr->GetKeyMapMode());
}

// ---- SetKeyMapping / GetKeyMapping ----
TEST(TransmitterSetters, KeyMapping_ValidRoundTrip)
{
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_A, N3DS_KEY_INDEX_X);
    CHECK_EQUAL(N3DS_KEY_INDEX_X, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, KeyMapping_InvalidInputIgnored)
{
    int before = h.tr->GetKeyMapping(INPUT_KEY_INDEX_A);
    h.tr->SetKeyMapping(MAX_INPUT_KEY_INDEX + 5, N3DS_KEY_INDEX_A);
    CHECK_EQUAL(before, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, KeyMapping_NegativeInputIgnored)
{
    int before = h.tr->GetKeyMapping(INPUT_KEY_INDEX_A);
    h.tr->SetKeyMapping(-1, N3DS_KEY_INDEX_A);
    CHECK_EQUAL(before, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, KeyMapping_InvalidTargetIgnored)
{
    int before = h.tr->GetKeyMapping(INPUT_KEY_INDEX_A);
    h.tr->SetKeyMapping(INPUT_KEY_INDEX_A, MAX_N3DS_KEY_INDEX + 5);
    CHECK_EQUAL(before, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterSetters, GetKeyMapping_InvalidInputReturnsInvalid)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(MAX_INPUT_KEY_INDEX + 5));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(-1));
}

// ---- SetHomeMap / GetHomeMap ----
TEST(TransmitterSetters, HomeMap_Enable)
{
    h.tr->SetHomeMap(true);
    CHECK(h.tr->GetHomeMap());
}

TEST(TransmitterSetters, HomeMap_Disable)
{
    h.tr->SetHomeMap(true);
    h.tr->SetHomeMap(false);
    CHECK_FALSE(h.tr->GetHomeMap());
}

// ---- SetPowerMap / GetPowerMap ----
TEST(TransmitterSetters, PowerMap_Enable)
{
    h.tr->SetPowerMap(true);
    CHECK(h.tr->GetPowerMap());
}

TEST(TransmitterSetters, PowerMap_Disable)
{
    h.tr->SetPowerMap(true);
    h.tr->SetPowerMap(false);
    CHECK_FALSE(h.tr->GetPowerMap());
}

// ---- SetPowerOffMap / GetPowerOffMap ----
TEST(TransmitterSetters, PowerOffMap_Enable)
{
    h.tr->SetPowerOffMap(true);
    CHECK(h.tr->GetPowerOffMap());
}

TEST(TransmitterSetters, PowerOffMap_Disable)
{
    h.tr->SetPowerOffMap(true);
    h.tr->SetPowerOffMap(false);
    CHECK_FALSE(h.tr->GetPowerOffMap());
}

// ---- SetTurbo / GetTurbo (for supported key) ----
TEST(TransmitterSetters, Turbo_EnableDisable)
{
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, true);
    CHECK(h.tr->GetTurbo(N3DS_KEY_INDEX_A));
    h.tr->SetTurbo(N3DS_KEY_INDEX_A, false);
    CHECK_FALSE(h.tr->GetTurbo(N3DS_KEY_INDEX_A));
}

// ---- SetTurboInterval / GetTurboInterval ----
TEST(TransmitterSetters, TurboInterval_RoundTrip)
{
    h.tr->SetTurboInterval(100);
    CHECK_EQUAL(100, h.tr->GetTurboInterval());
    h.tr->SetTurboInterval(30);
    CHECK_EQUAL(30, h.tr->GetTurboInterval());
}

// ---- SetTurboMode / GetTurboMode ----
TEST(TransmitterSetters, TurboMode_SemiFull)
{
    h.tr->SetTurboMode(N3DS_KEY_INDEX_A, false);
    CHECK_FALSE(h.tr->GetTurboMode(N3DS_KEY_INDEX_A));
    h.tr->SetTurboMode(N3DS_KEY_INDEX_A, true);
    CHECK(h.tr->GetTurboMode(N3DS_KEY_INDEX_A));
}

TEST(TransmitterSetters, TurboMode_InvalidIndex)
{
    CHECK_FALSE(h.tr->GetTurboMode(-1));
    CHECK_FALSE(h.tr->GetTurboMode(MAX_N3DS_KEY_TURBO_INDEX));
}
