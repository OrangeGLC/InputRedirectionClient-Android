//
// Created by 荆荣 on 2025/8/9.
//

#include "Transmitter.h"
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include "JNIAdapt.h"
#include "AndroidOut.h"
#include "cJSON.h"

#define CPAD_BOUND          0x5d0
#define CPP_BOUND           0x7f

#define LOG_TAG "IRC"

extern std::mutex mutexCfgPathReady;
extern std::string gCfgPath;
extern struct android_app *gApp;
Transmitter* Transmitter::_singleton = nullptr;

Transmitter::Transmitter(struct android_app *app)
{
    ALOGD("Transmitter: initializing.");
    if(nullptr == app)
    {
        ALOGF("app is nullptr. Abort.");
    }
    mApp = app;
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_X);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_Y);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_Z);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_RZ);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_HAT_X); //xbox left ←(-1) →(1)
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_HAT_Y); //xbox left ↑(-1) ↓(1)
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_BRAKE); //xbox LT
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_GAS); //xbox RT
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_RX); //joy-con_R
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_RY); //joy-con_R

    //init keys state
    for(auto i=0;i<MAX_INPUT_KEY_INDEX;++i)
    {
        mKeysState[i] = KEY_STATE_UP;
    }
    for(auto i=0; i<MAX_N3DS_KEY_TURBO_INDEX; ++i)
    {
        mTurboMark[i] = false;
        mTurboActive[i] = false;
        mLastTurboTime[i] = clock::now();
    }
    for(auto i=0;i<MAX_JOYSTICK_INDEX;++i)
    {
        mJoystick[i].x = 0.0f;
        mJoystick[i].y = 0.0f;
    }

    mConfigPath = gCfgPath+mConfigName;
    mSock = -1;
    mLastSendTime = clock::now();
    LoadConfig();
    GenerateFrame(); // initialize mFrameBuffer with neutral frame before first SendFrame
    ALOGD("Transmitter: Finish initialization.");
}

Transmitter::~Transmitter()
{
    if(mSock >= 0) close(mSock);
}

Transmitter *Transmitter::CreateInstance(struct android_app *app) {
    if(nullptr == _singleton)
        _singleton = new Transmitter(app);
    return _singleton;
}

Transmitter *Transmitter::GetInstance()
{
    return _singleton;
}

void Transmitter::DestroyInstance()
{
    if(nullptr != _singleton)
        delete _singleton;
}

void Transmitter::SetDefaultConfigValue()
{
    strncpy(mCfg.ip, "192.168.100.100", CONFIG_IP_MAX_LEN - 1);
    mCfg.ip[CONFIG_IP_MAX_LEN - 1] = '\0';
    mCfg.gamepadCfg.deadZone[JOYSTICK_L] = 0.05f;
    mCfg.gamepadCfg.deadZone[JOYSTICK_R] = 0.05f;
    mCfg.gamepadCfg.keyMapMode = KEYMAP_MODE_SIMPLE;
    mCfg.gamepadCfg.ctrlType = CONTROLLER_TYPE_XBOX;
    mCfg.gamepadCfg.swapJoysticks = false;
    mCfg.gamepadCfg.invertAB = false;
    mCfg.gamepadCfg.invertXY = false;
    mCfg.gamepadCfg.mapHome = false;
    mCfg.gamepadCfg.mapPower = false;
    mCfg.gamepadCfg.mapShut = false;
    mCfg.gamepadCfg.turboIntervalMs = DEFAULT_TURBO_INTERVAL;
    for(auto i=0; i<MAX_N3DS_KEY_TURBO_INDEX; ++i)
        mCfg.gamepadCfg.turboMode[i] = TURBO_MODE_SEMI;
    SetDefaultKeyMapValue();
}

void Transmitter::SetDefaultKeyMapValue()
{
    for(auto i=0; i<MAX_INPUT_KEY_INDEX; ++i)
    {
        switch(i)
        {
            case INPUT_KEY_INDEX_A:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_A;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_B:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_B;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_X:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_X;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_Y:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_Y;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_RB:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_R;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_LB:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_L;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_RT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_ZR;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_LT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_ZL;
                mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] = TURBO_DISABLE;
                break;
            case INPUT_KEY_INDEX_SELECT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_SELECT;
                break;
            case INPUT_KEY_INDEX_START:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_START;
                break;
            case INPUT_KEY_INDEX_UP:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_UP;
                break;
            case INPUT_KEY_INDEX_DOWN:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_DOWN;
                break;
            case INPUT_KEY_INDEX_RIGHT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_RIGHT;
                break;
            case INPUT_KEY_INDEX_LEFT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_LEFT;
                break;
            case INPUT_KEY_INDEX_R3:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_INVALID;
                break;
            case INPUT_KEY_INDEX_L3:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_INVALID;
                break;
            case INPUT_KEY_INDEX_HOME:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_HOME;
                break;
            case INPUT_KEY_INDEX_SHARE:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_POWER;
                break;
            case INPUT_KEY_INDEX_SCRSHOT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_INVALID;
                break;
            case INPUT_KEY_INDEX_JCL_UP:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_INVALID;
                break;
            case INPUT_KEY_INDEX_JCL_DOWN:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_INVALID;
                break;
            case INPUT_KEY_INDEX_JCL_LEFT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_INVALID;
                break;
            case INPUT_KEY_INDEX_JCL_RIGHT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_INVALID;
                break;
            default:
                ALOGE("Invalid index %d", i);
                break;
        }
    }
}

void Transmitter::AdaptToCtrlType(CONTROLLER_TYPE type)
{
    switch(type)
    {
        case CONTROLLER_TYPE_JOYCON:
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_UP;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_DOWN;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_LEFT] = N3DS_KEY_INDEX_LEFT;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_RIGHT] = N3DS_KEY_INDEX_RIGHT;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_UP] = N3DS_KEY_INDEX_INVALID;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_DOWN] = N3DS_KEY_INDEX_INVALID;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LEFT] = N3DS_KEY_INDEX_INVALID;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_RIGHT] = N3DS_KEY_INDEX_INVALID;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_SHARE] = N3DS_KEY_INDEX_INVALID;
            break;

        case CONTROLLER_TYPE_XBOX:
        case CONTROLLER_TYPE_UNKNOWN:
        default:
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_UP] = N3DS_KEY_INDEX_UP;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_DOWN] = N3DS_KEY_INDEX_DOWN;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LEFT] = N3DS_KEY_INDEX_LEFT;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_RIGHT] = N3DS_KEY_INDEX_RIGHT;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_SHARE] = N3DS_KEY_INDEX_POWER;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_UP] = N3DS_KEY_INDEX_INVALID;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_DOWN] = N3DS_KEY_INDEX_INVALID;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_LEFT] = N3DS_KEY_INDEX_INVALID;
            mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_JCL_RIGHT] = N3DS_KEY_INDEX_INVALID;
    }
}

