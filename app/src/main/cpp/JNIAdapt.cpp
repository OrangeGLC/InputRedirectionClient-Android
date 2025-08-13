//
// Created by 荆荣 on 2025/8/10.
//
#include "JNIAdapt.h"
#include "Transmitter.h"
#include "AndroidOut.h"
#include <unistd.h>
#include <mutex>

#define LOG_TAG "IRC"


std::mutex mutexCfgPathReady;
bool gCfgPathReady=false;
std::string gCfgPath;
struct android_app *gApp = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_initNative(JNIEnv *env,
                                                                          jobject thiz,
                                                                          jstring path)
{
    std::lock_guard<std::mutex> lock(mutexCfgPathReady);
    gCfgPath = env->GetStringUTFChars(path, nullptr);
    gCfgPathReady = true;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_saveIPAddress(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jstring input)
{
    Transmitter::GetInstance()->SetCfgIP(
            std::string(env->GetStringUTFChars(input, nullptr)));
    return env->NewStringUTF(Transmitter::GetInstance()->SaveConfig() == Transmitter::OK ? "Saved Successfully.":"Failed.");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getCfgIP(JNIEnv *env,
                                                                                 jobject thiz)
{
    char ip[20]={0};
    Transmitter::GetInstance()->GetCfgIP(ip,20);
    ALOGD("%s: ip=%s", __FUNCTION__, ip);
    return env->NewStringUTF(ip);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_handleKeyEvent(JNIEnv *env,
                                                                              jobject thiz,
                                                                              jint keyCode,
                                                                              jint action,
                                                                              jint source)
{
    GameActivityKeyEvent keyEvent;
    keyEvent.action = action;
    keyEvent.keyCode = keyCode;
    keyEvent.source = source;
    Transmitter* tr = Transmitter::GetInstance();
    if(nullptr!= tr)
    {
        tr->HandleKeyEvent(&keyEvent);
        tr->GenerateFrame();
        tr->SendFrame();
    }
}

JNIEnv* getJniEnv()
{
    if(nullptr == gApp)
    {
        ALOGE(" gApp is nullptr");
        return nullptr;
    }
    JNIEnv * env = nullptr;
    if(0 != gApp->activity->vm->AttachCurrentThread(&env, nullptr))
    {
        ALOGE(" Failed to getJniEnv");
    }
    return env;
}

void updateUI()
{
    JNIEnv* env = getJniEnv();
    if(nullptr == env)
    {
        ALOGE("Failed to get JNI env.");
        return;
    }

    jobject mainActivity = gApp->activity->javaGameActivity;
    if(nullptr == mainActivity)
    {
        ALOGE("Failed to get activity.");
        return;
    }
    jclass main = env->GetObjectClass(mainActivity);
    if (env->ExceptionCheck() || main == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "获取类失败");
        env->ExceptionClear();
        return;
    }
    jmethodID UIMethod = env->GetMethodID(main, "updateUI", "()V");
    if (env->ExceptionCheck() || UIMethod == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "方法查找失败");
        env->ExceptionClear();
        env->DeleteLocalRef(main);
        return;
    }
    env->CallVoidMethod(mainActivity, UIMethod);
    env->DeleteLocalRef(main);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_cleanNative(JNIEnv *env,
                                                                           jobject thiz)
{
}