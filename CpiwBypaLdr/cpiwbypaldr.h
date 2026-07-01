#pragma once

#include <KexComm.h>
#include <KxCfgHlp.h>
#include "resource.h"
#include <tlhelp32.h>

#if defined(KEX_TARGET_TYPE_EXE) || defined(KEX_TARGET_TYPE_DLL)
#  if defined(KEX_ARCH_X64)
#    pragma comment(lib, "msvcrt_x64.lib")
#  elif defined(KEX_ARCH_X86)
#    pragma comment(lib, "msvcrt_x86.lib")
#  endif
#endif

#define INITIAL_CAPACITY 1024
#define POLL_INTERVAL_MS 500