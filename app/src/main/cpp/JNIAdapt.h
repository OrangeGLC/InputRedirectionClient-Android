//
// Created by 荆荣 on 2025/8/12.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_JNIADAPT_H
#define INPUTREDIRECTIONCLIENT_ANDROID_JNIADAPT_H
#include <jni.h>
JNIEnv* getJniEnv(struct android_app *pApp);
void updateUI();
void callOnCaptureResult(const char* n3dsName, const char* physName,
                         bool conflict, const char* conflictN3dsName);
#endif //INPUTREDIRECTIONCLIENT_ANDROID_JNIADAPT_H
