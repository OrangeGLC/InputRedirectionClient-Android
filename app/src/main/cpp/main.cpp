#include <jni.h>

#include "AndroidOut.h"

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <paddleboat/paddleboat.h>

const std::string CONFIG_FILE_NAME= "config";
std::string CONFIG_FILE_PATH = "";
const auto BUFFER_SIZE = 16;
char ip[BUFFER_SIZE] = {0};
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

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);



}

static JNIEnv* getJniEnv(struct android_app *pApp) {
    JNIEnv * env = nullptr;
    if(0 != pApp->activity->vm->AttachCurrentThread(&env, NULL))
    {
        aout << "Error: Failed to getJniEnv" << std::endl;
    }
    return env;
}

static int findActiveGameController()
{
    for(auto index=0; index<PADDLEBOAT_MAX_CONTROLLERS; ++index)
    {
        if(0 != Paddleboat_getControllerStatus(index))
        {
            return index;
        }
    }
    return -1;
}

void getControllerInfo()
{
    int controllerIndex = findActiveGameController();
    if(-1 == controllerIndex)
    {
        aout << "Info: Game controller is disconnected." << std::endl;
        return;
    }
    char controllerName[32] = {0};
    aout << "Info: Find active controller, id:" << controllerIndex << "." << std::endl;
    if(PADDLEBOAT_NO_ERROR != Paddleboat_getControllerName(controllerIndex, 32, controllerName))
    {
        aout << "Warrning: Failed to get controller name." << std::endl;
    }
    aout << "Info: Controller name: " << controllerName << std::endl;
    Paddleboat_Controller_Info controllerInfo;
    if(PADDLEBOAT_NO_ERROR != Paddleboat_getControllerInfo(controllerIndex, &controllerInfo))
    {
        aout << "Warrning: Failed to get controller info." << std::endl;
    }
    auto btLayout = controllerInfo.controllerFlags&PADDLEBOAT_CONTROLLER_LAYOUT_MASK;
    switch((Paddleboat_ControllerButtonLayout)btLayout)
    {
        case PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD:
            aout << "Info: Xbox layout" << std::endl;
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES:
            aout << "Info: PS layout" << std::endl;
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_REVERSE:
            aout << "Info: NS layout" << std::endl;
            break;
        default:
            aout << "Info: Other layout" << std::endl;
    }
#if 0
    Paddleboat_Controller_Mapping_Data mapData;
    Paddleboat_getControllerRemapTableData(1, &mapData);
    mapData.flags = PADDLEBOAT_CONTROLLER_LAYOUT_REVERSE;
    Paddleboat_addControllerRemapData(PADDLEBOAT_REMAP_ADD_MODE_DEFAULT, 1, &mapData);
#endif
}

void initControllerLib()
{
    if(PADDLEBOAT_NO_ERROR != Paddleboat_init(gJNIEnv, gApp->activity->javaGameActivity))
    {
        aout << "Error: Failed to init Game Controller lib." << std::endl;
        abort();
    }
    if(!Paddleboat_isInitialized())
    {
        aout << "Error: Failed to init Game Controller lib." << std::endl;
        abort();
    }
    Paddleboat_update(gJNIEnv);
}
/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    // Can be removed, useful to ensure your code is running
    aout << "Welcome to android_main" << std::endl;
    gApp = pApp;
    // Register an event handler for Android events
    pApp->onAppCmd = handle_cmd;

    // Set input event filters (set it to NULL if the app wants to process all inputs).
    // Note that for key inputs, this example uses the default default_key_filter()
    // implemented in android_native_app_glue.c.
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    gJNIEnv = getJniEnv(pApp);
    if(gJNIEnv == nullptr)
    {
        aout << "Error: Failed to get JNIEnv. Abort." << std::endl;
        abort();
    }

    initControllerLib();
    getControllerInfo();

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    do {
        // Process all pending events before running game logic.
        bool done = false;
        while (!done) {
            // 0 is non-blocking.
            int timeout = 0;
            int events;
            android_poll_source *pSource;
            int result = ALooper_pollOnce(timeout, nullptr, &events,
                                          reinterpret_cast<void**>(&pSource));
            switch (result) {
                case ALOOPER_POLL_TIMEOUT:
                    [[clang::fallthrough]];
                case ALOOPER_POLL_WAKE:
                    // No events occurred before the timeout or explicit wake. Stop checking for events.
                    done = true;
                    break;
                case ALOOPER_EVENT_ERROR:
                    aout << "ALooper_pollOnce returned an error" << std::endl;
                    break;
                case ALOOPER_POLL_CALLBACK:
                    break;
                default:
                    if (pSource) {
                        pSource->process(pApp, pSource);
                    }
            }
        }

    } while (!pApp->destroyRequested);
    if(Paddleboat_isInitialized())
    {
        Paddleboat_destroy(gJNIEnv);
        aout << "Info: Destory Game Controller lib." << std::endl;
    }
}
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_inputredirectionclient_1android_MainActivity_saveIPAddress(JNIEnv *env,
                            jobject thiz,
                            jstring input) {
    std::string ip = env->GetStringUTFChars(input, nullptr);
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

    if(-1 == read(fd, ip, BUFFER_SIZE))
    {
        aout << "Error: Failed to read config file." << std::endl;
        close(fd);
        return env->NewStringUTF("192.168.1.1");
    }

    close(fd);
    return env->NewStringUTF(ip);
}
