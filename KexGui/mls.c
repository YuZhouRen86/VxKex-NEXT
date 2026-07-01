///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     mls.c
//
// Abstract:
//
//     User interface multi-language support functions.
//
// Author:
//
//     vxiiduu (20-May-2026)
//
// Environment:
//
//     Win32 graphical incl. KexSetup
//
// Revision History:
//
//     vxiiduu               20-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>
#include <KexMls.h>

STATIC BOOLEAN MlsgpUserLanguageIsEnglish(
	VOID)
{
	STATIC BOOLEAN AlreadyInitialized = FALSE;
	STATIC BOOLEAN UserLanguageIsEnglish = FALSE;
	NTSTATUS Status;
	LANGID LangId;

	if (AlreadyInitialized) {
		return UserLanguageIsEnglish;
	}

	Status = MlsInitialize();

	if (NT_SUCCESS(Status)) {
		Status = MlsGetCurrentLangId(&LangId);

		if (NT_SUCCESS(Status) && LangId == LANG_ENGLISH) {
			UserLanguageIsEnglish = TRUE;
		}
	}

	AlreadyInitialized = TRUE;
	return UserLanguageIsEnglish;
}

//
// Translate all static user-interface text inside a window or control to
// the current MLS language.
// Return TRUE if everything is OK, and FALSE if an error occurred.
//

STATIC BOOLEAN MlsgpTranslateWindowNoRecursion(
	IN	HWND	Window,
	IN	BOOLEAN	Force)
{
	BOOLEAN Success;
	WCHAR ClassName[64];
	WCHAR WindowText[512];
	ULONG WindowTextCch;

	ASSERT (IsWindow(Window));

	Success = GetClassName(Window, ClassName, ARRAYSIZE(ClassName));
	if (!Success) {
		return FALSE;
	}

	if (Force ||
		StringEqual(ClassName, WC_BUTTON) ||
		StringEqual(ClassName, WC_STATIC)) {

		//
		// Buttons and statics - just translate the window text.
		// The Button window class includes pushbuttons, checkboxes, radio
		// buttons, and group boxes.
		// Isn't it weird that group boxes are a type of button?
		//

		WindowTextCch = GetWindowText(Window, WindowText, ARRAYSIZE(WindowText));

		if (WindowTextCch == 0) {
			ULONG LastError;

			//
			// No text in window. Cannot translate it.
			//

			LastError = GetLastError();
			ASSERT (LastError != ERROR_INSUFFICIENT_BUFFER);
			ASSERT (LastError != ERROR_BUFFER_OVERFLOW);

			if (LastError != ERROR_SUCCESS) {
				return FALSE;
			}

			return TRUE;
		}
	} else {
		// Nothing needs to be done, because we don't want to translate
		// this kind of window. It might be e.g. an edit control.
		return TRUE;
	}

	Success = SetWindowText(Window, MlsMapString(WindowText));
	ASSERT (Success);

	return Success;
}

//
// Translate a single window without recursing into its children.
//

KEXGDECLSPEC BOOLEAN KEXGAPI MlsgTranslateWindowNoRecursion(
	IN	HWND	Window)
{
	ASSERT (IsWindow(Window));

	if (MlsgpUserLanguageIsEnglish()) {
		// No need to translate English
		return TRUE;
	}

	return MlsgpTranslateWindowNoRecursion(Window, TRUE);
}

STATIC BOOL CALLBACK MlsgpEnumChildWindowsProc(
	IN	HWND	Window,
	IN	LPARAM	LParam)
{
	MlsgpTranslateWindowNoRecursion(Window, FALSE);
	return TRUE;
}

//
// Translate a top-level window and all of its children.
//

KEXGDECLSPEC BOOLEAN KEXGAPI MlsgTranslateWindow(
	IN	HWND	Window)
{
	ASSERT (IsWindow(Window));

	if (MlsgpUserLanguageIsEnglish()) {
		// No need to translate English
		return TRUE;
	}

	// Forcibly translate the application window/dialog title.
	MlsgpTranslateWindowNoRecursion(Window, TRUE);

	// Translate the child windows/controls recursively.
	EnumChildWindows(Window, MlsgpEnumChildWindowsProc, 0);
	return TRUE;
}