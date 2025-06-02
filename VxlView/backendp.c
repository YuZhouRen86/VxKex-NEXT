#include "vxlview.h"
#include <DbgHelp.h>
#include "resource.h"
#include "backendp.h"

//
// This file contains private functions of the backend.
//

#define FSRTL_FAT_LEGAL 0x01
#define FSRTL_HPFS_LEGAL 0x02
#define FSRTL_NTFS_LEGAL 0x04
#define FSRTL_WILD_CHARACTER 0x08
#define FSRTL_OLE_LEGAL 0x10

static const UCHAR LegalAnsiCharacterArray[] = {
	0,                                                                          /* CTRL+@, 0x00 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+A, 0x01 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+B, 0x02 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+C, 0x03 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+D, 0x04 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+E, 0x05 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+F, 0x06 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+G, 0x07 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+H, 0x08 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+I, 0x09 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+J, 0x0a */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+K, 0x0b */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+L, 0x0c */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+M, 0x0d */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+N, 0x0e */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+O, 0x0f */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+P, 0x10 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+Q, 0x11 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+R, 0x12 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+S, 0x13 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+T, 0x14 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+U, 0x15 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+V, 0x16 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+W, 0x17 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+X, 0x18 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+Y, 0x19 */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+Z, 0x1a */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+[, 0x1b */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+\, 0x1c */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+], 0x1d */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+^, 0x1e */
	FSRTL_OLE_LEGAL,                                                            /* CTRL+_, 0x1f */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ` ',    0x20 */
	FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `!',    0x21 */
	FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `"',    0x22 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `#',    0x23 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `$',    0x24 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `%',    0x25 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `&',    0x26 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `'',    0x27 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `(',    0x28 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `)',    0x29 */
	FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `*',    0x2a */
	FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `+',    0x2b */
	FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `,',    0x2c */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `-',    0x2d */
	FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `.',    0x2e */
	0,                                                                          /* `/',    0x2f */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `0',    0x30 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `1',    0x31 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `2',    0x32 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `3',    0x33 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `4',    0x34 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `5',    0x35 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `6',    0x36 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `7',    0x37 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `8',    0x38 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `9',    0x39 */
	FSRTL_NTFS_LEGAL,                                                           /* `:',    0x3a */
	FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `;',    0x3b */
	FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `<',    0x3c */
	FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `=',    0x3d */
	FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `>',    0x3e */
	FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `?',    0x3f */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `@',    0x40 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `A',    0x41 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `B',    0x42 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `C',    0x43 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `D',    0x44 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `E',    0x45 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `F',    0x46 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `G',    0x47 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `H',    0x48 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `I',    0x49 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `J',    0x4a */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `K',    0x4b */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `L',    0x4c */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `M',    0x4d */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `N',    0x4e */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `O',    0x4f */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `P',    0x50 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Q',    0x51 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `R',    0x52 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `S',    0x53 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `T',    0x54 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `U',    0x55 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `V',    0x56 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `W',    0x57 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `X',    0x58 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Y',    0x59 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Z',    0x5a */
	FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `[',    0x5b */
	0,                                                                          /* `\',    0x5c */
	FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `]',    0x5d */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `^',    0x5e */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `_',    0x5f */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ``',    0x60 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `a',    0x61 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `b',    0x62 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `c',    0x63 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `d',    0x64 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `e',    0x65 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `f',    0x66 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `g',    0x67 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `h',    0x68 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `i',    0x69 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `j',    0x6a */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `k',    0x6b */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `l',    0x6c */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `m',    0x6d */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `n',    0x6e */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `o',    0x6f */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `p',    0x70 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `q',    0x71 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `r',    0x72 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `s',    0x73 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `t',    0x74 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `u',    0x75 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `v',    0x76 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `w',    0x77 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `x',    0x78 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `y',    0x79 */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `z',    0x7a */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `{',    0x7b */
	FSRTL_OLE_LEGAL,                                                            /* `|',    0x7c */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `}',    0x7d */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `~',    0x7e */
	FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL     /*         0x7f */
};

const UCHAR * const FsRtlLegalAnsiCharacterArray = LegalAnsiCharacterArray;

#define FsRtlIsUnicodeCharacterWild(C)                                         \
    ((((C) >= 0x40) ? FALSE :                                                  \
    (FsRtlLegalAnsiCharacterArray[(C)] & FSRTL_WILD_CHARACTER )))

BOOLEAN NTAPI DoesNameContainWildCards(
    IN	PUNICODE_STRING	Name)
{
	if (Name->Length) {
		PWSTR Pointer;
		for (Pointer = Name->Buffer + Name->Length / sizeof(WCHAR) - 1; Pointer >= Name->Buffer && *Pointer != L'\\'; Pointer--) {
			if (FsRtlIsUnicodeCharacterWild(*Pointer)) return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN NTAPI IsNameInExpression(
	IN	PUNICODE_STRING	Expression,
	IN	PUNICODE_STRING	Name,
	IN	BOOLEAN			IgnoreCase,
	IN	PWCHAR			UpcaseTable OPTIONAL)
{
	BOOLEAN Result;
	NTSTATUS Status;
	UNICODE_STRING IntName;
	USHORT Offset, Position, BackTrackingPosition, OldBackTrackingPosition;
	USHORT BackTrackingBuffer[16], OldBackTrackingBuffer[16] = {0};
	PUSHORT BackTrackingSwap, BackTracking = BackTrackingBuffer, OldBackTracking = OldBackTrackingBuffer;
	ULONG BackTrackingBufferSize = RTL_NUMBER_OF(BackTrackingBuffer);
	PUSHORT AllocatedBuffer = NULL;
	UNICODE_STRING IntExpression;
	USHORT ExpressionPosition, NamePosition = 0, MatchingChars = 1;
	BOOLEAN EndOfName = FALSE;
	BOOLEAN DontSkipDot;
	WCHAR CompareChar;

	if (IgnoreCase && !UpcaseTable) {
		Status = RtlUpcaseUnicodeString(&IntName, Name, TRUE);
		if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
		Name = &IntName;
		IgnoreCase = FALSE;
	} else {
		IntName.Buffer = NULL;
	}

	if (!Name->Length || !Expression->Length) {
		Result = !Name->Length && !Expression->Length;
		goto Exit;
	}

	if (Expression->Length == sizeof(WCHAR)) {
		if (Expression->Buffer[0] == L'*') {
			Result = TRUE;
			goto Exit;
		}
	}

	ASSERT(!IgnoreCase || UpcaseTable);

	if (Expression->Buffer[0] == L'*') {
		IntExpression = *Expression;

		IntExpression.Buffer++;
		IntExpression.Length -= sizeof(WCHAR);

		if (!DoesNameContainWildCards(&IntExpression)) {
			if (Name->Length < (Expression->Length - sizeof(WCHAR))) {
				Result = FALSE;
				goto Exit;
			}

			NamePosition = (Name->Length - IntExpression.Length) / sizeof(WCHAR);

			if (!IgnoreCase) {
				Result = RtlEqualMemory(IntExpression.Buffer,
										Name->Buffer + NamePosition,
										IntExpression.Length);
				goto Exit;
			}
			else {
				for (ExpressionPosition = 0; ExpressionPosition < (IntExpression.Length / sizeof(WCHAR)); ExpressionPosition++) {
					ASSERT(IntExpression.Buffer[ExpressionPosition] == UpcaseTable[IntExpression.Buffer[ExpressionPosition]]);
					if (UpcaseTable[Name->Buffer[NamePosition + ExpressionPosition]] != IntExpression.Buffer[ExpressionPosition]) {
						Result = FALSE;
						goto Exit;
					}
				}

				Result = TRUE;
				goto Exit;
			}
		}
	}

	for (; !EndOfName; MatchingChars = BackTrackingPosition, NamePosition++) {
		OldBackTrackingPosition = BackTrackingPosition = 0;

		if (NamePosition >= Name->Length / sizeof(WCHAR)) {
			EndOfName = TRUE;
			if (MatchingChars && (OldBackTracking[MatchingChars - 1] == Expression->Length * 2)) break;
		}

		while (MatchingChars > OldBackTrackingPosition) {
			ExpressionPosition = (OldBackTracking[OldBackTrackingPosition++] + 1) / 2;

			for (Offset = 0; ExpressionPosition < Expression->Length; Offset = sizeof(WCHAR)) {
				ExpressionPosition += Offset;

				if (ExpressionPosition == Expression->Length) {
					BackTracking[BackTrackingPosition++] = Expression->Length * 2;
					break;
				}

				if (BackTrackingPosition > BackTrackingBufferSize - 3) {
					ASSERT(AllocatedBuffer == NULL);
					ASSERT((BackTracking == BackTrackingBuffer) || (BackTracking == OldBackTrackingBuffer));
					ASSERT((OldBackTracking == BackTrackingBuffer) || (OldBackTracking == OldBackTrackingBuffer));

					BackTrackingBufferSize = Expression->Length / sizeof(WCHAR) * 2 + 1;

					AllocatedBuffer = SafeAlloc(USHORT, 2 * BackTrackingBufferSize);
					if (AllocatedBuffer == NULL) {
						Result = FALSE;
						goto Exit;
					}

					RtlCopyMemory(
						AllocatedBuffer,
						BackTracking,
						RTL_NUMBER_OF(BackTrackingBuffer) * sizeof(USHORT));

					BackTracking = AllocatedBuffer;

					RtlCopyMemory(
						&BackTracking[BackTrackingBufferSize],
						OldBackTracking,
						RTL_NUMBER_OF(OldBackTrackingBuffer) * sizeof(USHORT));

					OldBackTracking = &BackTracking[BackTrackingBufferSize];
				}

				CompareChar = (NamePosition >= Name->Length / sizeof(WCHAR)) ? UNICODE_NULL : (IgnoreCase ? UpcaseTable[Name->Buffer[NamePosition]] :
											Name->Buffer[NamePosition]);
				if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == CompareChar && !EndOfName) {
					BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
				} else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == L'?' && !EndOfName) {
					BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
				} else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == L'*') {
					BackTracking[BackTrackingPosition++] = ExpressionPosition * 2;
					BackTracking[BackTrackingPosition++] = (ExpressionPosition * 2) + 3;
					continue;
				} else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == L'<') {
					DontSkipDot = TRUE;
					if (!EndOfName && Name->Buffer[NamePosition] == '.') {
						for (Position = NamePosition + 1; Position < Name->Length / sizeof(WCHAR); Position++) {
							if (Name->Buffer[Position] == L'.') {
								DontSkipDot = FALSE;
								break;
							}
						}
					}

					if (EndOfName || Name->Buffer[NamePosition] != L'.' || !DontSkipDot)
						BackTracking[BackTrackingPosition++] = ExpressionPosition * 2;
					BackTracking[BackTrackingPosition++] = (ExpressionPosition * 2) + 3;
					continue;
				} else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == L'"') {
					if (EndOfName) continue;
					if (Name->Buffer[NamePosition] == L'.')
						BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
				} else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == L'>') {
					if (EndOfName || Name->Buffer[NamePosition] == L'.') continue;
					BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
				}

				break;
			}

			for (Position = 0; MatchingChars > OldBackTrackingPosition && Position < BackTrackingPosition; Position++) {
				while (MatchingChars > OldBackTrackingPosition &&
						BackTracking[Position] > OldBackTracking[OldBackTrackingPosition]) ++OldBackTrackingPosition;
			}
		}

		BackTrackingSwap = BackTracking;
		BackTracking = OldBackTracking;
		OldBackTracking = BackTrackingSwap;
	}

	Result = MatchingChars > 0 && (OldBackTracking[MatchingChars - 1] == (Expression->Length * 2));