void Transmitter::ParseJsonConfig(cJSON *root)
{
    cJSON *ip = cJSON_GetObjectItem(root, "ip");
    if(cJSON_IsString(ip) && ip->valuestring)
        strncpy(mCfg.ip, ip->valuestring, CONFIG_IP_MAX_LEN - 1);

    cJSON *kmm = cJSON_GetObjectItem(root, "keyMapMode");
    if(cJSON_IsNumber(kmm)) SetKeyMapMode(kmm->valueint);

    cJSON *swj = cJSON_GetObjectItem(root, "swapJoysticks");
    if(cJSON_IsBool(swj)) SetSwapJoysticks(cJSON_IsTrue(swj));

    cJSON *iab = cJSON_GetObjectItem(root, "invertAB");
    if(cJSON_IsBool(iab)) SetInvertAB(cJSON_IsTrue(iab));

    cJSON *ixy = cJSON_GetObjectItem(root, "invertXY");
    if(cJSON_IsBool(ixy)) SetInvertXY(cJSON_IsTrue(ixy));

    cJSON *ct = cJSON_GetObjectItem(root, "ctrlType");
    if(cJSON_IsNumber(ct))
    {
        int typeVal = ct->valueint;
        if(typeVal >= 0 && typeVal < CONTROLLER_TYPE_UNKNOWN)
            mCfg.gamepadCfg.ctrlType = static_cast<CONTROLLER_TYPE>(typeVal);
    }

    cJSON *keyMappings = cJSON_GetObjectItem(root, "keyMappings");
    if(cJSON_IsObject(keyMappings) && mCfg.gamepadCfg.keyMapMode == KEYMAP_MODE_CUSTOM)
    {
        // keyMappings: N3DS key name → physical key index
        for(int i = 0; i < MAX_N3DS_KEY_INDEX; ++i)
        {
            cJSON *physIdx = cJSON_GetObjectItem(keyMappings, gN3DsKeyTab[i].name);
            if(!cJSON_IsNumber(physIdx)) continue;
            int j = physIdx->valueint;
            if(j >= 0 && j < MAX_INPUT_KEY_INDEX)
                mCfg.gamepadCfg.targetKeyIndex[j] = static_cast<N3DS_KEY_INDEX>(i);
        }
    }

    cJSON *mh = cJSON_GetObjectItem(root, "mapHome");
    if(cJSON_IsBool(mh)) SetHomeMap(cJSON_IsTrue(mh));

    cJSON *mp = cJSON_GetObjectItem(root, "mapPower");
    if(cJSON_IsBool(mp)) SetPowerMap(cJSON_IsTrue(mp));

    cJSON *ms = cJSON_GetObjectItem(root, "mapShut");
    if(cJSON_IsBool(ms)) SetPowerOffMap(cJSON_IsTrue(ms));

    cJSON *ti = cJSON_GetObjectItem(root, "turboIntervalMs");
    if(cJSON_IsNumber(ti)) SetTurboInterval((u32)ti->valueint);

    cJSON *turboKeys = cJSON_GetObjectItem(root, "turboKeys");
    if(cJSON_IsObject(turboKeys))
    {
        for(int i = 0; i < MAX_N3DS_KEY_TURBO_INDEX; ++i)
        {
            cJSON *keyObj = cJSON_GetObjectItem(turboKeys, gN3DsKeyTab[i].name);
            if(!keyObj) continue;
            cJSON *enb = cJSON_GetObjectItem(keyObj, "enable");
            if(cJSON_IsBool(enb))
                SetTurbo(static_cast<N3DS_KEY_INDEX>(i), cJSON_IsTrue(enb));
            cJSON *fa = cJSON_GetObjectItem(keyObj, "fullAuto");
            if(cJSON_IsBool(fa))
                SetTurboMode(i, cJSON_IsTrue(fa));
        }
    }
}

void Transmitter::LoadConfig()
{
    ALOGD("%s: loading config.", __FUNCTION__);
    SetDefaultConfigValue();

    std::string jsonPath = gCfgPath + "/config.json";

    /* Try JSON config */
    auto fd = open(jsonPath.c_str(), O_RDONLY);
    if(fd >= 0)
    {
        off_t sz = lseek(fd, 0, SEEK_END);
        if(sz > 0 && sz < CONFIG_FILE_MAX_SIZE)
        {
            lseek(fd, 0, SEEK_SET);
            char *buf = (char*)malloc((size_t)sz + 1);
            if(buf)
            {
                ssize_t n = read(fd, buf, sz);
                if(n == sz)
                {
                    buf[sz] = '\0';
                    cJSON *root = cJSON_Parse(buf);
                    if(root)
                    {
                        ParseJsonConfig(root);
                        cJSON_Delete(root);
                        free(buf);
                        close(fd);
                        mCfg.ip[CONFIG_IP_MAX_LEN - 1] = '\0';
                        ALOGD("%s: OK (json), ip=%s", __FUNCTION__, mCfg.ip);
                        return;
                    }
                    ALOGE("Failed to parse JSON config.");
                    cJSON_Delete(root);
                }
                free(buf);
            }
        }
        close(fd);
        ALOGE("Failed to read JSON config, trying legacy binary.");
    }

    /* Save defaults on first run */
    ALOGD("No config file, using defaults.");
    SaveConfig();
}

