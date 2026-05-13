#ifndef MOCK_ANDROIDOUT_H
#define MOCK_ANDROIDOUT_H

#include <cstdio>
#include <cstdlib>

#define LOG_TAG "IRC"

#define ALOGD(...) do { fprintf(stdout, "D/" LOG_TAG ": " __VA_ARGS__); fputc('\n', stdout); } while(0)
#define ALOGI(...) do { fprintf(stdout, "I/" LOG_TAG ": " __VA_ARGS__); fputc('\n', stdout); } while(0)
#define ALOGW(...) do { fprintf(stdout, "W/" LOG_TAG ": " __VA_ARGS__); fputc('\n', stdout); } while(0)
#define ALOGE(...) do { fprintf(stdout, "E/" LOG_TAG ": " __VA_ARGS__); fputc('\n', stdout); } while(0)
#define ALOGF(...) do { \
    fprintf(stderr, "F/" LOG_TAG ": " __VA_ARGS__); \
    fputc('\n', stderr); \
    abort(); \
} while(0)

#endif