Exit:
	if (IntName.Buffer != NULL) RtlFreeUnicodeString(&IntName);
	return Result;
}

NTSTATUS NTAPI ExportLogThreadProc(
	IN	PVOID	Parameter)
{
	NTSTATUS Status;
	HANDLE FileHandle;
	UNICODE_STRING TextFileNameNt;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PCWSTR TextFileNameWin32;
	ULONG EntryIndex;
	ULONG MaxEntryIndex;
	ULONG SizeofMaxEntryIndex;
	ULONG CompletedPercentage;
	ULONG PreviousCompletedPercentage;

	SetClassLongPtr(MainWindow, GCLP_HCURSOR, (LONG_PTR) LoadCursor(NULL, IDC_WAIT));
	EnableWindow(MainWindow, FALSE);

	TextFileNameWin32 = (PCWSTR) Parameter;
	EntryIndex = 0;
	SizeofMaxEntryIndex = sizeof(MaxEntryIndex);
	CompletedPercentage = 0;
	PreviousCompletedPercentage = 0;

	//
	// find out total number of log entries, for calculating percentage
	//

	Status = VxlQueryInformationLog(
		State->LogHandle,
		LogTotalNumberOfEvents,
		&MaxEntryIndex,
		&SizeofMaxEntryIndex);

	if (!NT_SUCCESS(Status)) {
		goto Finished;
	}

	// safe to do this because we refuse to open log files with no entries
	--MaxEntryIndex;

	//
	// Convert win32 name to NT name and open the destination file
	//

	Status = RtlDosPathNameToNtPathName_U_WithStatus(
		TextFileNameWin32,
		&TextFileNameNt,
		NULL,
		NULL);

	if (!NT_SUCCESS(Status)) {
		goto Finished;
	}

	InitializeObjectAttributes(&ObjectAttributes, &TextFileNameNt, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtCreateFile(
		&FileHandle,
		GENERIC_WRITE | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OVERWRITE_IF,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0);

	RtlFreeUnicodeString(&TextFileNameNt);

	if (!NT_SUCCESS(Status)) {
		goto Finished;
	}

	//
	// loop over every log entry, convert to text and write out to the file
	//

	while (EntryIndex <= MaxEntryIndex) {
		PLOGENTRYCACHEENTRY CacheEntry;
		UNICODE_STRING ExportedText;
		LONGLONG ByteOffset;

		CacheEntry = GetLogEntryRaw(EntryIndex++);
		ConvertCacheEntryToText(CacheEntry, &ExportedText, FALSE);

		//
		// Write out the text to the file.
		//

		ByteOffset = -1; // write at end of file
		
		// wait for previous write to complete before starting a new one
		Status = NtWaitForSingleObject(FileHandle, FALSE, NULL);
		ASSERT (Status == STATUS_SUCCESS);

		Status = NtWriteFile(
			FileHandle,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			ExportedText.Buffer,
			ExportedText.Length,
			&ByteOffset,
			NULL);

		RtlFreeUnicodeString(&ExportedText);
		ASSERT (NT_SUCCESS(Status));

		//
		// Calculate percentage completed, for UI update
		//

		CompletedPercentage = (EntryIndex + 1) * 100 / (MaxEntryIndex + 1);
		ASSERT (CompletedPercentage <= 100);
		ASSERT (PreviousCompletedPercentage <= 100);

		//
		// Avoid expensive UI update if the percentage hasn't changed
		//

		if (CompletedPercentage != PreviousCompletedPercentage) {
			if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) SetWindowTextF(StatusBarWindow, L"正在导出日志条目。请稍候...（%ld%%）", CompletedPercentage);
			else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) SetWindowTextF(StatusBarWindow, L"正在導出日誌條目。請稍候...（%ld%%）", CompletedPercentage);
			else SetWindowTextF(StatusBarWindow, L"Exporting log entries. Please wait... (%ld%%)", CompletedPercentage);
			PreviousCompletedPercentage = CompletedPercentage;
		}
	}

	NtClose(FileHandle);

