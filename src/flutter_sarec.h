#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#if _WIN32
#define FFI_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define FFI_PLUGIN_EXPORT
#endif


// A very short-lived native function.
//
// For very short-lived functions, it is fine to call them on the main isolate.
// They will block the Dart execution while running the native function, so
// only do this for native functions which are guaranteed to be short-lived.
FFI_PLUGIN_EXPORT intptr_t sum(intptr_t a, intptr_t b);

// A longer lived native function, which occupies the thread calling it.
//
// Do not call these kind of native functions in the main isolate. They will
// block Dart execution. This will cause dropped frames in Flutter applications.
// Instead, call these native functions on a separate isolate.
FFI_PLUGIN_EXPORT intptr_t sum_long_running(intptr_t a, intptr_t b);




FFI_PLUGIN_EXPORT void* CreateSarecClient();

FFI_PLUGIN_EXPORT intptr_t Start(void*,const char* filename);

FFI_PLUGIN_EXPORT intptr_t Pause(void*);

FFI_PLUGIN_EXPORT intptr_t Resume(void*);

FFI_PLUGIN_EXPORT intptr_t Stop(void*);

FFI_PLUGIN_EXPORT intptr_t IsRecording(void*);

FFI_PLUGIN_EXPORT intptr_t DestroySarecClient(void*);

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingFormatTag(void*);

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingChannels(void*);

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingSamplesPerSec(void*);

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingBitsPerSample(void*);

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingBlockAlign(void*);

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingAvgBytesPerSec(void*);


FFI_PLUGIN_EXPORT intptr_t GetMicRecordingFormatTag(void*);

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingChannels(void*);

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingSamplesPerSec(void*);

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingBitsPerSample(void*);

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingBlockAlign(void*);

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingAvgBytesPerSec(void*);


FFI_PLUGIN_EXPORT intptr_t IncludeMicRecording(void*,intptr_t);

FFI_PLUGIN_EXPORT intptr_t IsMicIncluded(void*);