Transmitter::RetVal Transmitter::SaveConfig()
{
    std::string jsonPath = gCfgPath + "/config.json";
    std::string tmpPath = jsonPath + ".tmp";

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "version", CONFIG_JSON_VERSION);
    cJSON_AddStringToObject(root, "ip", mCfg.ip);
    cJSON_AddNumberToObject(root, "keyMapMode", mCfg.gamepadCfg.keyMapMode);
    cJSON_AddBoolToObject(root, "swapJoysticks", mCfg.gamepadCfg.swapJoysticks);
    cJSON_AddBoolToObject(root, "invertAB", mCfg.gamepadCfg.invertAB);
    cJSON_AddBoolToObject(root, "invertXY", mCfg.gamepadCfg.invertXY);
    cJSON_AddNumberToObject(root, "ctrlType", static_cast<int>(mCfg.gamepadCfg.ctrlType));
    cJSON_AddBoolToObject(root, "mapHome", mCfg.gamepadCfg.mapHome);
    cJSON_AddBoolToObject(root, "mapPower", mCfg.gamepadCfg.mapPower);
    cJSON_AddBoolToObject(root, "mapShut", mCfg.gamepadCfg.mapShut);
    cJSON_AddNumberToObject(root, "turboIntervalMs", mCfg.gamepadCfg.turboIntervalMs);

    cJSON *keyMappings = cJSON_CreateObject();
    for(int i = 0; i < MAX_N3DS_KEY_INDEX; ++i)
    {
        const char* n3dsName = gN3DsKeyTab[i].name;
        int physIdx = -1;
        for(int j = 0; j < MAX_INPUT_KEY_INDEX; ++j)
        {
            if(mCfg.gamepadCfg.targetKeyIndex[j] == i)
            {
                physIdx = j;
                break;
            }
        }
        cJSON_AddNumberToObject(keyMappings, n3dsName, physIdx);
    }
    cJSON_AddItemToObject(root, "keyMappings", keyMappings);

    cJSON *turboKeys = cJSON_CreateObject();
    for(int i = 0; i < MAX_N3DS_KEY_TURBO_INDEX; ++i)
    {
        cJSON *keyObj = cJSON_CreateObject();
        cJSON_AddBoolToObject(keyObj, "enable",
            mCfg.gamepadCfg.turbo[i] == TURBO_ENABLE);
        cJSON_AddBoolToObject(keyObj, "fullAuto",
            mCfg.gamepadCfg.turboMode[i] == TURBO_MODE_FULL);
        cJSON_AddItemToObject(turboKeys, gN3DsKeyTab[i].name, keyObj);
    }
    cJSON_AddItemToObject(root, "turboKeys", turboKeys);

    char *jsonStr = cJSON_Print(root);
    cJSON_Delete(root);
    if(!jsonStr) return NOK;

    auto fd = open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0)
    {
        ALOGE("Failed to open temp config file. %s", strerror(errno));
        cJSON_free(jsonStr);
        return NOK;
    }

    ssize_t len = (ssize_t)strlen(jsonStr);
    ssize_t n = write(fd, jsonStr, len);
    close(fd);
    cJSON_free(jsonStr);

    if(n != len)
    {
        ALOGE("Failed to write config (wrote %zd of %zd bytes). %s", n, len, strerror(errno));
        unlink(tmpPath.c_str());
        return NOK;
    }

    if(rename(tmpPath.c_str(), jsonPath.c_str()) != 0)
    {
        ALOGE("Failed to rename temp config. %s", strerror(errno));
        unlink(tmpPath.c_str());
        return NOK;
    }

    ALOGD("%s: OK", __FUNCTION__);
    return OK;
}

void Transmitter::SendFrame()
{
    if(mSock < 0)
    {
        mSock = socket(AF_INET, SOCK_DGRAM, 0);
        if(mSock < 0) return;
    }
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(N3DS_UDP_PORT);
    inet_pton(AF_INET, mCfg.ip, &addr.sin_addr);
    if(sendto(mSock, mFrameBuffer, 20, 0,
              reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        close(mSock);
        mSock = -1;
    }
    mLastSendTime = clock::now();
}

void Transmitter::TaskLoop()
{
    int events;
    while (!mApp->destroyRequested)
    {
        android_poll_source *pSource;
        while(ALooper_pollOnce(2, nullptr, &events,
                               reinterpret_cast<void**>(&pSource)) >= 0)
        {
            if (pSource) {
                pSource->process(mApp, pSource);
            }
        }
        HandleInputEvent();
    }
}

inline N3DS_KEY_INDEX Transmitter::GetOutputKeyIndex(INPUT_KEY_INDEX inKeyIndex)
{
    return mCfg.gamepadCfg.targetKeyIndex[inKeyIndex];
}

void Transmitter::GenerateFrame()
{
    mFrameData.hidPad = HID_PAD_NEUTRAL;
    mFrameData.circlePadState = CIRCLE_PAD_NEUTRAL;
    mFrameData.cppState = CPP_STATE_NEUTRAL;
    mFrameData.irButtonsState = 0;
    mFrameData.interfaceButtons = 0;
    mFrameData.touchScreenState = TOUCH_SCREEN_NEUTRAL;

    KeyEventToFrameData();
    MotionEventToFrameData();

    memset(mFrameBuffer, 0, N3DS_FRAME_SIZE);
    memcpy(mFrameBuffer, &mFrameData.hidPad, 4);
    memcpy(mFrameBuffer + 4, &mFrameData.touchScreenState, 4);
    memcpy(mFrameBuffer + 8, &mFrameData.circlePadState, 4);
    memcpy(mFrameBuffer + 12, &mFrameData.cppState, 4);
    memcpy(mFrameBuffer + 16, &mFrameData.interfaceButtons, 4);

}

INPUT_KEY_INDEX Transmitter::GetInputKeyIndex(GameActivityKeyEvent* keyEvent)
{
    for(auto i=0; i<MAX_INPUT_KEY_INDEX; ++i)
    {
        if(gInputKeyTab[i].isScanCode)
        {
            if(keyEvent->scanCode == gInputKeyTab[i].keycode)
                return static_cast<INPUT_KEY_INDEX>(i);
        }
        else
        {
            if(keyEvent->keyCode == gInputKeyTab[i].keycode)
                return static_cast<INPUT_KEY_INDEX>(i);
        }
    }
    return INPUT_KEY_INDEX_INVALID;
}

void Transmitter::KeyEventToFrameData()
{
    /* Full-auto: apply virtual key states */
    for(auto i=0; i<MAX_N3DS_KEY_TURBO_INDEX; ++i)
    {
        if(mCfg.gamepadCfg.turboMode[i] == TURBO_MODE_FULL &&
            mTurboActive[i] && mCfg.gamepadCfg.turbo[i] == TURBO_ENABLE)
            OutputKeyIndexToFrameData(static_cast<N3DS_KEY_INDEX>(i));
    }

    for(auto i=0; i<MAX_INPUT_KEY_INDEX; ++i)
    {
        if(mKeysState[i]==KEY_STATE_DOWN)
        {
            N3DS_KEY_INDEX out = mCfg.gamepadCfg.targetKeyIndex[i];
            if(out == N3DS_KEY_INDEX_INVALID) continue;
            if(out < MAX_N3DS_KEY_TURBO_INDEX &&
                mCfg.gamepadCfg.turboMode[out] == TURBO_MODE_FULL &&
                mTurboActive[out])
                continue;
            OutputKeyIndexToFrameData(out);
        }
    }
    /* Check if need power off (L3 + R3 press down meanwhile) */
    if(mKeysState[INPUT_KEY_INDEX_L3] == KEY_STATE_DOWN &&
        mKeysState[INPUT_KEY_INDEX_R3] == KEY_STATE_DOWN &&
        mCfg.gamepadCfg.mapShut )
        OutputKeyIndexToFrameData(N3DS_KEY_INDEX_SHUTDOWN);
}

void Transmitter::OutputKeyIndexToFrameData(N3DS_KEY_INDEX outIndex)
{
    if(outIndex == N3DS_KEY_INDEX_POWER && !mCfg.gamepadCfg.mapPower) return;

    if(outIndex == N3DS_KEY_INDEX_HOME && !mCfg.gamepadCfg.mapHome) return;
    if(outIndex < MAX_N3DS_KEY_TURBO_INDEX &&
        mCfg.gamepadCfg.turbo[outIndex] == TURBO_ENABLE)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now() - mLastTurboTime[outIndex]).count();
        if(elapsed >= (int64_t)mCfg.gamepadCfg.turboIntervalMs)
        {
            mTurboMark[outIndex] = !mTurboMark[outIndex];
            mLastTurboTime[outIndex] = clock::now();
        }
        if(mTurboMark[outIndex])
            return;
    }

    switch(gN3DsKeyTab[outIndex].obj)
    {
        case FIRST:
            mFrameData.hidPad &= ~(1 << gN3DsKeyTab[outIndex].offset);
            break;
        case FOURTH:
            mFrameData.irButtonsState |= 1 << gN3DsKeyTab[outIndex].offset;
            break;
        case FIFTH:
            mFrameData.interfaceButtons |= 1 << gN3DsKeyTab[outIndex].offset;
            break;
        default:
            break;
    }
}

