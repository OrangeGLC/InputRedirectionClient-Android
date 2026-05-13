#ifndef MOCK_JNI_H
#define MOCK_JNI_H

#include <cstdint>

typedef uint8_t jboolean;
typedef int32_t jint;
typedef int64_t jlong;
typedef float jfloat;
typedef double jdouble;
typedef signed char jbyte;
typedef unsigned short jchar;
typedef short jshort;

class _jobject {};
typedef _jobject* jobject;

class _jclass {};
typedef _jclass* jclass;

class _jmethodID {};
typedef _jmethodID* jmethodID;

class _jstring {};
typedef _jstring* jstring;

class _JNIEnv {};
typedef _JNIEnv JNIEnv;

class _JavaVM {};
typedef _JavaVM JavaVM;

#define JNIEXPORT
#define JNICALL
#define JNI_TRUE 1
#define JNI_FALSE 0
#define JNI_OK 0

#endif
