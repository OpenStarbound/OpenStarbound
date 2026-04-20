#ifndef _GL4ES_DEBUG_H_
#define _GL4ES_DEBUG_H_

#include "gles.h"

#define WAIT_FOR_ENTER()                                                       \
    do {                                                                       \
        FILE *file = fopen("/sdcard/gl_wait_for_enter.txt", "w");              \
        if (file) {                                                            \
            fprintf(file, "1");                                                \
            fclose(file);                                                      \
        } else {                                                               \
            SHUT_LOGE("Failed to create gl_wait_for_enter.txt");               \
        }                                                                      \
        while (1) {                                                            \
            file = fopen("/sdcard/gl_wait_for_enter.txt", "r");                \
            if (file) {                                                        \
                char buffer[16] = {0};                                         \
                fread(buffer, 1, sizeof(buffer) - 1, file);                    \
                fclose(file);                                                  \
                if (buffer[0] != '1') {                                        \
                    break;                                                     \
                }                                                              \
            } else {                                                           \
                SHUT_LOGE("Failed to read gl_wait_for_enter.txt");                \
            }                                                                  \
        }                                                                      \
    } while (0)

const char* PrintEnum(GLenum what);

const char* PrintEGLError(int onlyerror);

void CheckGLError(int fwd);

#endif // _GL4ES_DEBUG_H_