// ---- Capture helpers ----

bool Transmitter::IsCapturableKey(INPUT_KEY_INDEX idx)
{
    switch(idx)
    {
        case INPUT_KEY_INDEX_A:
        case INPUT_KEY_INDEX_B:
        case INPUT_KEY_INDEX_X:
        case INPUT_KEY_INDEX_Y:
        case INPUT_KEY_INDEX_LB:
        case INPUT_KEY_INDEX_RB:
        case INPUT_KEY_INDEX_LT:
        case INPUT_KEY_INDEX_RT:
        case INPUT_KEY_INDEX_L3:
        case INPUT_KEY_INDEX_R3:
        case INPUT_KEY_INDEX_SELECT:
        case INPUT_KEY_INDEX_START:
        case INPUT_KEY_INDEX_HOME:
        case INPUT_KEY_INDEX_SHARE:
        case INPUT_KEY_INDEX_SCRSHOT:
        case INPUT_KEY_INDEX_UP:
        case INPUT_KEY_INDEX_DOWN:
        case INPUT_KEY_INDEX_LEFT:
        case INPUT_KEY_INDEX_RIGHT:
        case INPUT_KEY_INDEX_JCL_UP:
        case INPUT_KEY_INDEX_JCL_DOWN:
        case INPUT_KEY_INDEX_JCL_LEFT:
        case INPUT_KEY_INDEX_JCL_RIGHT:
            return true;
        default:
            return false;
    }
}

bool Transmitter::IsInputKeyRelevantForCtrlType(INPUT_KEY_INDEX idx, CONTROLLER_TYPE type)
{
    switch(type)
    {
        case CONTROLLER_TYPE_JOYCON:
            return idx != INPUT_KEY_INDEX_UP
                && idx != INPUT_KEY_INDEX_DOWN
                && idx != INPUT_KEY_INDEX_LEFT
                && idx != INPUT_KEY_INDEX_RIGHT
                && idx != INPUT_KEY_INDEX_SHARE;
        case CONTROLLER_TYPE_XBOX:
        case CONTROLLER_TYPE_UNKNOWN:
        default:
            return idx != INPUT_KEY_INDEX_JCL_UP
                && idx != INPUT_KEY_INDEX_JCL_DOWN
                && idx != INPUT_KEY_INDEX_JCL_LEFT
                && idx != INPUT_KEY_INDEX_JCL_RIGHT;
    }
}

CONTROLLER_TYPE Transmitter::DetectCtrlTypeFromKey(INPUT_KEY_INDEX idx)
{
    switch(idx)
    {
        case INPUT_KEY_INDEX_JCL_UP:
        case INPUT_KEY_INDEX_JCL_DOWN:
        case INPUT_KEY_INDEX_JCL_LEFT:
        case INPUT_KEY_INDEX_JCL_RIGHT:
            return CONTROLLER_TYPE_JOYCON;
        case INPUT_KEY_INDEX_UP:
        case INPUT_KEY_INDEX_DOWN:
        case INPUT_KEY_INDEX_LEFT:
        case INPUT_KEY_INDEX_RIGHT:
            return CONTROLLER_TYPE_XBOX;  // keyCode-based D-pad (NS Pro Controller)
        default:
            return mCfg.gamepadCfg.ctrlType;  // Non-D-pad keys: keep current type
    }
}

INPUT_KEY_INDEX Transmitter::FindPhysKeyForN3dsKey(N3DS_KEY_INDEX n3dsKey, CONTROLLER_TYPE type)
{
    for(int i = 0; i < MAX_INPUT_KEY_INDEX; ++i)
    {
        if(!IsInputKeyRelevantForCtrlType(static_cast<INPUT_KEY_INDEX>(i), type))
            continue;
        if(mCfg.gamepadCfg.targetKeyIndex[i] == n3dsKey)
            return static_cast<INPUT_KEY_INDEX>(i);
    }
    return INPUT_KEY_INDEX_INVALID;
}

