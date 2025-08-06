#include <jni.h>
#include <android/log.h>
#include "AndroidOut.h"

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Gamepad.h"
#include <math.h>

#define EPSILON 1e-7
#define TAG "GamepadInput"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

#define CPAD_BOUND          0x5d0
#define CPP_BOUND           0x7f

#define TOUCH_SCREEN_WIDTH  320
#define TOUCH_SCREEN_HEIGHT 240

#define DEADZONE_MIN 	    0.05f

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;


const std::string CONFIG_FILE_NAME= "config";
std::string CONFIG_FILE_PATH = "";
const auto BUFFER_SIZE = 16;
std::string ip;
struct android_app *gApp = nullptr;


JNIEnv* gJNIEnv = nullptr;
const float yAxisMultiplier = 1.0f;
KEY_STATE gKeysState[KEY_INDEX_INVALID];
AxisValue gJoystick[JOYSTICK_INVALID];

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            //pApp->userData = new Renderer(pApp);
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being destroyed. Use this to clean up your userData to avoid leaking
            // resources.
            //
            // We have to check if userData is assigned just in case this comes in really quickly
            if (pApp->userData) {
                //
                //auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
                pApp->userData = nullptr;
                //delete pRenderer;
            }
            break;
        default:
            break;
    }
}

bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

bool key_event_filter_func(const GameActivityKeyEvent *keynEvent) {
   return true;
}

static JNIEnv* getJniEnv(struct android_app *pApp) {
    JNIEnv * env = nullptr;
    if(0 != pApp->activity->vm->AttachCurrentThread(&env, NULL))
    {
        aout << "Error: Failed to getJniEnv" << std::endl;
    }
    return env;
}

void initController()
{
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_X);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_Y);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_Z);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_RZ);
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_HAT_X); //xbox left ←(-1) →(1)
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_HAT_Y); //xbox left ↑(-1) ↓(1)
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_BRAKE); //xbox LT
    GameActivityPointerAxes_enableAxis(AMOTION_EVENT_AXIS_GAS); //xbox RT

    //init keys state
    for(auto i=0;i<KEY_INDEX_INVALID;++i)
    {
        gKeysState[i] = KEY_STATE_UP;
    }
    for(auto i=0;i<JOYSTICK_INVALID;++i)
    {
        gJoystick[i].x = 0.0f;
        gJoystick[i].y = 0.0f;
    }
}

void sendFrame(char* frame) {
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4950);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    sendto(sock, frame, 20, 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    close(sock);
}


