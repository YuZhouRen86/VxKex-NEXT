#include "buildcfg.h"
#include "kxuserp.h"

//
// The Windows 7.1 SDK BluetoothAPIs.h contains malformed
// "#pragma deprecate" directives which produce C4068 warnings.
// KxUser is built with /WX, so do not include that header here.
//
// The six classic APIs below only need opaque structure pointers and
// HANDLE-based enumeration types, so forward-declare the types locally.
// This also avoids introducing conflicting declarations from the
// Windows 7.1 SDK Bluetooth header.
//

typedef struct _BLUETOOTH_DEVICE_SEARCH_PARAMS BLUETOOTH_DEVICE_SEARCH_PARAMS;
typedef struct _BLUETOOTH_DEVICE_INFO BLUETOOTH_DEVICE_INFO;
typedef struct _BLUETOOTH_FIND_RADIO_PARAMS BLUETOOTH_FIND_RADIO_PARAMS;

typedef HANDLE HBLUETOOTH_DEVICE_FIND;
typedef HANDLE HBLUETOOTH_RADIO_FIND;

//
// Classic Bluetooth APIs are implemented by Windows in bthprops.cpl.
//
// Do not use a .def forwarder to bthprops.cpl. KxUser instead loads the
// system copy explicitly and resolves the required entry points.
//
// LoadSystemLibrary() temporarily disables VxKex DLL rewriting while
// loading bthprops.cpl and its dependencies.
//

static INIT_ONCE BthPropsInitOnce = INIT_ONCE_STATIC_INIT;
static HMODULE BthPropsModule;

static BOOL CALLBACK
InitializeBthProps(
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID *Context)
{
    UNREFERENCED_PARAMETER(InitOnce);
    UNREFERENCED_PARAMETER(Parameter);
    UNREFERENCED_PARAMETER(Context);

    BthPropsModule = LoadSystemLibrary(L"bthprops.cpl");

    return BthPropsModule != NULL;
}

static HMODULE
GetBthPropsModule(
    VOID)
{
    if (!InitOnceExecuteOnce(
        &BthPropsInitOnce,
        InitializeBthProps,
        NULL,
        NULL))
    {
        return NULL;
    }

    return BthPropsModule;
}

KXUSERAPI HBLUETOOTH_DEVICE_FIND WINAPI
BluetoothFindFirstDevice(
    BLUETOOTH_DEVICE_SEARCH_PARAMS *pbtsp,
    BLUETOOTH_DEVICE_INFO *pbtdi)
{
    typedef HBLUETOOTH_DEVICE_FIND (WINAPI *PFN_BluetoothFindFirstDevice)(
        BLUETOOTH_DEVICE_SEARCH_PARAMS *,
        BLUETOOTH_DEVICE_INFO *);

    HMODULE Module;
    PFN_BluetoothFindFirstDevice Function;

    Module = GetBthPropsModule();

    if (!Module)
        return NULL;

    Function = (PFN_BluetoothFindFirstDevice)GetProcAddress(
        Module,
        "BluetoothFindFirstDevice");

    if (!Function)
        return NULL;

    return Function(pbtsp, pbtdi);
}

KXUSERAPI BOOL WINAPI
BluetoothFindNextDevice(
    HBLUETOOTH_DEVICE_FIND hFind,
    BLUETOOTH_DEVICE_INFO *pbtdi)
{
    typedef BOOL (WINAPI *PFN_BluetoothFindNextDevice)(
        HBLUETOOTH_DEVICE_FIND,
        BLUETOOTH_DEVICE_INFO *);

    HMODULE Module;
    PFN_BluetoothFindNextDevice Function;

    Module = GetBthPropsModule();

    if (!Module)
        return FALSE;

    Function = (PFN_BluetoothFindNextDevice)GetProcAddress(
        Module,
        "BluetoothFindNextDevice");

    if (!Function)
        return FALSE;

    return Function(hFind, pbtdi);
}

KXUSERAPI BOOL WINAPI
BluetoothFindDeviceClose(
    HBLUETOOTH_DEVICE_FIND hFind)
{
    typedef BOOL (WINAPI *PFN_BluetoothFindDeviceClose)(
        HBLUETOOTH_DEVICE_FIND);

    HMODULE Module;
    PFN_BluetoothFindDeviceClose Function;

    Module = GetBthPropsModule();

    if (!Module)
        return FALSE;

    Function = (PFN_BluetoothFindDeviceClose)GetProcAddress(
        Module,
        "BluetoothFindDeviceClose");

    if (!Function)
        return FALSE;

    return Function(hFind);
}