void Transmitter::HandleKeyEvent(GameActivityKeyEvent* keyEvent)
{
    if(keyEvent == nullptr)
    {
        ALOGE("keyEvent is nullptr." );
        return;
    }
    if (keyEvent->source & AINPUT_SOURCE_GAMEPAD)
    {
        ALOGD("keyEvent: keyCode=%d scanCode=%d action=%s", keyEvent->keyCode,
              keyEvent->scanCode,
             (keyEvent->action == AKEY_EVENT_ACTION_DOWN) ? "DOWN" : "UP");
        /* Get input index by KEYCODE */
        auto inIndex = GetInputKeyIndex(keyEvent);
        if(INPUT_KEY_INDEX_INVALID == inIndex)
        {
            ALOGD("Unknown key: keyCode=%d scanCode=%d", keyEvent->keyCode, keyEvent->scanCode);
            return;
        }

        /* Key capture mode: intercept next key press for remapping */
        if(mCaptureTargetN3dsKey != N3DS_KEY_INDEX_INVALID
            && keyEvent->action == AKEY_EVENT_ACTION_DOWN
            && keyEvent->repeatCount == 0)
        {
            std::lock_guard<std::mutex> lock(mCaptureMutex);

            // Re-check after acquiring lock (may have been cancelled)
            if(mCaptureTargetN3dsKey == N3DS_KEY_INDEX_INVALID) return;

            // Whitelist: only capture specific keys
            if(!IsCapturableKey(inIndex)) return;

            // Save old target BEFORE AdaptToCtrlType (which may modify mappings)
            N3DS_KEY_INDEX oldTarget = mCfg.gamepadCfg.targetKeyIndex[inIndex];

            // Auto-detect controller type from D-pad keys
            bool adaptRan = false;
            CONTROLLER_TYPE detected = DetectCtrlTypeFromKey(inIndex);
            if(detected != mCfg.gamepadCfg.ctrlType)
            {
                mCfg.gamepadCfg.ctrlType = detected;
                AdaptToCtrlType(detected);
                adaptRan = true;
            }

            // If AdaptToCtrlType just gave this key a default, use it for displacement
            N3DS_KEY_INDEX displacedValue = oldTarget;
            if(adaptRan && oldTarget == N3DS_KEY_INDEX_INVALID)
                displacedValue = mCfg.gamepadCfg.targetKeyIndex[inIndex];

            CONTROLLER_TYPE ctrlType = mCfg.gamepadCfg.ctrlType;

            if(oldTarget == mCaptureTargetN3dsKey)
            {
                // No conflict: physical key already maps to this N3DS key
                N3DS_KEY_INDEX target = mCaptureTargetN3dsKey;
                mCaptureTargetN3dsKey = N3DS_KEY_INDEX_INVALID;
                callOnCaptureResult(gN3DsKeyTab[target].name,
                                    gInputKeyTab[inIndex].name, false, nullptr);
            }
            else if(oldTarget == N3DS_KEY_INDEX_INVALID
                    || !IsInputKeyRelevantForCtrlType(inIndex, ctrlType))
            {
                // Unmapped key (or key not relevant to current controller):
                // assign directly, displace old occupant (ctrlType-filtered)
                N3DS_KEY_INDEX target = mCaptureTargetN3dsKey;
                mCfg.gamepadCfg.targetKeyIndex[inIndex] = target;

                INPUT_KEY_INDEX oldPhys = FindPhysKeyForN3dsKey(target, ctrlType);
                if(oldPhys != INPUT_KEY_INDEX_INVALID && oldPhys != inIndex)
                    mCfg.gamepadCfg.targetKeyIndex[oldPhys] = displacedValue;

                mCaptureTargetN3dsKey = N3DS_KEY_INDEX_INVALID;
                SaveConfig();
                callOnCaptureResult(gN3DsKeyTab[target].name,
                                    gInputKeyTab[inIndex].name, false, nullptr);
            }
            else
            {
                // Conflict: relevant physical key already mapped to a different N3DS key
                mConflictInputIdx = inIndex;
                mConflictOldN3dsIdx = oldTarget;
                mConflictSessionId = mCaptureSessionId;
                callOnCaptureResult(gN3DsKeyTab[mCaptureTargetN3dsKey].name,
                                    gInputKeyTab[inIndex].name, true,
                                    gN3DsKeyTab[oldTarget].name);
            }
            return;
        }

        /* Full-auto mode: turbo keys toggle on press */
        N3DS_KEY_INDEX outIndex = mCfg.gamepadCfg.targetKeyIndex[inIndex];
        if(outIndex < MAX_N3DS_KEY_TURBO_INDEX &&
            mCfg.gamepadCfg.turbo[outIndex] == TURBO_ENABLE &&
            mCfg.gamepadCfg.turboMode[outIndex] == TURBO_MODE_FULL)
        {
            if(keyEvent->action == AKEY_EVENT_ACTION_DOWN && keyEvent->repeatCount == 0)
                mTurboActive[outIndex] = !mTurboActive[outIndex];
            return;
        }

        mKeysState[inIndex] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
    }
}

