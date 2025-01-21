#include "buildcfg.h"
#include <KexComm.h>
#include <KexPathCch.h>

#define HRCHECKED(x) hr = (x); if (FAILED(hr)) goto Error

// check if path begins with \\?\ prefix and has a drive letter and colon after it
STATIC BOOL IsExtendedLengthDosDevicePath(
	IN	PCWSTR	lpszPath)
{
	if (StringBeginsWith(lpszPath, L"\\\\?\\") && isalpha(lpszPath[4]) && lpszPath[5] == ':') {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsDosDevicePath(
	IN	PCWSTR	lpszPath)
{
	if (isalpha(lpszPath[0]) && lpszPath[1] == ':' && (lpszPath[2] == '\\' || lpszPath[2] == '\0')) {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsVolumeGuidPath(
	IN	PCWSTR	lpszPath)
{
	if (StringBeginsWithI(lpszPath, L"\\\\?\\Volume{") && wcslen(&lpszPath[10]) >= 38) {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsPrefixedPath(
	IN	PCWSTR	lpszPath)
{
	return StringBeginsWith(lpszPath, L"\\\\?\\");
}

STATIC BOOL IsPrefixedUncPath(
	IN	PCWSTR	lpszPath)
{
	return StringBeginsWithI(lpszPath, L"\\\\?\\UNC\\");
}

STATIC BOOL IsFullyQualifiedPath(
	IN	PCWSTR	lpszPath)
{
	return IsDosDevicePath(lpszPath) || StringBeginsWith(lpszPath, L"\\\\");
}

// Find the last character of the root in a path, if there is one, without any segments
STATIC PCWSTR FindLastCharacterOfRoot(
	IN	OUT	PCWSTR	lpszPath)
{
	if (IsVolumeGuidPath(lpszPath)) return lpszPath[48] == '\\' ? lpszPath + 48 : lpszPath + 47;
	else if (IsPrefixedUncPath(lpszPath)) return lpszPath + 7;
	else if (IsExtendedLengthDosDevicePath(lpszPath)) return lpszPath[6] == '\\' ? lpszPath + 6 : lpszPath + 5;
	else if (lpszPath[0] == '\\' && lpszPath[1] == '\\') return lpszPath + 1;
	else if (lpszPath[0] == '\\') return lpszPath;
	else if (IsDosDevicePath(lpszPath)) return lpszPath[2] == '\\' ? lpszPath + 2 : lpszPath + 1;
	else return NULL;
}

WINPATHCCHAPI HRESULT WINAPI PathAllocCanonicalize(
	IN	PCWSTR	path_in,
	IN	DWORD	flags,
	OUT	PWSTR	*path_out)
{
	PWSTR buffer, dst;
	PCWSTR src, root_end;
	SIZE_T buffer_size, length;

	if (!path_in || !path_out
		|| ((flags & PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS) && (flags & PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS))
		|| (flags & (PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS)
			&& !(flags & PATHCCH_ALLOW_LONG_PATHS))
		|| ((flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH) && (flags & PATHCCH_ALLOW_LONG_PATHS)))
	{
		if (path_out) *path_out = NULL;
		return E_INVALIDARG;
	}

	length = wcslen(path_in);
	if ((length + 1 > MAX_PATH && !(flags & (PATHCCH_ALLOW_LONG_PATHS | PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH)))
		|| (length + 1 > PATHCCH_MAX_CCH))
	{
		*path_out = NULL;
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	}

	// PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH implies PATHCCH_DO_NOT_NORMALIZE_SEGMENTS
	if (flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH) flags |= PATHCCH_DO_NOT_NORMALIZE_SEGMENTS;

	// path length + possible \\?\ addition + possible \ addition + NUL
	buffer_size = (length + 6) * sizeof(WCHAR);
	buffer = (PWSTR) LocalAlloc(LMEM_ZEROINIT, buffer_size);
	if (!buffer)
	{
		*path_out = NULL;
		return E_OUTOFMEMORY;
	}

	src = path_in;
	dst = buffer;

	root_end = FindLastCharacterOfRoot(path_in);
	if (root_end) root_end = buffer + (root_end - path_in);

	// Copy path root
	if (root_end)
	{
		RtlCopyMemory(dst, src, (root_end - buffer + 1) * sizeof(WCHAR));
		src += root_end - buffer + 1;
		if (PathCchStripPrefix(dst, length + 6) == S_OK)
		{
			// Fill in \ in X:\ if the \ is missing
			if (IsDosDevicePath(dst) && dst[2] != '\\')
			{
				dst[2] = '\\';
				dst[3] = 0;
			}
			dst = buffer + wcslen(buffer);
			root_end = dst;
		}
		else
			dst += root_end - buffer + 1;
	}

	while (*src)
	{
		if (src[0] == '.')
		{
			if (src[1] == '.')
			{
				// Keep one . after *
				if (dst > buffer && dst[-1] == '*')
				{
					*dst++ = *src++;
					continue;
				}

				// Keep the .. if not surrounded by \.
				if ((src[2] != '\\' && src[2]) || (dst > buffer && dst[-1] != '\\'))
				{
					*dst++ = *src++;
					*dst++ = *src++;
					continue;
				}

				// Remove the \ before .. if the \ is not part of root
				if (dst > buffer && dst[-1] == '\\' && (!root_end || dst - 1 > root_end))
				{
					*--dst = '\0';
					// Remove characters until a \ is encountered
					while (dst > buffer)
					{
						if (dst[-1] == '\\')
						{
							*--dst = 0;
							break;
						}
						else
							*--dst = 0;
					}
				}
				// Remove the extra \ after .. if the \ before .. wasn't deleted
				else if (src[2] == '\\')
					src++;

				src += 2;
			}
			else
			{
				// Keep the . if not surrounded by \.
				if ((src[1] != '\\' && src[1]) || (dst > buffer && dst[-1] != '\\'))
				{
					*dst++ = *src++;
					continue;
				}

				// Remove the \ before . if the \ is not part of root
				if (dst > buffer && dst[-1] == '\\' && (!root_end || dst - 1 > root_end)) dst--;
				// Remove the extra \ after . if the \ before . wasn't deleted
				else if (src[1] == '\\')
					src++;

				src++;
			}

			// If X:\ is not complete, then complete it
			if (IsDosDevicePath(buffer) && buffer[2] != '\\')
			{
				root_end = buffer + 2;
				dst = buffer + 3;
				buffer[2] = '\\';
				// If next character is \, use the \ to fill in
				if (src[0] == '\\') src++;
			}
		}
		// Copy over
		else
			*dst++ = *src++;
	}
	// End the path
	*dst = 0;

	// Strip multiple trailing .
	if (!(flags & PATHCCH_DO_NOT_NORMALIZE_SEGMENTS))
	{
		while (dst > buffer && dst[-1] == '.')
		{
			// Keep a . after *
			if (dst - 1 > buffer && dst[-2] == '*')
				break;
			// If . follow a : at the second character, remove the . and add a \.
			else if (dst - 1 > buffer && dst[-2] == ':' && dst - 2 == buffer + 1)
				*--dst = '\\';
			else
				*--dst = 0;
		}
	}

	// If result path is empty, fill in \.
	if (!*buffer)
	{
		buffer[0] = '\\';
		buffer[1] = 0;
	}

	// Extend the path if needed
	length = wcslen(buffer);
	if (((length + 1 > MAX_PATH && IsDosDevicePath(buffer))
		|| (IsDosDevicePath(buffer) && flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH))
		&& !(flags & PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS))
	{
		RtlMoveMemory(buffer + 4, buffer, (length + 1) * sizeof(WCHAR));
		buffer[0] = '\\';
		buffer[1] = '\\';
		buffer[2] = '?';
		buffer[3] = '\\';
	}

	// Add a trailing backslash to the path if needed
	if (flags & PATHCCH_ENSURE_TRAILING_SLASH) PathCchAddBackslash(buffer, buffer_size);

	*path_out = buffer;
	return S_OK;
}

WINPATHCCHAPI HRESULT WINAPI PathAllocCombine(
	IN	PCWSTR	pszPathIn,
	IN	PCWSTR	pszMore,
	IN	ULONG	dwFlags,
	OUT	PWSTR	*ppszPathOut)
{
	SIZE_T combined_length, length2;
	WCHAR *combined_path;
	BOOL from_path2 = FALSE;
	HRESULT hr;

	if ((!pszPathIn && !pszMore) || !ppszPathOut)
	{
		if (ppszPathOut) *ppszPathOut = NULL;
		return E_INVALIDARG;
	}

	if (!pszPathIn || !pszMore) return PathAllocCanonicalize(pszPathIn ? pszMore : pszMore, dwFlags, ppszPathOut);

	// If pszMore is fully qualified, use pszMore only
	if (IsDosDevicePath(pszMore) || (pszMore[0] == '\\' && pszMore[1] == '\\'))
	{
		pszPathIn = pszMore;
		pszMore = NULL;
		from_path2 = TRUE;
	}

	length2 = pszMore ? wcslen(pszMore) : 0;
	// pszPathIn length + pszMore length + possible backslash + NULL
	combined_length = wcslen(pszPathIn) + length2 + 2;
	combined_path = (PWSTR) HeapAlloc(GetProcessHeap(), 0, combined_length * sizeof(WCHAR));
	if (!combined_path)
	{
		*ppszPathOut = NULL;
		return E_OUTOFMEMORY;
	}

	StringCchCopy(combined_path, wcslen(pszPathIn) + 1, pszPathIn);
	PathCchStripPrefix(combined_path, combined_length);
	if (from_path2) PathCchAddBackslashEx(combined_path, combined_length, NULL, NULL);

	if (pszMore && pszMore[0])
	{
		if (pszMore[0] == '\\' && pszMore[1] != '\\')
		{
			PathCchStripToRoot(combined_path, combined_length);
			pszMore++;
		}

		PathCchAddBackslashEx(combined_path, combined_length, NULL, NULL);
		StringCchCat(combined_path, wcslen(combined_path) + wcslen(pszMore) + 1, pszMore);
	}

	hr = PathAllocCanonicalize(combined_path, dwFlags, ppszPathOut);
	HeapFree(GetProcessHeap(), 0, combined_path);
	return hr;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchAddBackslash(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	return PathCchAddBackslashEx(lpszPath, cchPath, NULL, NULL);
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchAddBackslashEx(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	OUT		PWSTR	*ppszEnd OPTIONAL,
	OUT		PSIZE_T	pcchRemaining OPTIONAL)
{
	PWSTR lpszEnd;
	SIZE_T cchRemaining;
	SIZE_T cchPathLength;
	HRESULT hr;

	if (ppszEnd) {
		*ppszEnd = NULL;
	}

	if (pcchRemaining) {
		*pcchRemaining = 0;
	}

	hr = StringCchLength(lpszPath, cchPath, &cchPathLength);

	if (FAILED(hr)) {
		return STRSAFE_E_INSUFFICIENT_BUFFER;
	}

	cchRemaining = cchPath - cchPathLength;
	lpszEnd = &lpszPath[cchPathLength];

	if (cchPathLength != 0 && lpszEnd[-1] != '\\') {
		hr = StringCchCopyEx(lpszEnd, cchRemaining, L"\\", &lpszEnd, &cchRemaining, STRSAFE_NO_TRUNCATION);
	} else {
		hr = S_FALSE;
	}

	if (SUCCEEDED(hr)) {
		if (ppszEnd) {
			*ppszEnd = lpszEnd;
		}

		if (pcchRemaining) {
			*pcchRemaining = cchRemaining;
		}
	}

	return hr;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchAddExtension(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		PCWSTR	lpszExt)
{
	HRESULT hr;
	PCWSTR lpszExistingExt;
	BOOL bExtHasDot = FALSE;
	SIZE_T cchExt;

	if (!lpszPath || !cchPath || !lpszExt || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	// gather information about lpszExt
	for (cchExt = 0; lpszExt[cchExt] != '\0'; cchExt++) {
		switch (lpszExt[cchExt]) {
		case '.':
			if (cchExt == 0) {
				bExtHasDot = TRUE;
				continue;
			} // if the dot isn't at the beginning, it's invalid - fall through
		case ' ':
		case '\\':
			return E_INVALIDARG;
		}
	}

	// If lpszExt is empty, or has only a dot, return S_OK
	if (cchExt == 0 || (cchExt == 1 && bExtHasDot)) {
		return S_OK;
	}

	hr = PathCchFindExtension(lpszPath, cchPath, &lpszExistingExt);

	if (FAILED(hr)) {
		return hr;
	}

	if (*lpszExistingExt != '\0') {
		// there is already an extension
		return S_FALSE;
	}

	if (!bExtHasDot) {
		hr = StringCchCatEx(lpszPath, cchPath, L".", &lpszPath, &cchPath, STRSAFE_NO_TRUNCATION);

		if (FAILED(hr)) {
			return hr;
		}
	}

	return StringCchCatEx(lpszPath, cchPath, lpszExt, NULL, NULL, STRSAFE_NO_TRUNCATION);
}

WINPATHCCHAPI HRESULT WINAPI PathCchAppend(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		PCWSTR	lpszMore)
{
	return PathCchAppendEx(lpszPath, cchPath, lpszMore, 0);
}

WINPATHCCHAPI HRESULT WINAPI PathCchAppendEx(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		PCWSTR	lpszMore OPTIONAL,
	IN		DWORD	dwFlags)
{
	HRESULT hr;
	SIZE_T cchPathCopy;
	PWSTR lpszPathCopy;

	if (!lpszPath) {
		return E_INVALIDARG;
	}

	if (lpszMore && !PathIsUNCEx(lpszMore, NULL) && !StringBeginsWith(lpszMore, L"\\\\?\\")) {
		until (*lpszMore != '\\') {
			lpszMore++;
		}
	}

	cchPathCopy = wcslen(lpszPath) + 1;
	lpszPathCopy = StackAlloc(WCHAR, cchPathCopy);

	RtlCopyMemory(lpszPathCopy, lpszPath, cchPathCopy * sizeof(WCHAR));
	hr = PathCchCombineEx(lpszPath, cchPath, lpszPathCopy, lpszMore, dwFlags);

	return hr;
}

WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalize(
	OUT		PWSTR	lpszPathOut,
	IN		SIZE_T	cchPathOut,
	IN		PCWSTR	lpszPathIn)
{
	return PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszPathIn, PATHCCH_NONE);
}

// Test results:
// NULL														-> Access violation
// <empty string>											-> \
// ..C\a\b													-> ..C\a\b
// C														-> C
// C\..														-> \
// C\..\D:\a\b												-> \D:\a\b (Failed)
// C:														-> C:\
// C:D														-> C:D
// C:\														-> C:\
// C:\\														-> C:\\
// C:\\.													-> C:\
// C:\a\\..													-> C:\a\
// C:\a\\.\													-> C:\a\\
// C:\a\b\c\..\d											-> C:\a\b\d
// \\?\														-> \\?\
// \\?\C													-> \\?\C
// \\?\C\..\D												-> \\?\D
// \\?\C\..\D:\a\b											-> \\?\D:\a\b
// \\?\C:													-> C:\
// \\?\C:\Windows\..\Program Files							-> C:\Program Files
// \\?\Volume{00000000-0000-0000-0000-000000000000}			-> \\?\Volume{00000000-0000-0000-0000-000000000000}
// \\?\Volume{00000000-0000-0000-0000-000000000000}\		-> \\?\Volume{00000000-0000-0000-0000-000000000000}\
// \\?\Volume{00000000-0000-0000-0000-000000000000}\..\		-> \\?\Volume{00000000-0000-0000-0000-000000000000}\
// \\?\Volume{Invalid0-0000-0000-0000-000000000000}\..\		-> \\?\
// \\?\Volume{Invalid0-0000-0000-0000-000000000000}\.\		-> \\?\Volume{Invalid0-0000-0000-0000-000000000000}\
// \\a\b\c\d\e\f											-> \\a\b\c\d\e\f
// \\a\..\b\c\d\e\f											-> \\b\c\d\e\f
// \\..\a\..\b\c\d\e\f										-> \\b\c\d\e\f
// \\?\UNC\a\b\c\d											-> \\a\b\c\d
// \\?\UNC\a\b\c\d\											-> \\a\b\c\d\ 
// \\?\UNC\a\b\c\d\.										-> \\a\b\c\d
// \\?\UNC\a\b\c\d\.\										-> \\a\b\c\d\ 
// \\?\UNC\a\..\b\c\d										-> \\b\c\d
WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalizeEx(
	OUT	PWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	PCWSTR	lpszPathIn,
	IN	DWORD	dwFlags)
{
	PCWSTR lpszServer;
	PWSTR lpszOriginalPathOut = lpszPathOut;
	SIZE_T cchOriginalPathOut = cchPathOut;
	SIZE_T cchPathIn;
	BOOL bLongPathAllowed = (dwFlags & (PATHCCH_ALLOW_LONG_PATHS | PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH));
	HRESULT hr;

	if (lpszPathOut == lpszPathIn) {
		return E_INVALIDARG;
	}

	HRCHECKED(StringCchCopy(lpszOriginalPathOut, cchOriginalPathOut, L""));

	if (cchPathOut > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	cchPathIn = wcslen(lpszPathIn) + 1;

	if (cchPathIn > MAX_PATH) {
		if (cchPathIn > PATHCCH_MAX_CCH || !bLongPathAllowed) {
			return PATHCCH_E_FILENAME_TOO_LONG;
		}
	}

	if (!bLongPathAllowed && cchPathOut > MAX_PATH) {
		// limit the number of written characters, if long paths aren't permitted
		cchPathOut = MAX_PATH;
	}

	// Basically there are 6 formatting cases that we handle:
	// 1. Plain DOS-device path: C:\a\b\c (incl. stub such as "C:")
	// 2. Extended DOS-device path: \\?\C:\a\b\c (incl. stub such as "\\?\C:")
	// 3. Volume GUID path: \\?\Volume{guid}\a\b\c
	// 4. Plain UNC path: \\server\share\a\b\c
	// 5. Extended UNC path: \\?\UNC\server\share\a\b\c
	// 6. None of the above
	// In all cases except 5, we will strip away a beginning portion of the path as follows,
	// to produce a "stripped path". (Not the same as what PathCchSkipRoot does):
	// 1. Remove drive letter, colon and first backslash (if present) -> a\b\c
	// 2. Remove \\?\ prefix and treat as (1) -> a\b\c
	// 3. Remove volume GUID prefix and first backslash (if present) -> a\b\c
	// 4. Remove \\ prefix -> server\share\a\b\c
	// 5. Remove \\?\UNC prefix and first backslash (if present) -> server\share\a\b\c

	if (PathIsUNCEx(lpszPathIn, &lpszServer)) {
		// This branch handles both plain and extended UNC paths.
		lpszPathIn = lpszServer;

		if (bLongPathAllowed) {
			HRCHECKED(StringCchCopyEx(lpszPathOut, cchPathOut, L"\\\\?\\UNC\\", &lpszPathOut, &cchPathOut, 0));
		} else {
			HRCHECKED(StringCchCopyEx(lpszPathOut, cchPathOut, L"\\\\", &lpszPathOut, &cchPathOut, 0));
		}
	} else if (IsVolumeGuidPath(lpszPathIn)) {
		// Handles volume GUID paths.
		HRCHECKED(StringCchCopyNEx(lpszPathOut, cchPathOut, lpszPathIn, 49, &lpszPathOut, &cchPathOut, 0));
		lpszPathIn += 48;

		if (*lpszPathIn != '\0') {
			lpszPathIn++;
		}
	} else if (IsExtendedLengthDosDevicePath(lpszPathIn)) {
		// Handles extended DOS-device paths.
		lpszPathIn += 4; // skip past \\?\ prefix since we will decide whether to add it separately
		goto HandleDosDevicePath;
	} else if (IsDosDevicePath(lpszPathIn)) {
		// Handles plain DOS-device paths.
		WCHAR wcDriveLetter;

HandleDosDevicePath:
		wcDriveLetter = lpszPathIn[0];
		lpszPathIn += 2;

		if (*lpszPathIn != '\0') {
			lpszPathIn++;
		}

		if (bLongPathAllowed) {
			HRCHECKED(StringCchPrintfEx(lpszPathOut, cchPathOut, &lpszPathOut, &cchPathOut, 0, L"\\\\?\\%wc:\\", wcDriveLetter));
		} else {
			HRCHECKED(StringCchPrintfEx(lpszPathOut, cchPathOut, &lpszPathOut, &cchPathOut, 0, L"%wc:\\", wcDriveLetter));
		}
	}

	// Handle the "stripped path".
	while (*lpszPathIn != '\0') {
		PCWSTR lpszNextSegment = wcschr(lpszPathIn, '\\');

		if (!lpszNextSegment) {
			// This means we are already at the last path segment. Fast forward until
			// lpszSectionEnd points to the terminating null.
			lpszNextSegment = lpszPathIn;

			until (*lpszNextSegment == '\0') {
				lpszNextSegment++;
			}
		} else {
			lpszNextSegment++;
		}

		// if the current path segment is a . or .. then skip it
		if (lpszPathIn[0] == '.') {
			if (lpszPathIn[1] == '.') {
				if (lpszPathIn[2] == '\\' || lpszPathIn[2] == '\0') {
					lpszPathIn = lpszNextSegment;
					continue;
				}
			} else {
				if (lpszPathIn[1] == '\\' || lpszPathIn[1] == '\0') {
					lpszPathIn = lpszNextSegment;
					continue;
				}
			}
		}

		// if the next path segment is a .. then skip the current one and the next one
		if (lpszNextSegment[0] == '.') {
			if (lpszNextSegment[1] == '.') {
				if (lpszNextSegment[2] == '\\') {
					lpszPathIn = lpszNextSegment + 3;
					continue;
				} else if (lpszNextSegment[2] == '\0') {
					// remove the backslash from the output
					if (lpszPathOut > lpszOriginalPathOut && lpszPathOut[-1] == '\\') {
						lpszPathOut[-1] = '\0';
					}

					break;
				}
			} else if (lpszNextSegment[1] == '\0') {
				// don't append backslash if next segment is \. and the path ends there
				StringCchCopyNEx(lpszPathOut, cchPathOut, lpszPathIn, lpszNextSegment - lpszPathIn - 1, &lpszPathOut, &cchPathOut, 0);
				break;
			}
		}

		// otherwise copy the current segment to output
		StringCchCopyNEx(lpszPathOut, cchPathOut, lpszPathIn, lpszNextSegment - lpszPathIn, &lpszPathOut, &cchPathOut, 0);
		lpszPathIn = lpszNextSegment;
	}

	// Remove all trailing periods, unless preceded by a '*' character.
	// In this case, retain 1 trailing period.
	lpszPathOut--;

	if (lpszPathOut > lpszOriginalPathOut) {
		until (*lpszPathOut != '.') {
			lpszPathOut--;
		}

		if (*lpszPathOut == '*') {
			lpszPathOut += 2;
		} else {
			lpszPathOut += 1;
		}

		*lpszPathOut = '\0';
	}

	// If output path is a root (e.g. "C:"), or if the caller specifies it, ensure it has a backslash at the end.
	if ((lpszOriginalPathOut[0] && lpszOriginalPathOut[1] == ':' && lpszOriginalPathOut[2] == '\0')
		|| (dwFlags & PATHCCH_ENSURE_TRAILING_SLASH)) {
		HRCHECKED(PathCchAddBackslash(lpszOriginalPathOut, cchOriginalPathOut));
	} else if (*lpszOriginalPathOut == '\0') {
		// If the output path is empty, replace it with "\"
		HRCHECKED(StringCchCopy(lpszOriginalPathOut, cchOriginalPathOut, L"\\"));
	}

	return S_OK;

Error:
	StringCchCopy(lpszOriginalPathOut, cchOriginalPathOut, L"");
	return hr;
}

WINPATHCCHAPI HRESULT WINAPI PathCchCombine(
	OUT	PWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	PCWSTR	lpszPathIn OPTIONAL,
	IN	PCWSTR	lpszMore OPTIONAL)
{
	return PathCchCombineEx(lpszPathOut, cchPathOut, lpszPathIn, lpszMore, 0);
}

WINPATHCCHAPI HRESULT WINAPI PathCchCombineEx(
	OUT	PWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	PCWSTR	lpszPathIn OPTIONAL,
	IN	PCWSTR	lpszMore OPTIONAL,
	IN	DWORD	dwFlags)
{
	HRESULT hr;
	PWSTR ppszPathOut;
	SIZE_T cchLength;

	if (!lpszPathOut || !cchPathOut || cchPathOut > PATHCCH_MAX_CCH) return E_INVALIDARG;

	hr = PathAllocCombine(lpszPathIn, lpszMore, dwFlags, &ppszPathOut);
	if (FAILED(hr))
	{
		lpszPathOut[0] = 0;
		return hr;
	}
	cchLength = wcslen(ppszPathOut);
	if (cchLength + 1 > cchPathOut)
	{
		lpszPathOut[0] = 0;
		LocalFree(ppszPathOut);
		return STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	else
	{
		StringCchCopy(lpszPathOut, cchLength + 1, ppszPathOut);
		LocalFree(ppszPathOut);
		return S_OK;
	}
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchFindExtension(
	IN	PCWSTR	lpszPath,
	IN	SIZE_T	cchPath,
	OUT	PCWSTR	*ppszExt)
{
	PCWSTR lpszEnd;
	PCWSTR lpszTempExt;

	*ppszExt = NULL;

	if (!lpszPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	lpszEnd = &lpszPath[cchPath];
	lpszTempExt = NULL;

	if (lpszPath >= lpszEnd) {
		return E_INVALIDARG;
	}

	do {
		if (*lpszPath == '\0') {
			break;
		}

		if (*lpszPath == '.') {
			lpszTempExt = lpszPath;
		} else if (*lpszPath == '\\' || *lpszPath == ' ') {
			lpszTempExt = NULL;
		}

		lpszPath++;
	} while (lpszPath < lpszEnd);

	if (lpszPath >= lpszEnd) {
		return E_INVALIDARG;
	}

	if (!lpszTempExt) {
		// point the extension to the null-terminator at the end of the path
		lpszTempExt = lpszPath;
	}

	*ppszExt = lpszTempExt;
	return S_OK;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI BOOL WINAPI PathCchIsRoot(
	IN	PCWSTR	lpszPath)
{
	PCWSTR lpszServer;

	if (!lpszPath || *lpszPath == '\0') {
		return FALSE;
	}

	if (lpszPath[0] == '\\' && lpszPath[1] == '\0') {
		return TRUE;
	}

	if (IsVolumeGuidPath(lpszPath)) {
		if (lpszPath[48] == '\\' && lpszPath[49] == '\0') {
			return TRUE;
		} else {
			return FALSE;
		}
	}

	if (IsExtendedLengthDosDevicePath(lpszPath)) {
		lpszPath = &lpszPath[4];
		goto ProcessDosPath;
	}

	if (PathIsUNCEx(lpszPath, &lpszServer)) {
		// fast forward to either a) end of string, or b) a backslash
		while (*lpszServer && *lpszServer != '\\') lpszServer++;

		if (lpszServer[0] == '\0') {
			return TRUE;
		}

		if (lpszServer[0] == '\\' && lpszServer[1] != '\0') {
			while (*++lpszServer) {
				if (*lpszServer == '\\') {
					return FALSE;
				}
			}

			return TRUE;
		}
	} else {
ProcessDosPath:
		if (isalpha(lpszPath[0]) && lpszPath[1] == ':' && lpszPath[2] == '\\' && lpszPath[3] == '\0') {
			return TRUE;
		}
	}

	return FALSE;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveBackslash(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	return PathCchRemoveBackslashEx(lpszPath, cchPath, NULL, NULL);
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveBackslashEx(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	OUT		PWSTR	*ppszEnd OPTIONAL,
	OUT		PSIZE_T	pcchRemaining OPTIONAL)
{
	HRESULT hr;
	SIZE_T cch;

	hr = StringCchLength(lpszPath, cchPath, &cch);

	if (FAILED(hr)) {
		goto Error;
	}

	if (*lpszPath == '\0') {
		if (ppszEnd) {
			*ppszEnd = lpszPath;
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath;
		}

		return S_FALSE;
	}

	if (PathCchIsRoot(lpszPath)) {
		if (lpszPath[cch - 1] != '\\') {
			goto NoBackslash;
		}

		if (ppszEnd) {
			*ppszEnd = &lpszPath[cch - 1];
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath - cch + 1;
		}

		return S_FALSE;
	}

	if (lpszPath[cch - 1] == '\\') {
		lpszPath[cch - 1] = '\0';
		
		if (ppszEnd) {
			*ppszEnd = &lpszPath[cch - 1];
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath - cch + 1;
		}

		return S_OK;
	} else {
NoBackslash:
		if (ppszEnd) {
			*ppszEnd = &lpszPath[cch];
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath - cch;
		}

		return S_FALSE;
	}

Error:
	if (ppszEnd) {
		*ppszEnd = NULL;
	}

	if (pcchRemaining) {
		*pcchRemaining = 0;
	}

	return E_INVALIDARG;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveExtension(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	HRESULT hr;
	PWSTR lpszExt;

	if (!lpszPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	// Limit non-\\?\ paths to a maximum length of MAX_PATH
	if (wcsncmp(lpszPath, L"\\\\?\\", 4) && cchPath > MAX_PATH) {
		cchPath = MAX_PATH;
	}

	hr = PathCchFindExtension(lpszPath, cchPath, &lpszExt);

	if (FAILED(hr)) {
		return hr;
	}

	if (*lpszExt != '\0') {
		// extension was removed
		*lpszExt = '\0';
		return S_OK;
	} else {
		// extension was not found and not removed
		return S_FALSE;
	}
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveFileSpec(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	PCWSTR lpszPathBufEnd;
	PWSTR lpszPtr;
	PWSTR lpszBackslash;
	PWSTR lpszRootEnd;
	PWSTR lpszPathOriginal = lpszPath;
	HRESULT hr;

	if (!lpszPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	hr = PathCchSkipRoot(lpszPath, &lpszRootEnd);

	// if path has a root, skip past it
	if (SUCCEEDED(hr)) {
		lpszPath = lpszRootEnd;
	}

	lpszPathBufEnd = &lpszPath[cchPath];

	if (lpszPath >= lpszPathBufEnd) {
		return E_INVALIDARG;
	}

	for (lpszPtr = lpszPath;; lpszPtr = lpszBackslash + 1) {
		lpszBackslash = wcschr(lpszPtr, '\\');

		if (!lpszBackslash) {
			break;
		}

		lpszPath = lpszBackslash;

		if (lpszBackslash >= lpszPathBufEnd) {
			return E_INVALIDARG;
		}
	}

	if (*lpszPath != '\0') {
		*lpszPath = '\0';
		hr = PathCchRemoveBackslash(lpszPathOriginal, cchPath);

		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	} else {
		return PathCchRemoveBackslash(lpszPathOriginal, cchPath);
	}
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRenameExtension(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		PCWSTR	lpszExt)
{
	HRESULT hr;

	hr = PathCchRemoveExtension(lpszPath, cchPath);
	if (FAILED(hr)) return hr;

	hr = PathCchAddExtension(lpszPath, cchPath, lpszExt);
	if (FAILED(hr)) return hr;

	return S_OK;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
// Examples:
// C:\Windows				-> S_OK, Windows
// C:\						-> S_OK, <empty string>
// C:						-> S_OK, <empty string>
// C						-> E_INVALIDARG, NULL
// <empty string>			-> E_INVALIDARG, NULL
// \						-> S_OK, <empty string>
// \whatever				-> S_OK, whatever
// \\whatever				-> S_OK, <empty string>
// \\\whatever				-> S_OK, <empty string>
// \\\s1\s2\s3				-> S_OK, s2\s3
// \\server\share			-> S_OK, <empty string>
// \\server\share\1\2\3		-> S_OK, 1\2\3
// \\?\C:\Windows			-> S_OK, Windows
// \\?\C					-> E_INVALIDARG, NULL
// \\?\UNC					-> E_INVALIDARG, NULL
// \\?\UNC\					-> S_OK, <empty string>
// \\?\UNC\server\share		-> S_OK, <empty string>
// \\?\UNC\server\share\1\2	-> S_OK, 1\2
// \\garbage\C:\Windows		-> S_OK, Windows
// \\s1\s2\C:\Windows		-> S_OK, C:\Windows
// \\s1\s2\s3\C:\Windows	-> S_OK, s3\C:\Windows
// 
WINPATHCCHAPI HRESULT WINAPI PathCchSkipRoot(
	IN	PCWSTR	lpszPath,
	OUT	PCWSTR	*ppszRootEnd)
{
	if (!lpszPath || !ppszRootEnd) {
		return E_INVALIDARG;
	}

	if (lpszPath[0] == '\\') {
		if (lpszPath[1] == '\\') {
			if (lpszPath[2] == '?') {
				if (lpszPath[3] == '\\') {
					// there are three possibilities here
					// either the following characters are "UNC\" (in which case we will process
					// the remaining string as a \\server\share\... path)
					// or the following characters are "Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
					// or we will simply process the string after \\?\ as a normal DOS-style path
					if (StringBeginsWithI(&lpszPath[4], L"UNC\\")) {
						lpszPath = &lpszPath[8];
						goto ProcessServerSharePath;
					} else if (StringBeginsWithI(&lpszPath[4], L"Volume{") && wcslen(&lpszPath[10]) >= 38) {
						if (lpszPath[48] == '\\') {
							*ppszRootEnd = &lpszPath[49];
						} else {
							*ppszRootEnd = &lpszPath[48];
						}

						return S_OK;
					} else {
						lpszPath = &lpszPath[4];
						goto ProcessDosPath;
					}
				}
			} else {
				// consume up to two more backslashes and then return the string after that
				SIZE_T i;
				INT nBackslashes;
				lpszPath += 2;

ProcessServerSharePath:
				i = 0;
				nBackslashes = 0;

				while (lpszPath[i] != '\0') {
					if (lpszPath[i++] == '\\') {
						if (++nBackslashes == 2) {
							break;
						}

						if (nBackslashes == 1 && lpszPath[i] == '\\') {
							// Apparently we're not supposed to skip over the "share" part of the
							// UNC path if it's empty. Dunno why but Windows 10 does it.
							break;
						}
					}
				}

				*ppszRootEnd = &lpszPath[i];
				return S_OK;
			}
		} else {
			// simply return the string after the first backslash
			*ppszRootEnd = &lpszPath[1];
			return S_OK;
		}
	} else {
ProcessDosPath:
		// the path must begin with a single [a-zA-Z] char, immediately followed by a colon ':'
		if (isalpha(lpszPath[0]) && lpszPath[1] == ':') {
			if (lpszPath[2] == '\\') {
				// if there is a backslash immediately after the colon,
				// return the string directly after it.
				// Note: this rule does not apply to forward slashes, since
				// it isn't like that in Win10. Maybe it's a bug.
				*ppszRootEnd = &lpszPath[3];
			} else {
				// otherwise, return the string directly after the colon
				*ppszRootEnd = &lpszPath[2];
			}

			return S_OK;
		}
	}

	*ppszRootEnd = NULL;
	return E_INVALIDARG;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchStripPrefix(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	HRESULT hr;

	if (!lpszPath || !cchPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	if (IsExtendedLengthDosDevicePath(lpszPath)) {
		hr = StringCchCopyW(lpszPath, cchPath, &lpszPath[4]);
	} else if (IsPrefixedUncPath(lpszPath)) {
		hr = StringCchCopyW(&lpszPath[2], cchPath - 2, &lpszPath[8]);
	} else {
		return S_FALSE;
	}

	if (SUCCEEDED(hr)) {
		return S_OK;
	} else {
		return E_INVALIDARG;
	}
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchStripToRoot(
	IN OUT	PWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	PWSTR lpszAfterRoot;
	PWSTR lpszServerPart;
	HRESULT hr;

	if (!lpszPath || !cchPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	hr = PathCchSkipRoot(lpszPath, &lpszAfterRoot);

	if (FAILED(hr)) {
		return hr;
	}

	if (lpszAfterRoot >= &lpszPath[cchPath]) {
		return E_INVALIDARG;
	}

	if (PathIsUNCEx(lpszPath, &lpszServerPart) && *lpszServerPart && lpszAfterRoot[-1] == '\\') {
		lpszAfterRoot[-1] = '\0';
	} else if (lpszAfterRoot[0] != '\0') {
		lpszAfterRoot[0] = '\0';
	} else {
		return S_FALSE;
	}

	return S_OK;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
// Examples taken from testing on a Windows 10 system:
// C:\Windows				-> FALSE, NULL
// \\server\share\whatever	-> TRUE, server\share\whatever
// \\?\C:\Users				-> FALSE, NULL
// \\?\UNC					-> FALSE, NULL
// \\?\UNC\					-> FALSE, <empty string>
// \\?\UNC\Whatever			-> TRUE, Whatever
// \\.\UNC\Whatever			-> TRUE, .\UNC\Whatever
WINPATHCCHAPI BOOL WINAPI PathIsUNCEx(
	IN	PCWSTR	lpszPath,
	OUT	PCWSTR	*ppszServer OPTIONAL)
{
	// Windows 10 does not check whether lpszPath is NULL.
	// If it is, we will simply AV. "Not my problem"

	if (lpszPath[0] == '\\' && lpszPath[1] == '\\') {
		if (lpszPath[2] == '?') {
			if (StringBeginsWithI(&lpszPath[3], L"\\UNC\\")) {
				if (ppszServer) {
					*ppszServer = &lpszPath[8];
				}

				return TRUE;
			}
		} else {
			if (ppszServer) {
				*ppszServer = &lpszPath[2];
			}

			return TRUE;
		}
	}

	if (ppszServer) {
		*ppszServer = NULL;
	}

	return FALSE;
}