KXUSERAPI HBLUETOOTH_RADIO_FIND WINAPI
BluetoothFindFirstRadio(
    BLUETOOTH_FIND_RADIO_PARAMS *pbtfrp,
    HANDLE *phRadio)
{
    typedef HBLUETOOTH_RADIO_FIND (WINAPI *PFN_BluetoothFindFirstRadio)(
        BLUETOOTH_FIND_RADIO_PARAMS *,
        HANDLE *);

    HMODULE Module;
    PFN_BluetoothFindFirstRadio Function;

    Module = GetBthPropsModule();

    if (!Module)
        return NULL;

    Function = (PFN_BluetoothFindFirstRadio)GetProcAddress(
        Module,
        "BluetoothFindFirstRadio");

    if (!Function)
        return NULL;

    return Function(pbtfrp, phRadio);
}

KXUSERAPI BOOL WINAPI
BluetoothFindNextRadio(
    HBLUETOOTH_RADIO_FIND hFind,
    HANDLE *phRadio)
{
    typedef BOOL (WINAPI *PFN_BluetoothFindNextRadio)(
        HBLUETOOTH_RADIO_FIND,
        HANDLE *);

    HMODULE Module;
    PFN_BluetoothFindNextRadio Function;

    Module = GetBthPropsModule();

    if (!Module)
        return FALSE;

    Function = (PFN_BluetoothFindNextRadio)GetProcAddress(
        Module,
        "BluetoothFindNextRadio");

    if (!Function)
        return FALSE;

    return Function(hFind, phRadio);
}

KXUSERAPI BOOL WINAPI
BluetoothFindRadioClose(
    HBLUETOOTH_RADIO_FIND hFind)
{
    typedef BOOL (WINAPI *PFN_BluetoothFindRadioClose)(
        HBLUETOOTH_RADIO_FIND);

    HMODULE Module;
    PFN_BluetoothFindRadioClose Function;

    Module = GetBthPropsModule();

    if (!Module)
        return FALSE;

    Function = (PFN_BluetoothFindRadioClose)GetProcAddress(
        Module,
        "BluetoothFindRadioClose");

    if (!Function)
        return FALSE;

    return Function(hFind);
}

//
// These are all stubs. No idea how to begin implementing them properly.
//

KXUSERAPI HRESULT WINAPI BluetoothGATTAbortReliableWrite(
	IN	HANDLE	Device,
	IN	ULONG	ReliableWriteContext,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTBeginReliableWrite(
	IN	HANDLE	Device,
	OUT	PVOID	ReliableWriteContext,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTEndReliableWrite(
	IN	HANDLE	Device,
	IN	ULONG	ReliableWriteContext,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetCharacteristics(
	IN	HANDLE	Device,
	IN	PVOID	Service OPTIONAL,
	IN	USHORT	CharacteristicsBufferCount,
	OUT	PVOID	CharacteristicsBuffer OPTIONAL,
	OUT	PUSHORT	CharacteristicsBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetCharacteristicValue(
	IN	HANDLE	Device,
	IN	PVOID	Characteristic,
	IN	ULONG	CharacteristicValueDataSize,
	OUT	PVOID	CharacteristicValue OPTIONAL,
	OUT	PUSHORT	CharacteristicValueSizeRequired OPTIONAL,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetDescriptors(
	IN	HANDLE	Device,
	IN	PVOID	Characteristic,
	IN	USHORT	DescriptorsBufferCount,
	OUT	PVOID	DescriptorsBuffer OPTIONAL,
	OUT	PUSHORT	DescriptorsBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetDescriptorValue(
	IN	HANDLE	Device,
	IN	PVOID	Descriptor,
	IN	ULONG	DescriptorValueDataSize,
	OUT	PVOID	DescriptorValue OPTIONAL,
	OUT	PUSHORT	DescriptorValueSizeRequired OPTIONAL,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetIncludedServices(
	IN	HANDLE	Device,
	IN	PVOID	ParentService OPTIONAL,
	IN	USHORT	IncludedServicesBufferCount,
	OUT	PVOID	IncludedServicesBuffer OPTIONAL,
	OUT	PUSHORT	IncludedServicesBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetServices(
	IN	HANDLE	Device,
	IN	USHORT	ServicesBufferCount,
	OUT	PVOID	ServicesBuffer OPTIONAL,
	OUT	PUSHORT	ServicesBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTRegisterEvent(
	IN	HANDLE	Service,
	IN	ULONG	EventType,
	IN	PVOID	EventParameterIn,
	IN	PVOID	Callback,
	IN	PVOID	CallbackContext OPTIONAL,
	OUT	PVOID	EventHandle,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTSetCharacteristicValue(
	IN	HANDLE	Device,
	IN	PVOID	Characteristic,
	IN	PVOID	CharacteristicValue,
	IN	PVOID	ReliableWriteContext OPTIONAL,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTSetDescriptorValue(
	IN	HANDLE	Device,
	IN	PVOID	Descriptor,
	IN	PVOID	DescriptorValue,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTUnregisterEvent(
	IN	HANDLE	EventHandle,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}
