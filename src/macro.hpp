#pragma once

#include <stdio.h>
#include <stdlib.h>

//@TODO use tinyprintf
#define LUX_LOG(fmt, ...) { \
    printf("%s(): " fmt "\n", __func__ __VA_OPT__(,) __VA_ARGS__); }

#define LUX_FATAL(fmt, ...) { \
    fprintf(stderr, "FATAL %s(): " fmt "\n", \
            __func__ __VA_OPT__(,) __VA_ARGS__); \
    quick_exit(EXIT_FAILURE); }

#define LUX_PANIC(fmt, ...) { \
    fprintf(stderr, "PANIC %s(): " fmt "\n", \
            __func__ __VA_OPT__(,) __VA_ARGS__); \
    abort(); }

//we don't want our code to call assert from <assert.h>
#undef assert

#ifndef NDEBUG
    #define LUX_ASSERT(expr) { \
        if(!(expr)) LUX_PANIC("assertion `" #expr "` failed"); }
#else
    #define LUX_ASSERT(expr) do {} while(false)
#endif

