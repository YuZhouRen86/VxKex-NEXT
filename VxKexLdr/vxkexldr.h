#pragma once

#include <KexComm.h>
#include "resource.h"

EXTERN LANGID CURRENTLANG;
EXTERN PWSTR FRIENDLYAPPNAME;

BOOLEAN VklCreateProcess(
	IN	PCWSTR	Path,
	IN	PCWSTR	Arguments OPTIONAL);

INT_PTR CALLBACK VklDialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);