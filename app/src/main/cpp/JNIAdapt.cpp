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
    const char* ip = env->GetStringUTFChars(input, nullptr);
    Transmitter::GetInstance()->SetCfgIP(ip);
    env->ReleaseStringUTFChars(input, ip);
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
    if (env->ExceptionCheck() || main == nullptr)
    {
        ALOGE("Failed to get mainActivity");
        env->ExceptionClear();
        return;
    }
    jmethodID UIMethod = env->GetMethodID(main, "updateUI", "()V");
    if (env->ExceptionCheck() || UIMethod == nullptr)
    {
        ALOGE("Failed to find updateUI method");
        env->ExceptionClear();
        env->DeleteLocalRef(main);
        return;
    }
    env->CallVoidMethod(mainActivity, UIMethod);
    env->DeleteLocalRef(main);
}

void callOnCaptureResult(const char* n3dsName, const char* physName,
                         bool conflict, const char* conflictN3dsName)
{
    JNIEnv* env = getJniEnv();
    if(nullptr == env) return;

    jobject mainActivity = gApp->activity->javaGameActivity;
    if(nullptr == mainActivity) return;

    jclass main = env->GetObjectClass(mainActivity);
    if (env->ExceptionCheck() || main == nullptr)
    {
        env->ExceptionClear();
        return;
    }
    jmethodID method = env->GetMethodID(main, "onCaptureResult",
        "(Ljava/lang/String;Ljava/lang/String;ZLjava/lang/String;)V");
    if (env->ExceptionCheck() || method == nullptr)
    {
        env->ExceptionClear();
        env->DeleteLocalRef(main);
        return;
    }
    jstring jN3ds = env->NewStringUTF(n3dsName);
    jstring jPhys = env->NewStringUTF(physName);
    jstring jConflict = conflictN3dsName ? env->NewStringUTF(conflictN3dsName) : nullptr;
    env->CallVoidMethod(mainActivity, method, jN3ds, jPhys,
                        conflict ? JNI_TRUE : JNI_FALSE, jConflict);
    if(jN3ds) env->DeleteLocalRef(jN3ds);
    if(jPhys) env->DeleteLocalRef(jPhys);
    if(jConflict) env->DeleteLocalRef(jConflict);
    env->DeleteLocalRef(main);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_cleanNative(JNIEnv *env,
                                                                           jobject thiz)
{
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setInvertAB(JNIEnv *env,
                                                                           jobject thiz,
                                                                           jboolean flg)
{
    Transmitter::GetInstance()->SetInvertAB(flg);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setInvertXY(JNIEnv *env,
                                                                           jobject thiz,
                                                                           jboolean flg)
{
    Transmitter::GetInstance()->SetInvertXY(flg);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setTurbo(JNIEnv *env, jobject thiz,
                                                                        jint index, jboolean flg)
{
    Transmitter::GetInstance()->SetTurbo(static_cast<N3DS_KEY_INDEX>(index),flg);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getInvertAB(JNIEnv *env,
                                                                           jobject thiz)
{
    return Transmitter::GetInstance()->GetInvertAB();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getInvertXY(JNIEnv *env,
                                                                           jobject thiz)
{
    return Transmitter::GetInstance()->GetInvertXY();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getTurbo(JNIEnv *env, jobject thiz,
                                                                        jint index)
{
    return Transmitter::GetInstance()->GetTurbo(static_cast<N3DS_KEY_INDEX>(index));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setHomeMapEnable(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jboolean flg)
{
    Transmitter::GetInstance()->SetHomeMap(flg);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getHomeMapEnable(JNIEnv *env,
                                                                                jobject thiz)
{
   return Transmitter::GetInstance()->GetHomeMap();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setPowerMapEnable(JNIEnv *env,
                                                                                 jobject thiz,
                                                                                 jboolean flg)
{
    Transmitter::GetInstance()->SetPowerMap(flg);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getPowerMapEnable(JNIEnv *env,
                                                                                 jobject thiz)
{
    return Transmitter::GetInstance()->GetPowerMap();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setPowerOffMapEnable(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jboolean flg)
{
    Transmitter::GetInstance()->SetPowerOffMap(flg);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getPowerOffMapEnable(JNIEnv *env,
                                                                                    jobject thiz)
{
    return Transmitter::GetInstance()->GetPowerOffMap();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setTurboInterval(
        JNIEnv *env, jobject thiz, jint ms)
{
    Transmitter::GetInstance()->SetTurboInterval(ms);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setTurboMode(
        JNIEnv *env, jobject thiz, jint index, jboolean fullAuto)
{
    Transmitter::GetInstance()->SetTurboMode(index, fullAuto);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getTurboMode(
        JNIEnv *env, jobject thiz, jint index)
{
    return Transmitter::GetInstance()->GetTurboMode(index);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getTurboInterval(
        JNIEnv *env, jobject thiz)
{
    return Transmitter::GetInstance()->GetTurboInterval();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setKeyMapMode(
        JNIEnv *env, jobject thiz, jint mode)
{
    Transmitter::GetInstance()->SetKeyMapMode(mode);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getKeyMapMode(
        JNIEnv *env, jobject thiz)
{
    return Transmitter::GetInstance()->GetKeyMapMode();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setSwapJoysticks(
        JNIEnv *env, jobject thiz, jboolean flg)
{
    Transmitter::GetInstance()->SetSwapJoysticks(flg);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getSwapJoysticks(
        JNIEnv *env, jobject thiz)
{
    return Transmitter::GetInstance()->GetSwapJoysticks();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_setKeyMapping(
        JNIEnv *env, jobject thiz, jint inputIdx, jint targetIdx)
{
    Transmitter::GetInstance()->SetKeyMapping(inputIdx, targetIdx);
    Transmitter::GetInstance()->SaveConfig();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_getKeyMapping(
        JNIEnv *env, jobject thiz, jint inputIdx)
{
    return Transmitter::GetInstance()->GetKeyMapping(inputIdx);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_enterKeyCapture(
        JNIEnv *env, jobject thiz, jint n3dsKeyIndex)
{
    Transmitter::GetInstance()->EnterKeyCapture(n3dsKeyIndex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_exitKeyCapture(
        JNIEnv *env, jobject thiz)
{
    Transmitter::GetInstance()->ExitKeyCapture();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jingrong_inputredirectionclient_1android_MainActivity_resolveKeyConflict(
        JNIEnv *env, jobject thiz, jboolean accept)
{
    Transmitter::GetInstance()->ResolveKeyConflict(accept);
}