void Transmitter::HandleMotionEvent(GameActivityMotionEvent* motionEvent)
{
    if(motionEvent == nullptr)
    {
        ALOGE("motionEvent is nullptr.");
        return;
    }

    if (motionEvent->source & AINPUT_SOURCE_JOYSTICK)
    {
        mJoystick[JOYSTICK_L].x = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                       AMOTION_EVENT_AXIS_X);
        mJoystick[JOYSTICK_L].y = -GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                        AMOTION_EVENT_AXIS_Y) /** yAxisMultiplier*/;
        mJoystick[JOYSTICK_R].x = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                       AMOTION_EVENT_AXIS_Z);
        mJoystick[JOYSTICK_R].y = -GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                        AMOTION_EVENT_AXIS_RZ) /** yAxisMultiplier*/;
        int hx = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                       AMOTION_EVENT_AXIS_HAT_X));
        int hy = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                       AMOTION_EVENT_AXIS_HAT_Y));
        int lt = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                       AMOTION_EVENT_AXIS_BRAKE) * AXIS_INT_SCALE);
        int rt = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                       AMOTION_EVENT_AXIS_GAS) * AXIS_INT_SCALE);
        /* Support Right Joy-con */
        if(static_cast<int>(mJoystick[JOYSTICK_R].x * AXIS_INT_SCALE) == 0
           &&  static_cast<int>(mJoystick[JOYSTICK_R].y * AXIS_INT_SCALE) == 0)
        {
            mJoystick[JOYSTICK_R].x = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                           AMOTION_EVENT_AXIS_RX);
            mJoystick[JOYSTICK_R].y = -GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                            AMOTION_EVENT_AXIS_RY) /** yAxisMultiplier*/;
        }

        if(mCfg.gamepadCfg.swapJoysticks)
        {
            AxisValue tmp = mJoystick[JOYSTICK_L];
            mJoystick[JOYSTICK_L] = mJoystick[JOYSTICK_R];
            mJoystick[JOYSTICK_R] = tmp;
        }

        ALOGD("Stick LX=%.3f LY=%.3f RX=%.3f RY=%.3f LLR=%d LUD=%d LT=%d RT=%d",
              mJoystick[JOYSTICK_L].x, mJoystick[JOYSTICK_L].y,
              mJoystick[JOYSTICK_R].x, mJoystick[JOYSTICK_R].y,
             hx, hy, lt, rt);

        /* D-pad and analog trigger capture: intercept HAT/trigger rising edges */
        if(mCaptureTargetN3dsKey != N3DS_KEY_INDEX_INVALID)
        {
            std::lock_guard<std::mutex> lock(mCaptureMutex);

            if(mCaptureTargetN3dsKey != N3DS_KEY_INDEX_INVALID)
            {
                INPUT_KEY_INDEX capIdx = INPUT_KEY_INDEX_INVALID;
                bool isTrigger = false;

                // Check D-pad HAT first
                if(hx == 1 && mKeysState[INPUT_KEY_INDEX_RIGHT] != KEY_STATE_DOWN)
                    capIdx = INPUT_KEY_INDEX_RIGHT;
                else if(hx == -1 && mKeysState[INPUT_KEY_INDEX_LEFT] != KEY_STATE_DOWN)
                    capIdx = INPUT_KEY_INDEX_LEFT;
                else if(hy == 1 && mKeysState[INPUT_KEY_INDEX_DOWN] != KEY_STATE_DOWN)
                    capIdx = INPUT_KEY_INDEX_DOWN;
                else if(hy == -1 && mKeysState[INPUT_KEY_INDEX_UP] != KEY_STATE_DOWN)
                    capIdx = INPUT_KEY_INDEX_UP;
                // Then check analog triggers (LT/RT)
                else if(lt > 0 && mKeysState[INPUT_KEY_INDEX_LT] != KEY_STATE_DOWN)
                {
                    capIdx = INPUT_KEY_INDEX_LT;
                    isTrigger = true;
                }
                else if(rt > 0 && mKeysState[INPUT_KEY_INDEX_RT] != KEY_STATE_DOWN)
                {
                    capIdx = INPUT_KEY_INDEX_RT;
                    isTrigger = true;
                }

                if(capIdx != INPUT_KEY_INDEX_INVALID)
                {
                    // Save oldTarget BEFORE AdaptToCtrlType (which may modify mappings)
                    N3DS_KEY_INDEX oldTarget = mCfg.gamepadCfg.targetKeyIndex[capIdx];

                    // HAT D-pad → Xbox/Pro Controller (only auto-detect on first use)
                    if(!isTrigger && mCfg.gamepadCfg.ctrlType == CONTROLLER_TYPE_UNKNOWN)
                    {
                        mCfg.gamepadCfg.ctrlType = CONTROLLER_TYPE_XBOX;
                        AdaptToCtrlType(CONTROLLER_TYPE_XBOX);
                    }

                    CONTROLLER_TYPE ctrlType = mCfg.gamepadCfg.ctrlType;

                    bool consumed = false;
                    if(oldTarget == mCaptureTargetN3dsKey)
                    {
                        N3DS_KEY_INDEX target = mCaptureTargetN3dsKey;
                        mCaptureTargetN3dsKey = N3DS_KEY_INDEX_INVALID;
                        callOnCaptureResult(gN3DsKeyTab[target].name,
                                            gInputKeyTab[capIdx].name, false, nullptr);
                        consumed = true;
                    }
                    else if(oldTarget == N3DS_KEY_INDEX_INVALID
                            || !IsInputKeyRelevantForCtrlType(capIdx, ctrlType))
                    {
                        N3DS_KEY_INDEX target = mCaptureTargetN3dsKey;
                        mCfg.gamepadCfg.targetKeyIndex[capIdx] = target;

                        INPUT_KEY_INDEX oldPhys = FindPhysKeyForN3dsKey(target, ctrlType);
                        if(oldPhys != INPUT_KEY_INDEX_INVALID && oldPhys != capIdx)
                            mCfg.gamepadCfg.targetKeyIndex[oldPhys] = N3DS_KEY_INDEX_INVALID;

                        mCaptureTargetN3dsKey = N3DS_KEY_INDEX_INVALID;
                        SaveConfig();
                        callOnCaptureResult(gN3DsKeyTab[target].name,
                                            gInputKeyTab[capIdx].name, false, nullptr);
                        consumed = true;
                    }
                    else
                    {
                        mConflictInputIdx = capIdx;
                        mConflictOldN3dsIdx = oldTarget;
                        mConflictSessionId = mCaptureSessionId;
                        callOnCaptureResult(gN3DsKeyTab[mCaptureTargetN3dsKey].name,
                                            gInputKeyTab[capIdx].name, true,
                                            gN3DsKeyTab[oldTarget].name);
                        // Do NOT suppress HAT in conflict: let D-pad state track
                        // the held direction so subsequent events won't re-trigger
                    }
                    if(consumed)
                    {
                        if(!isTrigger) { hx = 0; hy = 0; }
                        // For triggers: suppress the state update so capture "consumes" it
                        else if(capIdx == INPUT_KEY_INDEX_LT) lt = 0;
                        else if(capIdx == INPUT_KEY_INDEX_RT) rt = 0;
                    }
                }
            }
        }
        switch(hx)
        {
            case 1:
                mKeysState[INPUT_KEY_INDEX_RIGHT] = KEY_STATE_DOWN;
                break;
            case -1:
                mKeysState[INPUT_KEY_INDEX_LEFT] = KEY_STATE_DOWN;
                break;
            default:
                mKeysState[INPUT_KEY_INDEX_LEFT] = KEY_STATE_UP;
                mKeysState[INPUT_KEY_INDEX_RIGHT] = KEY_STATE_UP;
        }
        switch(hy)
        {
            case 1:
                mKeysState[INPUT_KEY_INDEX_DOWN] = KEY_STATE_DOWN;
                break;
            case -1:
                mKeysState[INPUT_KEY_INDEX_UP] = KEY_STATE_DOWN;
                break;
            default:
                mKeysState[INPUT_KEY_INDEX_UP] = KEY_STATE_UP;
                mKeysState[INPUT_KEY_INDEX_DOWN] = KEY_STATE_UP;
        }
        /* Analog triggers on Xbox: detect rising edge for full-auto turbo toggle */
        bool ltWasDown = (mKeysState[INPUT_KEY_INDEX_LT] == KEY_STATE_DOWN);
        bool rtWasDown = (mKeysState[INPUT_KEY_INDEX_RT] == KEY_STATE_DOWN);

        mKeysState[INPUT_KEY_INDEX_LT] = lt>0?KEY_STATE_DOWN:KEY_STATE_UP;
        mKeysState[INPUT_KEY_INDEX_RT] = rt>0?KEY_STATE_DOWN:KEY_STATE_UP;

        N3DS_KEY_INDEX ltOut = mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_LT];
        N3DS_KEY_INDEX rtOut = mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_RT];

        auto handleAnalogTrigger = [this](N3DS_KEY_INDEX out, bool wasDown, int val) -> bool {
            if(out < MAX_N3DS_KEY_TURBO_INDEX &&
               mCfg.gamepadCfg.turbo[out] == TURBO_ENABLE &&
               mCfg.gamepadCfg.turboMode[out] == TURBO_MODE_FULL)
            {
                if(!wasDown && val > 0)
                    mTurboActive[out] = !mTurboActive[out];
                return true; // full-auto active, suppress mKeysState
            }
            return false;
        };

        if(handleAnalogTrigger(ltOut, ltWasDown, lt))
            mKeysState[INPUT_KEY_INDEX_LT] = KEY_STATE_UP;
        if(handleAnalogTrigger(rtOut, rtWasDown, rt))
            mKeysState[INPUT_KEY_INDEX_RT] = KEY_STATE_UP;
    }
}

void Transmitter::HandleInputEvent()
{
    if(mApp == nullptr)
    {
        ALOGF("mApp is not initialized.");
        return;
    }
    android_input_buffer* inputBuffer = android_app_swap_input_buffers(mApp);
    if(inputBuffer == nullptr)
    {
        /* No input event arrives*/
        if(NeedTurbo())
        {
            GenerateFrame();
            SendFrame();
            return;
        }
        if(std::chrono::duration_cast<std::chrono::milliseconds>(
               clock::now() - mLastSendTime).count() >= IDLE_FRAME_INTERVAL_MS)
        {
            SendFrame();
        }
        return;
    }
    if (inputBuffer->keyEventsCount != 0)
    {
        for(int i=0; i<inputBuffer->keyEventsCount; ++i)
        {
            HandleKeyEvent(&inputBuffer->keyEvents[i]);
        }
        android_app_clear_key_events(inputBuffer);
    }

    if (inputBuffer->motionEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
            HandleMotionEvent(&inputBuffer->motionEvents[i]);
        }
        android_app_clear_motion_events(inputBuffer);
    }

    GenerateFrame();
    SendFrame();
}

