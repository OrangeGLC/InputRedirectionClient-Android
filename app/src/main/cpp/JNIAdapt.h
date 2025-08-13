//
// Created by 荆荣 on 2025/8/12.
//

#ifndef INPUTREDIRECTIONCLIENT_ANDROID_JNIADAPT_H
#define INPUTREDIRECTIONCLIENT_ANDROID_JNIADAPT_H
#include <jni.h>
JNIEnv* getJniEnv(struct android_app *pApp);
void updateUI();
#endif //INPUTREDIRECTIONCLIENT_ANDROID_JNIADAPT_H
