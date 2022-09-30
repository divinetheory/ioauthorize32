// IOAUTHORIZE32D.SYS -- by Justin Johnson (Based on GIVEIO.SYS -- by Dale Roberts)
// Compile:    Use DDK BUILD facility (WDK 7.1)
// Purpose:    Give direct port I/O access to a user mode process.

#define _X86_ 1

#include <ntddk.h>

#define DEVICE_NAME_STRING L"ioauthorize32"
#define IOPM_SIZE 0x2000

typedef UCHAR IOPM[IOPM_SIZE];
IOPM* pIOPM;

NTSTATUS PsLookupProcessByProcessId(HANDLE, PEPROCESS*);

void Ke386SetIoAccessMap(int, IOPM*);
void Ke386IoSetAccessProcess(PEPROCESS, int);

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

__drv_dispatchType(IRP_MJ_CLOSE)
DRIVER_DISPATCH DeviceClose;

__drv_dispatchType(IRP_MJ_CREATE)
DRIVER_DISPATCH DeviceCreate;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH DeviceControl;

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject) {
	UNICODE_STRING uniNameStringDOS;
	WCHAR* nameBufferDOS;
	if (pIOPM) {
		MmFreeNonCachedMemory(pIOPM, sizeof(IOPM));
	}
	nameBufferDOS = (WCHAR*)L"\\DosDevices\\" DEVICE_NAME_STRING;
	RtlInitUnicodeString(&uniNameStringDOS, nameBufferDOS);
	IoDeleteSymbolicLink(&uniNameStringDOS);
	IoDeleteDevice(pDriverObject->DeviceObject);
	return;
}

NTSTATUS DeviceControl(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
	HANDLE hProcessId;
	NTSTATUS status;
	PEPROCESS pEProcess;
	PIO_STACK_LOCATION pIOStackLocation;
	ULONG bufferLength, inputBufferLength, outputBufferLength;
	size_t handleSize;
	pIOStackLocation = IoGetCurrentIrpStackLocation(pIrp);
	if ((inputBufferLength = pIOStackLocation->Parameters.DeviceIoControl.InputBufferLength) > (outputBufferLength = pIOStackLocation->Parameters.DeviceIoControl.OutputBufferLength)) {
		bufferLength = inputBufferLength;
	}
	else {
		bufferLength = outputBufferLength;
	}
	handleSize = sizeof(HANDLE);
	if (bufferLength == (ULONG)handleSize) {
		RtlCopyMemory(&hProcessId, pIrp->AssociatedIrp.SystemBuffer, handleSize);
		status = PsLookupProcessByProcessId(hProcessId, &pEProcess);
		if (NT_SUCCESS(status)) {
			Ke386IoSetAccessProcess(pEProcess, 1);
			Ke386SetIoAccessMap(1, pIOPM);
			ObDereferenceObject(pEProcess);
		}
	}
	else {
		bufferLength = 0;
		status = STATUS_INVALID_BUFFER_SIZE;
	}
	pIrp->IoStatus.Information = bufferLength;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DeviceClose(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DeviceCreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
	return DeviceClose(pDeviceObject, pIrp);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath) {
	NTSTATUS status;
	PDEVICE_OBJECT pDeviceObject;
	UNICODE_STRING uniNameString, uniNameStringDOS;
	WCHAR* nameBuffer, * nameBufferDOS;
	size_t IOPMSize;
	IOPMSize = sizeof(IOPM);
	pIOPM = (IOPM*)MmAllocateNonCachedMemory(IOPMSize);
	if (pIOPM == 0) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(pIOPM, IOPMSize);
	nameBuffer = (WCHAR*)L"\\Device\\" DEVICE_NAME_STRING;
	RtlInitUnicodeString(&uniNameString, nameBuffer);
	if (!NT_SUCCESS(status = IoCreateDevice(pDriverObject, 0, &uniNameString, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject))) {
		return status;
	}
	nameBufferDOS = (WCHAR*)L"\\DosDevices\\" DEVICE_NAME_STRING;
	RtlInitUnicodeString(&uniNameStringDOS, nameBufferDOS);
	if (!NT_SUCCESS(status = IoCreateSymbolicLink(&uniNameStringDOS, &uniNameString))) {
		return status;
	}
	pDriverObject->DriverUnload = DriverUnload;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
	return status;
}