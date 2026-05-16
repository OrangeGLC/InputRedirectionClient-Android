#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include "cJSON.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

TEST_GROUP(TransmitterConfig)
{
    TransmitterTestHarness h;

    void setup() override {
        h.setUp();
        // Overwrite defaults so we can test them cleanly
        h.tr->SetDefaultConfigValue();
    }

    void teardown() override { h.tearDown(); }
};

// ---- SetDefaultConfigValue ----
TEST(TransmitterConfig, DefaultIpAddress)
{
    char buf[64] = {};
    h.tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("192.168.100.100", buf);
}

TEST(TransmitterConfig, DefaultDeadZone)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    DOUBLES_EQUAL(0.05f, cfg->gamepadCfg.deadZone[JOYSTICK_L], 0.001f);
    DOUBLES_EQUAL(0.05f, cfg->gamepadCfg.deadZone[JOYSTICK_R], 0.001f);
}

TEST(TransmitterConfig, DefaultKeyMapMode)
{
    CHECK_EQUAL(KEYMAP_MODE_SIMPLE, h.tr->GetKeyMapMode());
}

TEST(TransmitterConfig, DefaultSwapJoysticks)
{
    CHECK_FALSE(h.tr->GetSwapJoysticks());
}

TEST(TransmitterConfig, DefaultInvertFlags)
{
    CHECK_FALSE(h.tr->GetInvertAB());
    CHECK_FALSE(h.tr->GetInvertXY());
}

TEST(TransmitterConfig, DefaultMapFlags)
{
    CHECK_FALSE(h.tr->GetHomeMap());
    CHECK_FALSE(h.tr->GetPowerMap());
    CHECK_FALSE(h.tr->GetPowerOffMap());
}

TEST(TransmitterConfig, DefaultTurboInterval)
{
    CHECK_EQUAL(DEFAULT_TURBO_INTERVAL, h.tr->GetTurboInterval());
}

TEST(TransmitterConfig, DefaultTurboMode)
{
    Config* cfg = TransmitterTestAccess::GetConfig(h.tr);
    for (int i = 0; i < MAX_N3DS_KEY_TURBO_INDEX; ++i) {
        CHECK_EQUAL(TURBO_MODE_SEMI, cfg->gamepadCfg.turboMode[i]);
    }
}

// ---- SetDefaultKeyMapValue ----
TEST(TransmitterConfig, DefaultKeyMapping_A)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_A, h.tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterConfig, DefaultKeyMapping_B)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_B, h.tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

TEST(TransmitterConfig, DefaultKeyMapping_HOME)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_HOME, h.tr->GetKeyMapping(INPUT_KEY_INDEX_HOME));
}

TEST(TransmitterConfig, DefaultKeyMapping_SHARE_mapsTo_POWER)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, h.tr->GetKeyMapping(INPUT_KEY_INDEX_SHARE));
}

TEST(TransmitterConfig, DefaultKeyMapping_LB_mapsTo_L)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_L, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
}

TEST(TransmitterConfig, DefaultKeyMapping_RB_mapsTo_R)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_R, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RB));
}

TEST(TransmitterConfig, DefaultKeyMapping_LT_mapsTo_ZL)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_ZL, h.tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
}

TEST(TransmitterConfig, DefaultKeyMapping_RT_mapsTo_ZR)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_ZR, h.tr->GetKeyMapping(INPUT_KEY_INDEX_RT));
}

TEST(TransmitterConfig, DefaultKeyMapping_L3_Unmapped)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
}

TEST(TransmitterConfig, DefaultKeyMapping_R3_Unmapped)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_R3));
}

TEST(TransmitterConfig, DefaultKeyMapping_JCL_AllUnmapped)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, h.tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));
}

// ---- ParseJsonConfig ----
TEST(TransmitterConfig, ParseJson_OnlyIp)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ip", "10.20.30.40");
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);

    char buf[64] = {};
    h.tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("10.20.30.40", buf);
}

TEST(TransmitterConfig, ParseJson_InvertAB)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "invertAB", 1);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(h.tr->GetInvertAB());
}

TEST(TransmitterConfig, ParseJson_InvertXY)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "invertXY", 1);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(h.tr->GetInvertXY());
}

TEST(TransmitterConfig, ParseJson_SwapJoysticks)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "swapJoysticks", 1);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(h.tr->GetSwapJoysticks());
}

TEST(TransmitterConfig, ParseJson_MapHome)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mapHome", 1);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(h.tr->GetHomeMap());
}

TEST(TransmitterConfig, ParseJson_MapPower)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mapPower", 1);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(h.tr->GetPowerMap());
}

TEST(TransmitterConfig, ParseJson_MapShut)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mapShut", 1);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(h.tr->GetPowerOffMap());
}

TEST(TransmitterConfig, ParseJson_TurboInterval)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "turboIntervalMs", 120);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK_EQUAL(120, h.tr->GetTurboInterval());
}

TEST(TransmitterConfig, ParseJson_TurboKeys)
{
    cJSON* root = cJSON_CreateObject();
    cJSON* turboKeys = cJSON_CreateObject();
    cJSON* keyA = cJSON_CreateObject();
    cJSON_AddBoolToObject(keyA, "enable", 1);
    cJSON_AddBoolToObject(keyA, "fullAuto", 1);
    cJSON_AddItemToObject(turboKeys, "A", keyA);
    cJSON_AddItemToObject(root, "turboKeys", turboKeys);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(h.tr->GetTurbo(N3DS_KEY_INDEX_A));
    CHECK(h.tr->GetTurboMode(N3DS_KEY_INDEX_A));
}

TEST(TransmitterConfig, ParseJson_KeyMapMode)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "keyMapMode", KEYMAP_MODE_CUSTOM);
    h.tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK_EQUAL(KEYMAP_MODE_CUSTOM, h.tr->GetKeyMapMode());
}
