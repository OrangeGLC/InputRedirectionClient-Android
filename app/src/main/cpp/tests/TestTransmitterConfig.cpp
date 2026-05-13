#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include "cJSON.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

static struct android_app gStubApp;

TEST_GROUP(TransmitterConfig)
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
        // Overwrite defaults so we can test them cleanly
        tr->SetDefaultConfigValue();
    }

    void teardown() override {
        tr->DestroyInstance();
        TransmitterTestAccess::ResetInstance();
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", tempDir);
        system(cmd);
    }
};

// ---- SetDefaultConfigValue ----
TEST(TransmitterConfig, DefaultIpAddress)
{
    char buf[64] = {};
    tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("192.168.100.100", buf);
}

TEST(TransmitterConfig, DefaultDeadZone)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    DOUBLES_EQUAL(0.05f, cfg->gamepadCfg.deadZone[JOYSTICK_L], 0.001f);
    DOUBLES_EQUAL(0.05f, cfg->gamepadCfg.deadZone[JOYSTICK_R], 0.001f);
}

TEST(TransmitterConfig, DefaultKeyMapMode)
{
    CHECK_EQUAL(KEYMAP_MODE_SIMPLE, tr->GetKeyMapMode());
}

TEST(TransmitterConfig, DefaultSwapJoysticks)
{
    CHECK_FALSE(tr->GetSwapJoysticks());
}

TEST(TransmitterConfig, DefaultInvertFlags)
{
    CHECK_FALSE(tr->GetInvertAB());
    CHECK_FALSE(tr->GetInvertXY());
}

TEST(TransmitterConfig, DefaultMapFlags)
{
    CHECK_FALSE(tr->GetHomeMap());
    CHECK_FALSE(tr->GetPowerMap());
    CHECK_FALSE(tr->GetPowerOffMap());
}

TEST(TransmitterConfig, DefaultTurboInterval)
{
    CHECK_EQUAL(DEFAULT_TURBO_INTERVAL, tr->GetTurboInterval());
}

TEST(TransmitterConfig, DefaultTurboMode)
{
    Config* cfg = TransmitterTestAccess::GetConfig(tr);
    for (int i = 0; i < MAX_N3DS_KEY_TURBO_INDEX; ++i) {
        CHECK_EQUAL(TURBO_MODE_SEMI, cfg->gamepadCfg.turboMode[i]);
    }
}

// ---- SetDefaultKeyMapValue ----
TEST(TransmitterConfig, DefaultKeyMapping_A)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_A, tr->GetKeyMapping(INPUT_KEY_INDEX_A));
}

TEST(TransmitterConfig, DefaultKeyMapping_B)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_B, tr->GetKeyMapping(INPUT_KEY_INDEX_B));
}

TEST(TransmitterConfig, DefaultKeyMapping_HOME)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_HOME, tr->GetKeyMapping(INPUT_KEY_INDEX_HOME));
}

TEST(TransmitterConfig, DefaultKeyMapping_SHARE_mapsTo_POWER)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_POWER, tr->GetKeyMapping(INPUT_KEY_INDEX_SHARE));
}

TEST(TransmitterConfig, DefaultKeyMapping_LB_mapsTo_L)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_L, tr->GetKeyMapping(INPUT_KEY_INDEX_LB));
}

TEST(TransmitterConfig, DefaultKeyMapping_RB_mapsTo_R)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_R, tr->GetKeyMapping(INPUT_KEY_INDEX_RB));
}

TEST(TransmitterConfig, DefaultKeyMapping_LT_mapsTo_ZL)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_ZL, tr->GetKeyMapping(INPUT_KEY_INDEX_LT));
}

TEST(TransmitterConfig, DefaultKeyMapping_RT_mapsTo_ZR)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_ZR, tr->GetKeyMapping(INPUT_KEY_INDEX_RT));
}

TEST(TransmitterConfig, DefaultKeyMapping_L3_Unmapped)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_L3));
}

TEST(TransmitterConfig, DefaultKeyMapping_R3_Unmapped)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_R3));
}

TEST(TransmitterConfig, DefaultKeyMapping_JCL_AllUnmapped)
{
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_UP));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_DOWN));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_LEFT));
    CHECK_EQUAL(N3DS_KEY_INDEX_INVALID, tr->GetKeyMapping(INPUT_KEY_INDEX_JCL_RIGHT));
}

// ---- ParseJsonConfig ----
TEST(TransmitterConfig, ParseJson_OnlyIp)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ip", "10.20.30.40");
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);

    char buf[64] = {};
    tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("10.20.30.40", buf);
}

TEST(TransmitterConfig, ParseJson_InvertAB)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "invertAB", 1);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(tr->GetInvertAB());
}

TEST(TransmitterConfig, ParseJson_InvertXY)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "invertXY", 1);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(tr->GetInvertXY());
}

TEST(TransmitterConfig, ParseJson_SwapJoysticks)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "swapJoysticks", 1);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(tr->GetSwapJoysticks());
}

TEST(TransmitterConfig, ParseJson_MapHome)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mapHome", 1);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(tr->GetHomeMap());
}

TEST(TransmitterConfig, ParseJson_MapPower)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mapPower", 1);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(tr->GetPowerMap());
}

TEST(TransmitterConfig, ParseJson_MapShut)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mapShut", 1);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(tr->GetPowerOffMap());
}

TEST(TransmitterConfig, ParseJson_TurboInterval)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "turboIntervalMs", 120);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK_EQUAL(120, tr->GetTurboInterval());
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
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK(tr->GetTurbo(N3DS_KEY_INDEX_A));
    CHECK(tr->GetTurboMode(N3DS_KEY_INDEX_A));
}

TEST(TransmitterConfig, ParseJson_KeyMapMode)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "keyMapMode", KEYMAP_MODE_CUSTOM);
    tr->ParseJsonConfig(root);
    cJSON_Delete(root);
    CHECK_EQUAL(KEYMAP_MODE_CUSTOM, tr->GetKeyMapMode());
}