void Transmitter::MotionEventToFrameData()
{
    float& lx = mJoystick[JOYSTICK_L].x;
    float& ly = mJoystick[JOYSTICK_L].y;
    float& rx = mJoystick[JOYSTICK_R].x;
    float& ry = mJoystick[JOYSTICK_R].y;

    if (lx < mCfg.gamepadCfg.deadZone[JOYSTICK_L]
        && lx > -mCfg.gamepadCfg.deadZone[JOYSTICK_L])
        lx = 0.0;
    if (ly < mCfg.gamepadCfg.deadZone[JOYSTICK_L]
        && ly > -mCfg.gamepadCfg.deadZone[JOYSTICK_L])
        ly = 0.0;
    if (rx < mCfg.gamepadCfg.deadZone[JOYSTICK_R]
        && rx > -mCfg.gamepadCfg.deadZone[JOYSTICK_R])
        rx = 0.0;
    if (ry < mCfg.gamepadCfg.deadZone[JOYSTICK_R]
        && ry > -mCfg.gamepadCfg.deadZone[JOYSTICK_R])
        ry = 0.0;

    if(lx != 0.0 || ly != 0.0)
    {
        u32 x = (u32)(lx * CPAD_BOUND + CIRCLE_PAD_CENTER);
        u32 y = (u32)(ly * CPAD_BOUND + CIRCLE_PAD_CENTER);
        x = x >= CIRCLE_PAD_MAX ? (lx < 0.0 ? CIRCLE_PAD_MIN : CIRCLE_PAD_MAX) : x;
        y = y >= CIRCLE_PAD_MAX ? (ly < 0.0 ? CIRCLE_PAD_MIN : CIRCLE_PAD_MAX) : y;
        mFrameData.circlePadState = (y << 12) | x;
    }

    if(rx != 0.0 || ry != 0.0 || mFrameData.irButtonsState != 0)
    {
        // We have to rotate the c-stick position 45°. Thanks, Nintendo.
        u32 x = (u32)(M_SQRT1_2 * (rx + ry) * CPP_BOUND + CPP_CENTER);
        u32 y = (u32)(M_SQRT1_2 * (ry - rx) * CPP_BOUND + CPP_CENTER);
        x = x >= CPP_MAX ? (rx < 0.0 ? CPP_MIN : CPP_MAX) : x;
        y = y >= CPP_MAX ? (ry < 0.0 ? CPP_MIN : CPP_MAX) : y;
        mFrameData.cppState = (y << 24) | (x << 16) | (mFrameData.irButtonsState << 8) | CPP_MAGIC;
    }
}

Transmitter::RetVal Transmitter::GetCfgIP(char *buffer, size_t size)
{
    if(nullptr == buffer || size == 0)
    {
        ALOGE("buffer is nullptr or size is 0");
        return NOK;
    }
    size_t len = strlen(mCfg.ip);
    if(len >= size)
    {
        ALOGE("Buffer too small: need %zu, got %zu", len + 1, size);
        return NOK;
    }
    memcpy(buffer, mCfg.ip, len + 1);
    return OK;
}

void Transmitter::SetCfgIP(const char* ip)
{
    strncpy(mCfg.ip, ip, CONFIG_IP_MAX_LEN - 1);
    mCfg.ip[CONFIG_IP_MAX_LEN - 1] = '\0';
}


void Transmitter::SetInvertAB(bool flg)
{
    mCfg.gamepadCfg.invertAB = flg;
    if(flg)
    {
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_B;
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_B] = N3DS_KEY_INDEX_A;
    }
    else
    {
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_A] = N3DS_KEY_INDEX_A;
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_B] = N3DS_KEY_INDEX_B;
    }

}

void Transmitter::SetInvertXY(bool flg)
{
    mCfg.gamepadCfg.invertXY = flg;
    if(flg)
    {
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_X] = N3DS_KEY_INDEX_Y;
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_Y] = N3DS_KEY_INDEX_X;
    }
    else
    {
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_X] = N3DS_KEY_INDEX_X;
        mCfg.gamepadCfg.targetKeyIndex[INPUT_KEY_INDEX_Y] = N3DS_KEY_INDEX_Y;
    }
}

bool Transmitter::GetInvertAB()
{
    return mCfg.gamepadCfg.invertAB;
}

bool Transmitter::GetInvertXY()
{
    return mCfg.gamepadCfg.invertXY;
}

void Transmitter::SetTurbo(N3DS_KEY_INDEX index, bool flg)
{
    if(mCfg.gamepadCfg.turbo[index] != TURBO_NOT_SUPPORT)
    {
        if(flg)
            mCfg.gamepadCfg.turbo[index] = TURBO_ENABLE;
        else
        {
            mCfg.gamepadCfg.turbo[index] = TURBO_DISABLE;
            mTurboActive[index] = false;
        }
    }
}

bool Transmitter::GetTurbo(N3DS_KEY_INDEX index)
{
    if(mCfg.gamepadCfg.turbo[index] == TURBO_ENABLE)
        return true;
    return false;
}

void Transmitter::SetTurboInterval(u32 ms)
{
    mCfg.gamepadCfg.turboIntervalMs = ms;
}
u32 Transmitter::GetTurboInterval()
{
    return mCfg.gamepadCfg.turboIntervalMs;
}

void Transmitter::SetTurboMode(int index, bool fullAuto)
{
    if(index < 0 || index >= MAX_N3DS_KEY_TURBO_INDEX) return;
    mCfg.gamepadCfg.turboMode[index] = fullAuto ? TURBO_MODE_FULL : TURBO_MODE_SEMI;
    if(!fullAuto) mTurboActive[index] = false;
}

bool Transmitter::GetTurboMode(int index)
{
    if(index < 0 || index >= MAX_N3DS_KEY_TURBO_INDEX) return false;
    return mCfg.gamepadCfg.turboMode[index] == TURBO_MODE_FULL;
}

void Transmitter::SetHomeMap(bool flg)
{
    mCfg.gamepadCfg.mapHome = flg;
}

bool Transmitter::GetHomeMap()
{
    return mCfg.gamepadCfg.mapHome;
}

void Transmitter::SetPowerMap(bool flg)
{
    mCfg.gamepadCfg.mapPower = flg;
}

bool Transmitter::GetPowerMap()
{
    return mCfg.gamepadCfg.mapPower;
}

void Transmitter::SetPowerOffMap(bool flg)
{
    mCfg.gamepadCfg.mapShut = flg;
}

bool Transmitter::GetPowerOffMap()
{
    return mCfg.gamepadCfg.mapShut;
}

