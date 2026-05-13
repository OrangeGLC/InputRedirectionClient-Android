#ifndef MOCK_JNIADAPT_H
#define MOCK_JNIADAPT_H

extern "C" {
void updateUI();
void callOnCaptureResult(const char* n3dsName, const char* physName,
                         bool conflict, const char* conflictN3dsName);
}

extern int gUpdateUICallCount;
extern int gCaptureResultCallCount;
extern const char* gLastCaptureN3dsName;
extern const char* gLastCapturePhysName;
extern bool gLastCaptureConflict;
extern const char* gLastCaptureConflictN3dsName;

#endif
