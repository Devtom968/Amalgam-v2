#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef STRICT
#define STRICT
#endif

#ifdef __cplusplus
#ifndef EXTERN_C
#define EXTERN_C extern "C"
#endif
#else
#ifndef EXTERN_C
#define EXTERN_C extern
#endif
#endif

#ifndef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT __declspec(dllimport)
#endif

#if 1
#include <windows.h>
#endif
#if 1
#include <shellapi.h>
#endif
#if 1
#include <shlobj.h>
#endif

#pragma comment(lib, "shell32.lib")
