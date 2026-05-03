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
    }
    for(auto i=0;i<MAX_JOYSTICK_INDEX;++i)
    {
        mJoystick[i].x = 0.0f;
        mJoystick[i].y = 0.0f;
    }

    mConfigPath = gCfgPath+mConfigName;
    mSock = -1;
    LoadConfig();
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
    mCfg.cfgSize = sizeof(mCfg);
    strncpy(mCfg.ip, "192.168.100.100", CONFIG_IP_MAX_LEN - 1);
    mCfg.ip[CONFIG_IP_MAX_LEN - 1] = '\0';
    mCfg.gamepadCfg.deadZone[JOYSTICK_L] = 0.05f;
    mCfg.gamepadCfg.deadZone[JOYSTICK_R] = 0.05f;
    mCfg.gamepadCfg.invertAB = false;
    mCfg.gamepadCfg.invertXY = false;
    mCfg.gamepadCfg.mapHome = false;
    mCfg.gamepadCfg.mapPower = false;
    mCfg.gamepadCfg.mapShut = false;
    mCfg.gamepadCfg.turboIntervalMs = 60;
    mCfg.isFirstRun = true;
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
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_R;
                break;
            case INPUT_KEY_INDEX_L3:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_L;
                break;
            case INPUT_KEY_INDEX_HOME:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_HOME;
                break;
            case INPUT_KEY_INDEX_SHARE:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_POWER;
                break;
            case INPUT_KEY_INDEX_SCRSHOT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_POWER;
                break;
            case INPUT_KEY_INDEX_JCL_UP:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_UP;
                break;
            case INPUT_KEY_INDEX_JCL_DOWN:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_DOWN;
                break;
            case INPUT_KEY_INDEX_JCL_LEFT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_LEFT;
                break;
            case INPUT_KEY_INDEX_JCL_RIGHT:
                mCfg.gamepadCfg.targetKeyIndex[i] = N3DS_KEY_INDEX_RIGHT;
                break;
            default:
                ALOGE("Invalid index %d", i);
                break;
        }
    }

}
void Transmitter::LoadConfig()
{
    ALOGD("Config file name: %s",mConfigPath.c_str());
    SetDefaultConfigValue();

    auto fd = open(mConfigPath.c_str(), O_RDONLY);
    if(-1 == fd)
    {
        ALOGD("No config file, using defaults. %s", strerror(errno));
        SaveConfig();
        return;
    }

    u32 savedCfgSize = 0;
    ssize_t n = read(fd, &savedCfgSize, sizeof(savedCfgSize));
    if(n != sizeof(savedCfgSize))
    {
        ALOGE("Failed to read config size (read %zd bytes). %s", n, strerror(errno));
        close(fd);
        SaveConfig();
        return;
    }

    if(savedCfgSize != sizeof(mCfg))
    {
        ALOGW("Config size mismatch: saved=%u current=%zu. Resetting to defaults.",
              savedCfgSize, sizeof(mCfg));
        close(fd);
        SaveConfig();
        return;
    }

    n = read(fd, &mCfg.isFirstRun, sizeof(mCfg)-sizeof(mCfg.cfgSize));
    if(n != (ssize_t)(sizeof(mCfg)-sizeof(mCfg.cfgSize)))
    {
        ALOGE("Failed to read config body (read %zd bytes). %s", n, strerror(errno));
        close(fd);
        SetDefaultConfigValue();
        SaveConfig();
        return;
    }

    close(fd);
    mCfg.cfgSize = sizeof(mCfg);
    mCfg.ip[CONFIG_IP_MAX_LEN - 1] = '\0';
    ALOGD("%s: OK, ip=%s", __FUNCTION__, mCfg.ip);
}