void generateKeysCode(u32* pKeyCode, u32* pZLZRCode, u32* pCirclePadState, u32* pCppState) {
    if(pKeyCode == nullptr || pZLZRCode == nullptr ||
        pCirclePadState == nullptr || pCppState == nullptr)
    {
        aout << "Error: Line " << __LINE__ << ": Nullptr." << std::endl;
        return;
    }
    u32& hidPad = *pKeyCode;
    u32& irButtonsState = *pZLZRCode;
    u32& circlePadState = *pCirclePadState;
    u32& cppState = *pCppState;
    float& lx = gJoystick[JOYSTICK_L].x;
    float& ly = gJoystick[JOYSTICK_L].y;
    float& rx = gJoystick[JOYSTICK_R].x;
    float& ry = gJoystick[JOYSTICK_R].y;

    hidPad = 0xfff;
    irButtonsState = 0;
    circlePadState = 0x7ff7ff;
    cppState = 0x80800081;

    for(auto i=0; i<KEY_INDEX_INVALID; ++i)
    {
        if(gKeysState[i] != KEY_STATE_DOWN)
            continue;

        switch(i)
        {
            case KEY_INDEX_A:
                hidPad &= ~(1 << GAMEPAD_BUTTON_A_OFFSET);
                continue;
            case KEY_INDEX_B:
                hidPad &= ~(1 << GAMEPAD_BUTTON_B_OFFSET);
                continue;
            case KEY_INDEX_X:
                hidPad &= ~(1 << GAMEPAD_BUTTON_X_OFFSET);
                continue;
            case KEY_INDEX_Y:
                hidPad &= ~(1 << GAMEPAD_BUTTON_Y_OFFSET);
                continue;
            case KEY_INDEX_SELECT:
                hidPad &= ~(1 << GAMEPAD_BUTTON_SELECT_OFFSET);
                continue;
            case KEY_INDEX_START:
                hidPad &= ~(1 << GAMEPAD_BUTTON_START_OFFSET);
                continue;
            case KEY_INDEX_LB:
                hidPad &= ~(1 << GAMEPAD_BUTTON_LB_OFFSET);
                continue;
            case KEY_INDEX_RB:
                hidPad &= ~(1 << GAMEPAD_BUTTON_RB_OFFSET);
                continue;
            case KEY_INDEX_LT:
                irButtonsState |= 1 << GAMEPAD_BUTTON_LT_OFFSET;
                continue;
            case KEY_INDEX_RT:
                irButtonsState |= 1 << GAMEPAD_BUTTON_RT_OFFSET;
                continue;
            /*case KEY_INDEX_L3://Map to LT
                hidPad &= ~(1 << GAMEPAD_BUTTON_LT_OFFSET);
                continue;
            case KEY_INDEX_R3://Map to RT
                hidPad &= ~(1 << GAMEPAD_BUTTON_RT_OFFSET);
                continue;*/
            case KEY_INDEX_UP:
                hidPad &= ~(1 << GAMEPAD_BUTTON_UP_OFFSET);
                continue;
            case KEY_INDEX_DOWN:
                hidPad &= ~(1 << GAMEPAD_BUTTON_DOWN_OFFSET);
                continue;
            case KEY_INDEX_LEFT:
                hidPad &= ~(1 << GAMEPAD_BUTTON_LEFT_OFFSET);
                continue;
            case KEY_INDEX_RIGHT:
                hidPad &= ~(1 << GAMEPAD_BUTTON_RIGHT_OFFSET);
                continue;
            default:
                continue;
        }
    }

    if (lx < DEADZONE_MIN && lx > -DEADZONE_MIN) lx = 0.0;
    if (ly < DEADZONE_MIN && ly > -DEADZONE_MIN) ly = 0.0;
    if (rx < DEADZONE_MIN && rx > -DEADZONE_MIN) rx = 0.0;
    if (ry < DEADZONE_MIN && ry > -DEADZONE_MIN) ry = 0.0;

    if(lx != 0.0 || ly != 0.0)
    {
        u32 x = (u32)(lx * CPAD_BOUND + 0x800);
        u32 y = (u32)(ly * CPAD_BOUND + 0x800);
        x = x >= 0xfff ? (lx < 0.0 ? 0x000 : 0xfff) : x;
        y = y >= 0xfff ? (ly < 0.0 ? 0x000 : 0xfff) : y;
        circlePadState = (y << 12) | x;
    }

    if(rx != 0.0 || ry != 0.0 || irButtonsState != 0)
    {
        // We have to rotate the c-stick position 45°. Thanks, Nintendo.
        u32 x = (u32)(M_SQRT1_2 * (rx + ry) * CPP_BOUND + 0x80);
        u32 y = (u32)(M_SQRT1_2 * (ry - rx) * CPP_BOUND + 0x80);
        x = x >= 0xff ? (rx < 0.0 ? 0x00 : 0xff) : x;
        y = y >= 0xff ? (ry < 0.0 ? 0x00 : 0xff) : y;

        cppState = (y << 24) | (x << 16) | (irButtonsState << 8) | 0x81;
    }
}

