#include <stdio.h>
#include "gl4es.h"
#include "build_info.h"
#include "logs.h"
#include "../../version.h"

void print_build_infos() {
    SHUT_LOGD("v%d.%d.%d built on %s %s", MAJOR, MINOR, REVISION, __DATE__, __TIME__);
}
