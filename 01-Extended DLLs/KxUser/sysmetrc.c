#include "buildcfg.h"
#include "kxuserp.h"

INT WINAPI GetSystemMetricsForDpi(
	IN	INT		Index,
	IN	UINT	Dpi)
{
	HMODULE User32;
	INT (WINAPI *pGetSystemMetricsForDpi) (INT, UINT);
	INT Value;
	
	User32 = LoadSystemLibrary(L"user32.dll");
	pGetSystemMetricsForDpi = (INT (WINAPI *) (INT, UINT)) GetProcAddress(User32, "GetSystemMetricsForDpi");
	if (pGetSystemMetricsForDpi) {
		Value = pGetSystemMetricsForDpi(Index, Dpi);
		FreeLibrary(User32);
		return Value;
	}
	FreeLibrary(User32);

	Value = GetSystemMetrics(Index);

	switch (Index) {
	case SM_CXVSCROLL:
	case SM_CYHSCROLL:
	case SM_CYCAPTION:
	case SM_CYVTHUMB:
	case SM_CXHTHUMB:
	case SM_CXICON:
	case SM_CYICON:
	case SM_CXCURSOR:
	case SM_CYCURSOR:
	case SM_CYMENU:
	case SM_CYVSCROLL:
	case SM_CXHSCROLL:
	case SM_CXMIN:
	case SM_CXMINTRACK:
	case SM_CYMIN:
	case SM_CYMINTRACK:
	case SM_CXSIZE:
	case SM_CXFRAME:
	case SM_CYFRAME:
	case SM_CXICONSPACING:
	case SM_CYICONSPACING:
	case SM_CXSMICON:
	case SM_CYSMICON:
	case SM_CYSMCAPTION:
	case SM_CXSMSIZE:
	case SM_CYSMSIZE:
	case SM_CXMENUSIZE:
	case SM_CYMENUSIZE:
	case SM_CXMENUCHECK:
	case SM_CYMENUCHECK:
		// These are pixel values that have to be scaled according to DPI.
		{
			UINT SystemDpi = GetDpiForSystem();
			if (SystemDpi == 0) SystemDpi = USER_DEFAULT_SCREEN_DPI;
			Value *= Dpi;
			Value /= SystemDpi;
		}
		break;
	}

	return Value;
}

