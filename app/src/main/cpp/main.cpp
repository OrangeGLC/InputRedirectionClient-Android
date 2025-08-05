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

#define TAG "GamepadInput"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define GAMEPAD_BUTTON_A    (96)
#define GAMEPAD_BUTTON_B    (97)
#define GAMEPAD_BUTTON_X    (99)
#define GAMEPAD_BUTTON_Y    (100)
#define GAMEPAD_BUTTON_UP    (19) //ns pro controller only
#define GAMEPAD_BUTTON_DOWN  (20) //ns pro controller only
#define GAMEPAD_BUTTON_L    (21) //ns pro controller only
#define GAMEPAD_BUTTON_R    (22) //ns pro controller only
#define GAMEPAD_BUTTON_LB   (102)
#define GAMEPAD_BUTTON_RB   (103)
#define GAMEPAD_BUTTON_L3   (106)
#define GAMEPAD_BUTTON_R3   (107)
#define GAMEPAD_BUTTON_LT   (104) //ns pro controller only
#define GAMEPAD_BUTTON_RT   (105) //ns pro controller only
#define GAMEPAD_BUTTON_START    (108)
#define GAMEPAD_BUTTON_SELECT   (109)
#define GAMEPAD_BUTTON_SCRSHOT   (110) //ns pro controller only
#define GAMEPAD_BUTTON_SHARE   (130) //xbox controller only

#define GAMEPAD_BUTTON_A_OFFSET    (0)
#define GAMEPAD_BUTTON_B_OFFSET    (1)
#define GAMEPAD_BUTTON_SELECT_OFFSET   (2)
#define GAMEPAD_BUTTON_START_OFFSET    (3)
#define GAMEPAD_BUTTON_R_OFFSET    (4) //ns pro controller only
#define GAMEPAD_BUTTON_L_OFFSET    (5) //ns pro controller only
#define GAMEPAD_BUTTON_UP_OFFSET    (6) //ns pro controller only
#define GAMEPAD_BUTTON_DOWN_OFFSET  (7) //ns pro controller only
#define GAMEPAD_BUTTON_RB_OFFSET   (8)
#define GAMEPAD_BUTTON_LB_OFFSET   (9)
#define GAMEPAD_BUTTON_RT_OFFSET   (8) //ns pro controller only
#define GAMEPAD_BUTTON_LT_OFFSET   (9) //ns pro controller only
#define GAMEPAD_BUTTON_X_OFFSET    (10)
#define GAMEPAD_BUTTON_Y_OFFSET    (11)

#define GAMEPAD_BUTTON_L3_OFFSET   (106)
#define GAMEPAD_BUTTON_R3_OFFSET   (107)
#define GAMEPAD_BUTTON_SCRSHOT_OFFSET   (110) //ns pro controller only
#define GAMEPAD_BUTTON_SHARE_OFFSET   (130) //xbox controller only


const std::string CONFIG_FILE_NAME= "config";
std::string CONFIG_FILE_PATH = "";
const auto BUFFER_SIZE = 16;
std::string ip;
struct android_app *gApp = nullptr;
JNIEnv* gJNIEnv = nullptr;

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
}

void sendFrame(u32 hidPad, u32 touchScreenState, u32 circlePadState, u32 cppState, u32 interfaceButtons) {
    unsigned char ba[20] = {};
    memcpy(ba, &hidPad, 4);
    memcpy(ba + 4, &touchScreenState, 4);
    memcpy(ba + 8, &circlePadState, 4);
    memcpy(ba + 12, &cppState, 4);
    memcpy(ba + 16, &interfaceButtons, 4);
    aout << "Send to IP: " << ip << std::endl;
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4950);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    sendto(sock, ba, 20, 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    close(sock);
}

void handleInput()
{
    u32 hidPad = 0xfff;
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

    if (inputBuffer->keyEventsCount != 0) {
        GameActivityKeyEvent* keyEvent = &inputBuffer->keyEvents[0];
        if (keyEvent->source & AINPUT_SOURCE_GAMEPAD)  {
            for(int i=0; i<inputBuffer->keyEventsCount; ++i) {
                if(keyEvent->action == AKEY_EVENT_ACTION_DOWN)
                {
                    switch (keyEvent->keyCode)
                    {
                        case GAMEPAD_BUTTON_A:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_A_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_B:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_B_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_SELECT:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_SELECT_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_START:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_START_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_R:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_R_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_L:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_L_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_RB:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_RB_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_LB:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_LB_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_LT:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_LT_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_RT:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_RT_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_X:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_X_OFFSET);
                            break;
                        case GAMEPAD_BUTTON_Y:
                            hidPad &= ~(1 << GAMEPAD_BUTTON_Y_OFFSET);
                            break;
                        default:
                            continue;
                    }
                }
            }

            LOGI("Key %s code=%d action=%s",
                 (keyEvent->action == AKEY_EVENT_ACTION_DOWN) ? "↓" : "↑",
                 keyEvent->keyCode,
                 (keyEvent->action == AKEY_EVENT_ACTION_DOWN) ? "DOWN" : "UP");
        }
        android_app_clear_key_events(inputBuffer);
    }
    if (inputBuffer->motionEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
            GameActivityMotionEvent* motionEvent = &inputBuffer->motionEvents[i];
            if (motionEvent->source & AINPUT_SOURCE_JOYSTICK) {
                float lx = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_X);
                float ly = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_Y);
                float hx = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_HAT_X);
                float hy = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_HAT_Y);
                float lt = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_BRAKE);
                float rt = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_GAS);
                float rx = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_Z);
                float ry = GameActivityPointerAxes_getAxisValue(&motionEvent->pointers[0],
                                                                AMOTION_EVENT_AXIS_RZ);

                LOGI("Stick LX=%.3f LY=%.3f RX=%.3f RY=%.3f LLR=%.3f LUD=%.3f LT=%.3f RT=%.3f",
                     lx, ly, rx, ry, hx, hy, lt, rt);
            }
        }
        android_app_clear_motion_events(inputBuffer);
    }
    sendFrame(hidPad, 0x2000000, 0x7ff7ff,0x80800081,0);
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
