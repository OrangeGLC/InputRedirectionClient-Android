#ifndef MOCK_ANDROID_NATIVE_APP_GLUE_H
#define MOCK_ANDROID_NATIVE_APP_GLUE_H

#include <cstdint>
#include <cstring>

// Input source constants
#define AINPUT_SOURCE_CLASS_MASK         0x000000ff
#define AINPUT_SOURCE_CLASS_POINTER      0x00000001
#define AINPUT_SOURCE_CLASS_JOYSTICK     0x00000010
#define AINPUT_SOURCE_GAMEPAD            0x00000401
#define AINPUT_SOURCE_JOYSTICK           0x01000010

// Key event constants
#define AKEY_EVENT_ACTION_DOWN           0
#define AKEY_EVENT_ACTION_UP             1
#define AKEY_EVENT_ACTION_MULTIPLE       2

// Motion axis constants
enum {
    AMOTION_EVENT_AXIS_X         = 0,
    AMOTION_EVENT_AXIS_Y         = 1,
    AMOTION_EVENT_AXIS_Z         = 11,
    AMOTION_EVENT_AXIS_RZ        = 14,
    AMOTION_EVENT_AXIS_HAT_X     = 15,
    AMOTION_EVENT_AXIS_HAT_Y     = 16,
    AMOTION_EVENT_AXIS_BRAKE     = 23,
    AMOTION_EVENT_AXIS_GAS       = 22,
    AMOTION_EVENT_AXIS_RX        = 12,
    AMOTION_EVENT_AXIS_RY        = 13,
    AMOTION_EVENT_AXIS_MAX       = 64
};

// GameActivity types
struct GameActivityKeyEvent {
    int32_t keyCode;
    int32_t scanCode;
    int32_t action;
    int32_t source;
    int32_t repeatCount;
};

struct GameActivityPointerAxes {
    float values[64];
};

struct GameActivityMotionEvent {
    uint32_t source;
    GameActivityPointerAxes pointers[1];
};

struct android_input_buffer {
    GameActivityKeyEvent* keyEvents;
    int keyEventsCount;
    GameActivityMotionEvent* motionEvents;
    uint64_t motionEventsCount;
};

struct android_poll_source {
    void (*process)(struct android_app* app, android_poll_source* source);
};

struct android_app {
    int destroyRequested;
    void* activity;
    android_poll_source* pendingSource;
};

// GameActivity stub functions
inline void GameActivityPointerAxes_enableAxis(int axis) {
    (void)axis;
}

inline float GameActivityPointerAxes_getAxisValue(GameActivityPointerAxes* axes, int axis) {
    if (axis >= 0 && axis < 64)
        return axes->values[axis];
    return 0.0f;
}

inline android_input_buffer* android_app_swap_input_buffers(android_app* app) {
    (void)app;
    return nullptr;
}

inline void android_app_clear_key_events(android_input_buffer* buf) {
    (void)buf;
}

inline void android_app_clear_motion_events(android_input_buffer* buf) {
    (void)buf;
}

inline int ALooper_pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    (void)timeoutMillis;
    (void)outFd;
    (void)outEvents;
    (void)outData;
    return 0;
}

// GameActivity filter functions (called from main.cpp, not used in tests)
inline void android_app_set_motion_event_filter(android_app* app, void* filter) {
    (void)app;
    (void)filter;
}

inline void android_app_set_key_event_filter(android_app* app, void* filter) {
    (void)app;
    (void)filter;
}

#endif