Finished:
	EnableWindow(MainWindow, TRUE);
	SetClassLongPtr(MainWindow, GCLP_HCURSOR, (LONG_PTR) LoadCursor(NULL, IDC_ARROW));

	if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
		SetWindowText(StatusBarWindow, L"完成。");
		if (NT_SUCCESS(Status)) InfoBoxF(L"导出完成。");
		else ErrorBoxF(L"导出日志失败（%s）", KexRtlNtStatusToString(Status));
	} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
		SetWindowText(StatusBarWindow, L"完成。");
		if (NT_SUCCESS(Status)) InfoBoxF(L"導出完成。");
		else ErrorBoxF(L"導出日誌失敗（%s）", KexRtlNtStatusToString(Status));
	} else {
		SetWindowText(StatusBarWindow, L"Finished.");
		if (NT_SUCCESS(Status)) InfoBoxF(L"Export complete.");
		else ErrorBoxF(L"Failed to export the log (%s)", KexRtlNtStatusToString(Status));
	}
	return Status;
}

VOID PopulateSourceComponents(
	IN	VXLHANDLE	LogHandle)
{
	HWND SourceComponentListViewWindow;
	ULONG Index;

	SourceComponentListViewWindow = GetDlgItem(FilterWindow, IDC_COMPONENTLIST);
	ListView_DeleteAllItems(SourceComponentListViewWindow);

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header->SourceComponents) &&
					LogHandle->Header->SourceComponents[Index][0] != '\0'; ++Index) {
		LVITEM Item;

		Item.mask = LVIF_TEXT;
		Item.iItem = Index;
		Item.iSubItem = 0;
		Item.pszText = LogHandle->Header->SourceComponents[Index];

		ListView_InsertItem(SourceComponentListViewWindow, &Item);
		ListView_SetCheckState(SourceComponentListViewWindow, Index, TRUE);
	}
}

