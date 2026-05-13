#include "CppUTest/CommandLineTestRunner.h"
#include <string>
#include <mutex>
#include <cstdio>
#include <cstdarg>

// Globals normally defined in JNIAdapt.cpp
std::mutex mutexCfgPathReady;
bool gCfgPathReady = false;
std::string gCfgPath;
struct android_app *gApp = nullptr;

// Test-side tracking for JNIAdapt mock
int gUpdateUICallCount = 0;
int gCaptureResultCallCount = 0;
const char* gLastCaptureN3dsName = nullptr;
const char* gLastCapturePhysName = nullptr;
bool gLastCaptureConflict = false;
const char* gLastCaptureConflictN3dsName = nullptr;

// Stub implementations for Android NDK
extern "C" int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
    (void)prio;
    fprintf(stdout, "[%s] ", tag);
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fputc('\n', stdout);
    return 0;
}

void updateUI() {
    gUpdateUICallCount++;
}

void callOnCaptureResult(const char* n3dsName, const char* physName,
                         bool conflict, const char* conflictN3dsName) {
    gCaptureResultCallCount++;
    gLastCaptureN3dsName = n3dsName;
    gLastCapturePhysName = physName;
    gLastCaptureConflict = conflict;
    gLastCaptureConflictN3dsName = conflictN3dsName;
}

int main(int argc, char** argv) {
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