char* generateFrame(char* buffer, size_t size) {
    if(buffer == nullptr || size < 20)
    {
        aout<< "Error: Line " << __LINE__ << ": Nullptr or buffer size is insufficient." << std::endl;
        return buffer;
    }
    u32 hidPad = 0;
    u32 touchScreenState = 0x2000000;
    u32 circlePadState = 0;
    u32 cppState = 0;
    u32 interfaceButtons = 0;
    generateKeysCode(&hidPad,
                     &interfaceButtons,
                     &circlePadState,
                     &cppState
                     );
    memcpy(buffer, &hidPad, 4);
    memcpy(buffer + 4, &touchScreenState, 4);
    memcpy(buffer + 8, &circlePadState, 4);
    memcpy(buffer + 12, &cppState, 4);
    memcpy(buffer + 16, &interfaceButtons, 4);
    return buffer;
}

void handleInput()
{
    if(gApp == nullptr)
    {
        aout << "Warrning: gApp is not initialized." << std::endl;
        return;
    }
    android_input_buffer* inputBuffer = android_app_swap_input_buffers(gApp);
    if(inputBuffer == nullptr)
    {
        return;
    }

    if (inputBuffer->keyEventsCount != 0)
    {
        for(int i=0; i<inputBuffer->keyEventsCount; ++i)
        {
            GameActivityKeyEvent* keyEvent = &inputBuffer->keyEvents[i];
            if (keyEvent->source & AINPUT_SOURCE_GAMEPAD)
            {
                switch (keyEvent->keyCode)
                {
                    case GAMEPAD_BUTTON_A:
                        gKeysState[KEY_INDEX_A] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_B:
                        gKeysState[KEY_INDEX_B] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_SELECT:
                        gKeysState[KEY_INDEX_SELECT] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_START:
                        gKeysState[KEY_INDEX_START] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_RIGHT:
                        gKeysState[KEY_INDEX_RIGHT] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_LEFT:
                        gKeysState[KEY_INDEX_LEFT] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_DOWN:
                        gKeysState[KEY_INDEX_DOWN] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_UP:
                        gKeysState[KEY_INDEX_UP] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_RB:
                        gKeysState[KEY_INDEX_RB] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_LB:
                        gKeysState[KEY_INDEX_LB] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_LT:
                        gKeysState[KEY_INDEX_LT] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_RT:
                        gKeysState[KEY_INDEX_RT] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_X:
                        gKeysState[KEY_INDEX_X] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    case GAMEPAD_BUTTON_Y:
                        gKeysState[KEY_INDEX_Y] = keyEvent->action == AKEY_EVENT_ACTION_DOWN?KEY_STATE_DOWN:KEY_STATE_UP;
                        break;
                    default:
                        break;
                }
            }
            LOGI("Key %s code=%d action=%s",
                 (keyEvent->action == AKEY_EVENT_ACTION_DOWN) ? "↓" : "↑",
                 keyEvent->keyCode,
                 (keyEvent->action == AKEY_EVENT_ACTION_DOWN) ? "DOWN" : "UP");
        }

    }
    android_app_clear_key_events(inputBuffer);
    if (inputBuffer->motionEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
            GameActivityMotionEvent* motionEvent = &inputBuffer->motionEvents[i];
            if (motionEvent->source & AINPUT_SOURCE_JOYSTICK) {
                gJoystick[JOYSTICK_L].x = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_X);
                gJoystick[JOYSTICK_L].y = -GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_Y) * yAxisMultiplier;
                gJoystick[JOYSTICK_R].x = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_Z);
                gJoystick[JOYSTICK_R].y = -GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                 AMOTION_EVENT_AXIS_RZ) * yAxisMultiplier;
                int hx = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_HAT_X));
                int hy = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_HAT_Y));
                int lt = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_BRAKE) * 1000);
                int rt = static_cast<int>(GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_GAS) * 1000);

                LOGI("Stick LX=%.3f LY=%.3f RX=%.3f RY=%.3f LLR=%d LUD=%d LT=%d RT=%d",
                     gJoystick[JOYSTICK_L].x, gJoystick[JOYSTICK_L].y,
                     gJoystick[JOYSTICK_R].x, gJoystick[JOYSTICK_R].y,
                     hx, hy, lt, rt);
                switch(hx)
                {
                    case 1:
                        gKeysState[KEY_INDEX_RIGHT] = KEY_STATE_DOWN;
                        break;
                    case -1:
                        gKeysState[KEY_INDEX_LEFT] = KEY_STATE_DOWN;
                        break;
                    default:
                        gKeysState[KEY_INDEX_LEFT] = KEY_STATE_UP;
                        gKeysState[KEY_INDEX_RIGHT] = KEY_STATE_UP;
                }
                switch(hy)
                {
                    case 1:
                        gKeysState[KEY_INDEX_DOWN] = KEY_STATE_DOWN;
                        break;
                    case -1:
                        gKeysState[KEY_INDEX_UP] = KEY_STATE_DOWN;
                        break;
                    default:
                        gKeysState[KEY_INDEX_UP] = KEY_STATE_UP;
                        gKeysState[KEY_INDEX_DOWN] = KEY_STATE_UP;
                }

                gKeysState[KEY_INDEX_LT] = lt>0?KEY_STATE_DOWN:KEY_STATE_UP;
                gKeysState[KEY_INDEX_RT] = rt>0?KEY_STATE_DOWN:KEY_STATE_UP;
            }
        }
        android_app_clear_motion_events(inputBuffer);
    }
    char buffer[20];
    sendFrame(generateFrame(buffer, 20));
}
/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    // Can be removed, useful to ensure your code is running
    aout << "Welcome to android_main" << std::endl;
    gApp = pApp;
    gJNIEnv = getJniEnv(pApp);
    if(gJNIEnv == nullptr)
    {
        aout << "Error: Failed to get JNIEnv. Abort." << std::endl;
        abort();
    }

    // Register an event handler for Android events
    pApp->onAppCmd = handle_cmd;
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);
    android_app_set_key_event_filter(pApp, key_event_filter_func);

    initController();

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    while (!pApp->destroyRequested){
        int timeout = 0;
        int events;
        android_poll_source *pSource;
        while(ALooper_pollOnce(timeout, nullptr, &events,
                                      reinterpret_cast<void**>(&pSource)) >= 0)
        {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }

        handleInput();
    }
}
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_inputredirectionclient_1android_MainActivity_saveIPAddress(JNIEnv *env,
                            jobject thiz,
                            jstring input) {
    ip = env->GetStringUTFChars(input, nullptr);
    aout << "Update IpAddress:" << ip << std::endl;
    auto fd = 0;
    auto fcfg = CONFIG_FILE_PATH + "/" + CONFIG_FILE_NAME;
    fd = open(fcfg.c_str(), O_WRONLY|O_CREAT|O_TRUNC , 0644);
    if(fd != 0)
    {
        if(write(fd, ip.c_str(), strlen(ip.c_str())) > 0)
        {
            aout << "Info: New IP Address Saved." << std::endl;
        }
    }
    close(fd);
    return env->NewStringUTF("Saved Successfully.");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_inputredirectionclient_1android_MainActivity_getSavedIPAddress(JNIEnv *env,
                                jobject thiz,
                                jstring inPath) {
    CONFIG_FILE_PATH = env->GetStringUTFChars(inPath, nullptr);
    auto fd = 0;
    auto fcfg = CONFIG_FILE_PATH + "/" + CONFIG_FILE_NAME;
    fd = open(fcfg.c_str(), O_RDONLY);
    if(-1 == fd)
    {
        aout << "Warrning: Failed to open config file." << std::endl;
        return env->NewStringUTF("192.168.1.1");
    }
    char buffer[BUFFER_SIZE] = {0};
    if(-1 == read(fd, buffer, BUFFER_SIZE))
    {
        aout << "Error: Failed to read config file." << std::endl;
        close(fd);
        return env->NewStringUTF("192.168.1.1");
    }

    close(fd);
    ip = std::string(buffer);
    return env->NewStringUTF(buffer);
}
