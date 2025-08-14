#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include "Transmitter.h"
#include "JNIAdapt.h"

extern struct android_app *gApp;
extern std::mutex mutexCfgPathReady;
extern bool gCfgPathReady;


extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

bool key_event_filter_func(const GameActivityKeyEvent *keynEvent)
{
    (void)keynEvent;
    return true;
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    ALOGD("Welcome to android_main");
    gApp = pApp;

    android_app_set_motion_event_filter(pApp, motion_event_filter_func);
    android_app_set_key_event_filter(pApp, key_event_filter_func);
    while(true)
    {
        std::lock_guard<std::mutex> lock(mutexCfgPathReady);
        if(gCfgPathReady)
            break;
    }
    Transmitter* tr = Transmitter::CreateInstance(pApp);
    if(tr)
    {
        updateUI();
        tr->TaskLoop();
        tr->DestroyInstance();
    }
}
}

