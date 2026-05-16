#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include "cJSON.h"
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

TEST_GROUP(TransmitterIO)
{
    TransmitterTestHarness h;

    void setup() override { h.setUp(false); }
    void teardown() override { h.tearDown(); }

    std::string configPath() const {
        return std::string(h.tempDir) + "/config.json";
    }

    void writeFile(const std::string& path, const std::string& content) {
        FILE* f = fopen(path.c_str(), "w");
        CHECK(f != nullptr);
        fwrite(content.c_str(), 1, content.size(), f);
        fclose(f);
    }
};

// ---- SaveConfig creates JSON file ----
TEST(TransmitterIO, SaveConfig_CreatesJsonFile)
{
    h.tr->SetCfgIP("10.0.0.50");
    CHECK_EQUAL(Transmitter::OK, h.tr->SaveConfig());

    struct stat st;
    CHECK_EQUAL(0, stat(configPath().c_str(), &st));
    CHECK(st.st_size > 0);
}

// ---- SaveConfig round-trip ----
TEST(TransmitterIO, SaveConfig_RoundTrip)
{
    h.tr->SetCfgIP("172.16.0.1");
    h.tr->SetInvertAB(true);
    h.tr->SetTurboInterval(80);
    CHECK_EQUAL(Transmitter::OK, h.tr->SaveConfig());

    // Destroy and recreate to force LoadConfig
    h.tr->DestroyInstance();
    TransmitterTestAccess::ResetInstance();
    Transmitter::CreateInstance(&h.stubApp);
    h.tr = Transmitter::GetInstance();

    char buf[64] = {};
    h.tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("172.16.0.1", buf);
    CHECK(h.tr->GetInvertAB());
    CHECK_EQUAL(80, h.tr->GetTurboInterval());
}

// ---- LoadConfig: missing file uses defaults ----
TEST(TransmitterIO, LoadConfig_MissingFile_UsesDefaults)
{
    // No config file exists (fresh temp dir). Constructor already called LoadConfig.
    char buf[64] = {};
    h.tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("192.168.100.100", buf); // default IP
}

// ---- LoadConfig: invalid JSON falls back to defaults ----
TEST(TransmitterIO, LoadConfig_InvalidJson_FallsBack)
{
    writeFile(configPath(), "not valid json {{{");

    h.tr->DestroyInstance();
    TransmitterTestAccess::ResetInstance();
    Transmitter::CreateInstance(&h.stubApp);
    h.tr = Transmitter::GetInstance();

    char buf[64] = {};
    h.tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("192.168.100.100", buf); // defaults used
}

// ---- SaveConfig: verify JSON content structure ----
TEST(TransmitterIO, SaveConfig_JsonContent)
{
    h.tr->SetCfgIP("192.168.1.1");
    h.tr->SetInvertAB(true);
    h.tr->SetInvertXY(false);
    h.tr->SetSwapJoysticks(true);
    h.tr->SetHomeMap(true);
    h.tr->SetPowerMap(false);
    h.tr->SetPowerOffMap(true);
    h.tr->SetTurboInterval(50);
    CHECK_EQUAL(Transmitter::OK, h.tr->SaveConfig());

    // Read back and parse
    FILE* f = fopen(configPath().c_str(), "r");
    CHECK(f != nullptr);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* content = (char*)malloc(sz + 1);
    fread(content, 1, sz, f);
    fclose(f);
    content[sz] = '\0';

    cJSON* root = cJSON_Parse(content);
    CHECK(root != nullptr);

    cJSON* ip = cJSON_GetObjectItem(root, "ip");
    CHECK(cJSON_IsString(ip));
    STRCMP_EQUAL("192.168.1.1", ip->valuestring);

    cJSON* iab = cJSON_GetObjectItem(root, "invertAB");
    CHECK(cJSON_IsTrue(iab));

    cJSON* swj = cJSON_GetObjectItem(root, "swapJoysticks");
    CHECK(cJSON_IsTrue(swj));

    cJSON* ti = cJSON_GetObjectItem(root, "turboIntervalMs");
    CHECK_EQUAL(50, ti->valueint);

    cJSON_Delete(root);
    free(content);
}
