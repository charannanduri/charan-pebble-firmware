#pragma once

// Stub for applib_malloc.auto.h
// Redirect to pbl_malloc.h which contains app_malloc, app_free, etc.
#include "pbl_malloc.h"

// It's possible this file might also define some of these applib_ prefixed macros/functions.
// For now, we'll map them directly to the app_ variants. If linking errors occur for
// symbols like `applib_malloc`, we might need to add actual function wrappers or macros here.

/*
#ifndef applib_malloc
#define applib_malloc app_malloc
#endif

#ifndef applib_free
#define applib_free app_free
#endif

#ifndef applib_realloc
#define applib_realloc app_realloc
#endif

#ifndef applib_calloc
#define applib_calloc app_calloc
#endif

#ifndef applib_zalloc
#define applib_zalloc app_zalloc
#endif
*/ 