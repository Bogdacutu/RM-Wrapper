#include "rm-wrapper.h"

#include <stdarg.h>
#include <dlfcn.h>
#include <string.h>
#include <math.h>

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[%s %d] ", tag, prio);
    printf(fmt, args);
    printf("\n");
    va_end(args);
}

int str_between(const char* str, const char* prefix, const char* suffix, char* output, size_t out_size) {
    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);
    size_t suffix_len = strlen(suffix);
    
    if (strncmp(str, prefix, prefix_len) == 0 &&
        strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0) {
        size_t out_len = min(out_size - 1, str_len - prefix_len - suffix_len);
        strncpy(output, str + prefix_len, out_len);
        output[out_len] = 0;
        return out_len;
    } else return -1;
}

FILE* fopen(const char* path, const char* mode) {
    static FILE* (*_fopen)(const char*, const char*) = NULL;
    if (!_fopen)
        _fopen = dlsym(RTLD_NEXT, "fopen");
    
    char device[256];
    if (str_between(path, "/sys/class/power_supply/", "/online", device, 256) >= 0) {
        //printf("FOPEN :: HOOKING POWER DEVICE: %s\n", device);
        FILE* ret = malloc(sizeof(FILE));
        ret->_p = POWER_MAGIC;
        return ret;
    }
    
    return _fopen(path, mode);
}

size_t fread(void* ptr, size_t size, size_t count, FILE* stream) {
    static size_t (*_fread)(void*, size_t, size_t, FILE*) = NULL;
    if (!_fread)
        _fread = dlsym(RTLD_NEXT, "fread");
    
    if (stream->_p == POWER_MAGIC)
        return 0;
    
    return _fread(ptr, size, count, stream);
}

void (*ts_main_resume)();
void (*ts_main_calc)();

int main() {
    void* dl = dlopen("ts.default.so", RTLD_NOW | RTLD_GLOBAL);
    if (!dl)
        return printf("%s\n", dlerror());
    
    ts_main_resume = dlsym(dl, "ts_main_resume");
    if (!ts_main_resume)
        return printf("%s\n", dlerror());
    ts_main_calc = dlsym(dl, "ts_main_calc");
    if (!ts_main_calc)
        return printf("%s\n", dlerror());
    
    (*ts_main_resume)();
    
    while (1) {
        (*ts_main_calc)();
        usleep(5000);
    }
}