PLOGENTRYCACHEENTRY AddLogEntryToCache(
	IN	ULONG			EntryIndex,
	IN	PVXLLOGENTRY	LogEntry)
{
	PLOGENTRYCACHEENTRY CacheEntry;
	WCHAR DateFormat[32];
	WCHAR TimeFormat[32];

	CacheEntry = SafeAlloc(LOGENTRYCACHEENTRY, 1);
	if (!CacheEntry) {
		return NULL;
	}

	RtlZeroMemory(CacheEntry, sizeof(*CacheEntry));
	CacheEntry->LogEntry = *LogEntry;
	
	//
	// pre-calc some strings
	//

	GetDateFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		DATE_AUTOLAYOUT | DATE_SHORTDATE,
		&LogEntry->Time,
		NULL,
		DateFormat,
		ARRAYSIZE(DateFormat),
		NULL);

	GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT,
		&LogEntry->Time,
		NULL,
		TimeFormat,
		ARRAYSIZE(CacheEntry->ShortDateTimeAsString));

	StringCchPrintf(
		CacheEntry->ShortDateTimeAsString,
		ARRAYSIZE(CacheEntry->ShortDateTimeAsString),
		L"%s %s", DateFormat, TimeFormat);

	StringCchPrintf(
		CacheEntry->SourceLineAsString,
		ARRAYSIZE(CacheEntry->SourceLineAsString),
		L"%lu", LogEntry->SourceLine);

	State->LogEntryCache[EntryIndex] = CacheEntry;
	return CacheEntry;
}

