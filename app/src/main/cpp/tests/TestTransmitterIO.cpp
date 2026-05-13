#include "Transmitter.h"
#include "CppUTest/TestHarness.h"
#include "TestHelpers.h"
#include "cJSON.h"
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

static struct android_app gStubApp;

TEST_GROUP(TransmitterIO)
{
    Transmitter* tr;
    char tempDir[256];

    void setup() override {
        snprintf(tempDir, sizeof(tempDir), "/tmp/irc_utest_XXXXXX");
        char* dir = mkdtemp(tempDir);
        CHECK(dir != nullptr);

        memset(&gStubApp, 0, sizeof(gStubApp));
        gCfgPath = tempDir;
        gApp = &gStubApp;
        TransmitterTestAccess::ResetInstance();
        Transmitter::CreateInstance(&gStubApp);
        tr = Transmitter::GetInstance();
    }

    void teardown() override {
        tr->DestroyInstance();
        TransmitterTestAccess::ResetInstance();
        // Clean up temp directory
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", tempDir);
        system(cmd);
    }

    std::string configPath() const {
        return std::string(tempDir) + "/config.json";
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
    tr->SetCfgIP("10.0.0.50");
    CHECK_EQUAL(Transmitter::OK, tr->SaveConfig());

    struct stat st;
    CHECK_EQUAL(0, stat(configPath().c_str(), &st));
    CHECK(st.st_size > 0);
}

// ---- SaveConfig round-trip ----
TEST(TransmitterIO, SaveConfig_RoundTrip)
{
    tr->SetCfgIP("172.16.0.1");
    tr->SetInvertAB(true);
    tr->SetTurboInterval(80);
    CHECK_EQUAL(Transmitter::OK, tr->SaveConfig());

    // Destroy and recreate to force LoadConfig
    tr->DestroyInstance();
    TransmitterTestAccess::ResetInstance();
    Transmitter::CreateInstance(&gStubApp);
    tr = Transmitter::GetInstance();

    char buf[64] = {};
    tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("172.16.0.1", buf);
    CHECK(tr->GetInvertAB());
    CHECK_EQUAL(80, tr->GetTurboInterval());
}

// ---- LoadConfig: missing file uses defaults ----
TEST(TransmitterIO, LoadConfig_MissingFile_UsesDefaults)
{
    // No config file exists (fresh temp dir). Constructor already called LoadConfig.
    char buf[64] = {};
    tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("192.168.100.100", buf); // default IP
}

// ---- LoadConfig: invalid JSON falls back to defaults ----
TEST(TransmitterIO, LoadConfig_InvalidJson_FallsBack)
{
    writeFile(configPath(), "not valid json {{{");

    tr->DestroyInstance();
    TransmitterTestAccess::ResetInstance();
    Transmitter::CreateInstance(&gStubApp);
    tr = Transmitter::GetInstance();

    char buf[64] = {};
    tr->GetCfgIP(buf, sizeof(buf));
    STRCMP_EQUAL("192.168.100.100", buf); // defaults used
}

// ---- SaveConfig: verify JSON content structure ----
TEST(TransmitterIO, SaveConfig_JsonContent)
{
    tr->SetCfgIP("192.168.1.1");
    tr->SetInvertAB(true);
    tr->SetInvertXY(false);
    tr->SetSwapJoysticks(true);
    tr->SetHomeMap(true);
    tr->SetPowerMap(false);
    tr->SetPowerOffMap(true);
    tr->SetTurboInterval(50);
    CHECK_EQUAL(Transmitter::OK, tr->SaveConfig());

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