BOOL WINAPI SystemParametersInfoForDpi(
	IN		UINT	Action,
	IN		UINT	Parameter,
	IN OUT	PVOID	Data,
	IN		UINT	WinIni,
	IN		UINT	Dpi)
{
	HMODULE User32;
	BOOL (WINAPI *pSystemParametersInfoForDpi) (UINT, UINT, PVOID, UINT, UINT);
	INT SystemDpi = GetDpiForSystem();
	
	User32 = LoadSystemLibrary(L"user32.dll");
	pSystemParametersInfoForDpi = (BOOL (WINAPI *) (UINT, UINT, PVOID, UINT, UINT)) GetProcAddress(User32, "SystemParametersInfoForDpi");
	if (pSystemParametersInfoForDpi) {
		BOOL Success = pSystemParametersInfoForDpi(Action, Parameter, Data, WinIni, Dpi);
		FreeLibrary(User32);
		return Success;
	}
	FreeLibrary(User32);
	
	if (SystemDpi == 0) SystemDpi = USER_DEFAULT_SCREEN_DPI;

	switch (Action) {
	case SPI_GETICONTITLELOGFONT:
		{
			BOOL Success;
			PLOGFONT LogFont;

			Success = SystemParametersInfo(Action, Parameter, Data, 0);

			if (Success) {
				LogFont = (PLOGFONT) Data;

				LogFont->lfWidth *= Dpi;
				LogFont->lfHeight *= Dpi;
				LogFont->lfWidth /= SystemDpi;
				LogFont->lfHeight /= SystemDpi;
			}

			return Success;
		}
	case SPI_GETICONMETRICS:
		{
			BOOL Success;
			PICONMETRICS IconMetrics;

			Success = SystemParametersInfo(Action, Parameter, Data, 0);

			if (Success) {
				IconMetrics = (PICONMETRICS) Data;

				IconMetrics->iHorzSpacing *= Dpi;
				IconMetrics->iVertSpacing *= Dpi;
				IconMetrics->iHorzSpacing /= SystemDpi;
				IconMetrics->iVertSpacing /= SystemDpi;
			}

			return Success;
		}
	case SPI_GETNONCLIENTMETRICS:
		{
			BOOL Success;
			PNONCLIENTMETRICS NonClientMetrics;

			Success = SystemParametersInfo(Action, Parameter, Data, 0);

			if (Success) {
				NonClientMetrics = (PNONCLIENTMETRICS) Data;

				NonClientMetrics->iBorderWidth			*= Dpi;
				NonClientMetrics->iScrollWidth			*= Dpi;
				NonClientMetrics->iScrollHeight			*= Dpi;
				NonClientMetrics->iCaptionWidth			*= Dpi;
				NonClientMetrics->iCaptionHeight		*= Dpi;
				NonClientMetrics->iSmCaptionWidth		*= Dpi;
				NonClientMetrics->iSmCaptionHeight		*= Dpi;
				NonClientMetrics->iMenuWidth			*= Dpi;
				NonClientMetrics->iMenuHeight			*= Dpi;
				NonClientMetrics->iPaddedBorderWidth	*= Dpi;

				NonClientMetrics->lfCaptionFont.lfWidth		*= Dpi;
				NonClientMetrics->lfCaptionFont.lfHeight	*= Dpi;
				NonClientMetrics->lfMenuFont.lfWidth		*= Dpi;
				NonClientMetrics->lfMenuFont.lfHeight		*= Dpi;
				NonClientMetrics->lfMessageFont.lfWidth		*= Dpi;
				NonClientMetrics->lfMessageFont.lfHeight	*= Dpi;
				NonClientMetrics->lfSmCaptionFont.lfWidth	*= Dpi;
				NonClientMetrics->lfSmCaptionFont.lfHeight	*= Dpi;
				NonClientMetrics->lfStatusFont.lfWidth		*= Dpi;
				NonClientMetrics->lfStatusFont.lfHeight		*= Dpi;

				NonClientMetrics->iBorderWidth			/= SystemDpi;
				NonClientMetrics->iScrollWidth			/= SystemDpi;
				NonClientMetrics->iScrollHeight			/= SystemDpi;
				NonClientMetrics->iCaptionWidth			/= SystemDpi;
				NonClientMetrics->iCaptionHeight		/= SystemDpi;
				NonClientMetrics->iSmCaptionWidth		/= SystemDpi;
				NonClientMetrics->iSmCaptionHeight		/= SystemDpi;
				NonClientMetrics->iMenuWidth			/= SystemDpi;
				NonClientMetrics->iMenuHeight			/= SystemDpi;
				NonClientMetrics->iPaddedBorderWidth	/= SystemDpi;
				
				NonClientMetrics->lfCaptionFont.lfWidth		/= SystemDpi;
				NonClientMetrics->lfCaptionFont.lfHeight	/= SystemDpi;
				NonClientMetrics->lfMenuFont.lfWidth		/= SystemDpi;
				NonClientMetrics->lfMenuFont.lfHeight		/= SystemDpi;
				NonClientMetrics->lfMessageFont.lfWidth		/= SystemDpi;
				NonClientMetrics->lfMessageFont.lfHeight	/= SystemDpi;
				NonClientMetrics->lfSmCaptionFont.lfWidth	/= SystemDpi;
				NonClientMetrics->lfSmCaptionFont.lfHeight	/= SystemDpi;
				NonClientMetrics->lfStatusFont.lfWidth		/= SystemDpi;
				NonClientMetrics->lfStatusFont.lfHeight		/= SystemDpi;
			}

			return Success;
		}
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
}