bool Transmitter::NeedTurbo()
{
    for(auto i=0; i<MAX_INPUT_KEY_INDEX; ++i)
    {
        N3DS_KEY_INDEX out = mCfg.gamepadCfg.targetKeyIndex[i];
        if(out >= MAX_N3DS_KEY_TURBO_INDEX) continue;
        if(mCfg.gamepadCfg.turbo[out] != TURBO_ENABLE) continue;

        if(mKeysState[i] == KEY_STATE_DOWN) return true;
        if(mCfg.gamepadCfg.turboMode[out] == TURBO_MODE_FULL && mTurboActive[out]) return true;
    }
    return false;
}

void Transmitter::SetKeyMapMode(int mode)
{
    if(mode != KEYMAP_MODE_SIMPLE && mode != KEYMAP_MODE_CUSTOM) return;

    bool wasCustom = (mCfg.gamepadCfg.keyMapMode == KEYMAP_MODE_CUSTOM);

    // Leaving custom mode: persist mappings before resetting to defaults
    if(wasCustom && mode == KEYMAP_MODE_SIMPLE)
        SaveConfig();

    mCfg.gamepadCfg.keyMapMode = static_cast<KEYMAP_MODE>(mode);

    if(mode == KEYMAP_MODE_SIMPLE)
    {
        SetDefaultKeyMapValue();
        SetInvertAB(mCfg.gamepadCfg.invertAB);
        SetInvertXY(mCfg.gamepadCfg.invertXY);
    }
    else
    {
        // Reload keyMappings from JSON (restores previously saved custom state)
        // Save after reload to persist the mode change (SIMPLE→CUSTOM)
        std::string jsonPath = gCfgPath + "/config.json";
        auto fd = open(jsonPath.c_str(), O_RDONLY);
        if(fd >= 0)
        {
            off_t sz = lseek(fd, 0, SEEK_END);
            if(sz > 0 && sz < CONFIG_FILE_MAX_SIZE)
            {
                lseek(fd, 0, SEEK_SET);
                char *buf = (char*)malloc((size_t)sz + 1);
                if(buf)
                {
                    ssize_t n = read(fd, buf, sz);
                    if(n == sz)
                    {
                        buf[sz] = '\0';
                        cJSON *root = cJSON_Parse(buf);
                        if(root)
                        {
                            cJSON *keyMappings = cJSON_GetObjectItem(root, "keyMappings");
                            if(cJSON_IsObject(keyMappings))
                            {
                                for(int i = 0; i < MAX_N3DS_KEY_INDEX; ++i)
                                {
                                    cJSON *physIdx = cJSON_GetObjectItem(keyMappings, gN3DsKeyTab[i].name);
                                    if(!cJSON_IsNumber(physIdx)) continue;
                                    int j = physIdx->valueint;
                                    if(j >= 0 && j < MAX_INPUT_KEY_INDEX)
                                        mCfg.gamepadCfg.targetKeyIndex[j] = static_cast<N3DS_KEY_INDEX>(i);
                                }
                            }
                            cJSON_Delete(root);
                        }
                    }
                    free(buf);
                }
            }
            close(fd);
        }
    }
}

int Transmitter::GetKeyMapMode()
{
    return mCfg.gamepadCfg.keyMapMode;
}

void Transmitter::SetSwapJoysticks(bool flg)
{
    mCfg.gamepadCfg.swapJoysticks = flg;
}

bool Transmitter::GetSwapJoysticks()
{
    return mCfg.gamepadCfg.swapJoysticks;
}

void Transmitter::SetKeyMapping(int inputIdx, int targetIdx)
{
    if(inputIdx < 0 || inputIdx >= MAX_INPUT_KEY_INDEX) return;
    if(targetIdx < 0 || targetIdx >= MAX_N3DS_KEY_INDEX) return;
    mCfg.gamepadCfg.targetKeyIndex[inputIdx] = static_cast<N3DS_KEY_INDEX>(targetIdx);
}

int Transmitter::GetKeyMapping(int inputIdx)
{
    if(inputIdx < 0 || inputIdx >= MAX_INPUT_KEY_INDEX) return N3DS_KEY_INDEX_INVALID;
    return static_cast<int>(mCfg.gamepadCfg.targetKeyIndex[inputIdx]);
}

void Transmitter::EnterKeyCapture(int n3dsKeyIndex)
{
    if(n3dsKeyIndex < 0 || n3dsKeyIndex >= MAX_N3DS_KEY_INDEX) return;
    std::lock_guard<std::mutex> lock(mCaptureMutex);
    mCaptureTargetN3dsKey = static_cast<N3DS_KEY_INDEX>(n3dsKeyIndex);
    mConflictInputIdx = INPUT_KEY_INDEX_INVALID;
    mConflictOldN3dsIdx = N3DS_KEY_INDEX_INVALID;
    mCaptureSessionId++;
}

void Transmitter::ExitKeyCapture()
{
    std::lock_guard<std::mutex> lock(mCaptureMutex);
    mCaptureTargetN3dsKey = N3DS_KEY_INDEX_INVALID;
    mConflictInputIdx = INPUT_KEY_INDEX_INVALID;
    mConflictOldN3dsIdx = N3DS_KEY_INDEX_INVALID;
}

int Transmitter::GetCaptureSessionId()
{
    return mCaptureSessionId;
}

void Transmitter::ResolveKeyConflict(bool accept, int sessionId)
{
    std::lock_guard<std::mutex> lock(mCaptureMutex);

    if(mCaptureTargetN3dsKey == N3DS_KEY_INDEX_INVALID) return;
    if(mConflictInputIdx == INPUT_KEY_INDEX_INVALID) return;

    // Reject stale resolution requests from dismissed capture sessions
    if(sessionId != 0 && sessionId != mConflictSessionId) return;

    if(accept)
    {
        CONTROLLER_TYPE ctrlType = mCfg.gamepadCfg.ctrlType;
        INPUT_KEY_INDEX oldPhysIdx = FindPhysKeyForN3dsKey(mCaptureTargetN3dsKey, ctrlType);
        if(oldPhysIdx == mConflictInputIdx)
            oldPhysIdx = INPUT_KEY_INDEX_INVALID;

        // Swap: new physical key → capture target
        mCfg.gamepadCfg.targetKeyIndex[mConflictInputIdx] = mCaptureTargetN3dsKey;
        // Reassign the displaced physical key to the old target
        if(oldPhysIdx != INPUT_KEY_INDEX_INVALID)
            mCfg.gamepadCfg.targetKeyIndex[oldPhysIdx] = mConflictOldN3dsIdx;

        SaveConfig();
    }
    // Exit capture mode in both cases
    mCaptureTargetN3dsKey = N3DS_KEY_INDEX_INVALID;
    mConflictInputIdx = INPUT_KEY_INDEX_INVALID;
    mConflictOldN3dsIdx = N3DS_KEY_INDEX_INVALID;
}