//
// Retrieve a log entry from the cache or from the log file.
// This function does not apply any filters.
//
PLOGENTRYCACHEENTRY GetLogEntryRaw(
	IN	ULONG	EntryIndex)
{
	NTSTATUS Status;
	PLOGENTRYCACHEENTRY CacheEntry;

	CacheEntry = State->LogEntryCache[EntryIndex];

	if (!CacheEntry) {
		VXLLOGENTRY LogEntry;

		Status = VxlReadLog(State->LogHandle, EntryIndex, &LogEntry);
		if (!NT_SUCCESS(Status)) {
			return NULL;
		}

		CacheEntry = AddLogEntryToCache(EntryIndex, &LogEntry);
	}

	return CacheEntry;
}

BOOLEAN LogEntryMatchesFilters(
	IN	PLOGENTRYCACHEENTRY	CacheEntry)
{
	BOOLEAN LogEntryMatchesTextFilter;

	//
	// The idea with this function is to apply the easiest/cheapest (in terms of
	// computation power) filters first. So that means, anything which requires
	// string comparisons and regex matching comes last.
	//
	// This function is one of the most critical when it comes to user experience,
	// since it is called potentially hundreds of thousands of times when the user
	// wants to search for text.
	//

	// 1. Does the log entry match the severity filter? If not, then we don't display
	//    this log entry.
	if (State->Filters.SeverityFilters[CacheEntry->LogEntry.Severity] == FALSE) {
		return FALSE;
	}

	// 2. Does this log entry match the source component filter? If not, then we don't
	//    display this log entry.
	if (State->Filters.ComponentFilters[CacheEntry->LogEntry.SourceComponentIndex] == FALSE) {
		return FALSE;
	}

	// 3. Does this log entry match the text filter? Note: empty filter always matches.
	LogEntryMatchesTextFilter = FALSE;

	if (State->Filters.TextFilter.Length == 0) {
		// empty filter
		LogEntryMatchesTextFilter = TRUE;
	} else {
		PUNICODE_STRING TextToSearch;

		TextToSearch = &CacheEntry->LogEntry.TextHeader;

SearchAgain:
		if (State->Filters.TextFilterWildcardMatch) {
			LogEntryMatchesTextFilter = IsNameInExpression(
				&State->Filters.TextFilter,
				TextToSearch,
				!State->Filters.TextFilterCaseSensitive,
				NULL);
		} else {
			if (State->Filters.TextFilterCaseSensitive) {
				if (State->Filters.TextFilterExact) {
					if (StringEqual(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (StringSearch(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
			} else {
				if (State->Filters.TextFilterExact) {
					if (StringEqualI(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (StringSearchI(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
			}
		}

		if (State->Filters.TextFilterWhole && TextToSearch == &CacheEntry->LogEntry.TextHeader) {
			if (CacheEntry->LogEntry.Text.Buffer != NULL) {
				TextToSearch = &CacheEntry->LogEntry.Text;
				goto SearchAgain;
			}
		}
	}

	if (State->Filters.TextFilterInverted) {
		LogEntryMatchesTextFilter = !LogEntryMatchesTextFilter;
	}

	return LogEntryMatchesTextFilter;
}

//
// Returns an ESTIMATE of the number of log entries that will be displayed
// by those filters. This estimate is not accurate for log files with many
// entries, because creating an accurate value for the number of log entries
// for any given filter requires evaluating the filter on all of the log
// entries.
// The estimate will always be equal to or greater than the actual number of
// log entries that match the filters.
//
ULONG EstimateNumberOfFilteredLogEntries(
	VOID)
{
	ULONG NumberOfFilteredLogEntries;
	ULONG Index;

	NumberOfFilteredLogEntries = 0;

	if (State->NumberOfLogEntries < 2000000) {
		//
		// do an accurate estimate - evaluate all the entries
		//
		for (Index = 0; Index < State->NumberOfLogEntries; Index++) {
			PLOGENTRYCACHEENTRY CacheEntry;

			CacheEntry = GetLogEntryRaw(Index);

			if (LogEntryMatchesFilters(CacheEntry)) {
				NumberOfFilteredLogEntries++;
			}
		}
	} else {
		//
		// inaccurate estimate - just wing it
		//
		for (Index = 0; Index < LogSeverityMaximumValue; Index++) {
			if (State->Filters.SeverityFilters[Index]) {
				NumberOfFilteredLogEntries += State->LogHandle->Header->EventSeverityTypeCount[Index];
			}
		}
	}

	return NumberOfFilteredLogEntries;
}

VOID RebuildFilterCache(
	VOID)
{
	ULONG EstimatedNumberOfFilteredLogEntries;

	//
	// clear the filtered entries lookup table
	//
	SafeFree(State->FilteredLookupCache);

	//
	// reallocate and initialize the lookup table
	//
	EstimatedNumberOfFilteredLogEntries = EstimateNumberOfFilteredLogEntries();
	State->EstimatedNumberOfFilteredLogEntries = EstimatedNumberOfFilteredLogEntries;
	State->FilteredLookupCache = SafeAlloc(ULONG, EstimatedNumberOfFilteredLogEntries);
	FillMemory(State->FilteredLookupCache, EstimatedNumberOfFilteredLogEntries * sizeof(ULONG), 0xFF);
}