Transmitter::RetVal Transmitter::SaveConfig()
{
    std::string tmpPath = mConfigPath + ".tmp";
    auto fd = open(tmpPath.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if(-1 == fd)
    {
        ALOGE("Failed to open temp config file. %s", strerror(errno));
        return NOK;
    }

    ssize_t n = write(fd, &mCfg, sizeof(mCfg));
    close(fd);
    if(n != sizeof(mCfg))
    {
        ALOGE("Failed to write config (wrote %zd of %zu bytes). %s", n, sizeof(mCfg), strerror(errno));
        unlink(tmpPath.c_str());
        return NOK;
    }

    if(rename(tmpPath.c_str(), mConfigPath.c_str()) != 0)
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
    addr.sin_port = htons(4950);
    inet_pton(AF_INET, mCfg.ip, &addr.sin_addr);
    if(sendto(mSock, mFrameBuffer, 20, 0,
              reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        close(mSock);
        mSock = -1;
    }
}

void Transmitter::TaskLoop()
{
    while (!mApp->destroyRequested)
    {
        int events;
        android_poll_source *pSource;
        while(ALooper_pollOnce(0, nullptr, &events,
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
    mFrameData.hidPad = 0xfff;
    mFrameData.circlePadState = 0x7ff7ff;
    mFrameData.cppState = 0x80800081;
    mFrameData.irButtonsState = 0;
    mFrameData.interfaceButtons = 0;
    mFrameData.touchScreenState = 0x2000000;

    KeyEventToFrameData();
    MotionEventToFrameData();

    memset(mFrameBuffer, 0, 20);
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
        if(keyEvent->keyCode == gInputKeyTab[i].keycode ||
                keyEvent->scanCode == gInputKeyTab[i].keycode)
            return static_cast<INPUT_KEY_INDEX>(i);
    }
    return INPUT_KEY_INDEX_INVALID;
}

void Transmitter::KeyEventToFrameData()
{
    for(auto i=0; i<MAX_INPUT_KEY_INDEX; ++i)
    {
        if(mKeysState[i]==KEY_STATE_DOWN)
            OutputKeyIndexToFrameData(mCfg.gamepadCfg.targetKeyIndex[i]);
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
        mTurboMark[outIndex] &&
        mCfg.gamepadCfg.turbo[outIndex] == TURBO_ENABLE)
    {
        ALOGD("Send empty frame");
        mTurboMark[outIndex] = false;
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
    if(outIndex < MAX_N3DS_KEY_TURBO_INDEX &&
        mCfg.gamepadCfg.turbo[outIndex] == TURBO_ENABLE)
    {
        ALOGD("Send normal frame");
        mLastTurboTime = clock::now();
        mTurboMark[outIndex] = true;
    }
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
        ALOGD("keyEvent = 0x%p, keysource = %d, KeyScanCode = %d, KeyCode %d action=%s",keyEvent,
              keyEvent->source,
              keyEvent->scanCode,
              keyEvent->keyCode,
             (keyEvent->action == AKEY_EVENT_ACTION_DOWN) ? "DOWN" : "UP");
        /* Get input index by KEYCODE */
        auto inIndex = GetInputKeyIndex(keyEvent);
        if(INPUT_KEY_INDEX_INVALID == inIndex)
        {
            ALOGD("Unknown key event.");
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
                                                                       AMOTION_EVENT_AXIS_BRAKE) * 1000);
        int rt = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                       AMOTION_EVENT_AXIS_GAS) * 1000);
        /* Support Right Joy-con */
        if(static_cast<int>(mJoystick[JOYSTICK_R].x * 1000) == 0
           &&  static_cast<int>(mJoystick[JOYSTICK_R].y * 1000) == 0)
        {
            mJoystick[JOYSTICK_R].x = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                           AMOTION_EVENT_AXIS_RX);
            mJoystick[JOYSTICK_R].y = -GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                            AMOTION_EVENT_AXIS_RY) /** yAxisMultiplier*/;
        }

        ALOGD("Stick LX=%.3f LY=%.3f RX=%.3f RY=%.3f LLR=%d LUD=%d LT=%d RT=%d",
              mJoystick[JOYSTICK_L].x, mJoystick[JOYSTICK_L].y,
              mJoystick[JOYSTICK_R].x, mJoystick[JOYSTICK_R].y,
             hx, hy, lt, rt);

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
        mKeysState[INPUT_KEY_INDEX_LT] = lt>0?KEY_STATE_DOWN:KEY_STATE_UP;
        mKeysState[INPUT_KEY_INDEX_RT] = rt>0?KEY_STATE_DOWN:KEY_STATE_UP;
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
        u32 x = (u32)(lx * CPAD_BOUND + 0x800);
        u32 y = (u32)(ly * CPAD_BOUND + 0x800);
        x = x >= 0xfff ? (lx < 0.0 ? 0x000 : 0xfff) : x;
        y = y >= 0xfff ? (ly < 0.0 ? 0x000 : 0xfff) : y;
        mFrameData.circlePadState = (y << 12) | x;
    }

    if(rx != 0.0 || ry != 0.0 || mFrameData.irButtonsState != 0)
    {
        // We have to rotate the c-stick position 45°. Thanks, Nintendo.
        u32 x = (u32)(M_SQRT1_2 * (rx + ry) * CPP_BOUND + 0x80);
        u32 y = (u32)(M_SQRT1_2 * (ry - rx) * CPP_BOUND + 0x80);
        x = x >= 0xff ? (rx < 0.0 ? 0x00 : 0xff) : x;
        y = y >= 0xff ? (ry < 0.0 ? 0x00 : 0xff) : y;
        mFrameData.cppState = (y << 24) | (x << 16) | (mFrameData.irButtonsState << 8) | 0x81;
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
            mCfg.gamepadCfg.turbo[index] = TURBO_DISABLE;
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
    if(std::chrono::duration_cast<std::chrono::milliseconds>(
           clock::now() - mLastTurboTime).count() < (int64_t)mCfg.gamepadCfg.turboIntervalMs)
        return false;
    for(auto i=0; i<MAX_INPUT_KEY_INDEX; ++i)
    {
        if(mKeysState[i] == KEY_STATE_DOWN &&
            mCfg.gamepadCfg.turbo[mCfg.gamepadCfg.targetKeyIndex[i]] == TURBO_ENABLE)
        {
            mLastTurboTime = clock::now();
            return true;
        }
    }
    return false;
}
