#include <ntifs.h>

struct info_t {
	HANDLE SourceProcess = 0;
	HANDLE access_handle = 0;
};

extern "C"
{
	NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
	PVOID NTAPI PsGetProcessSectionBaseAddress(PEPROCESS);
	PVOID NTAPI RtlFindExportedRoutineByName(PVOID, PCCH);
	PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID);
	NTSTATUS NTAPI ObReferenceObjectByName(PUNICODE_STRING, ULONG, PACCESS_STATE, ACCESS_MASK, POBJECT_TYPE, KPROCESSOR_MODE, PVOID OPTIONAL, PVOID*);
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
	NTSYSAPI NTSTATUS NTAPI ZwOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);

}

#define IOCTL_OPEN_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x002, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS CreateCall(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);

	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return irp->IoStatus.Status;
}

NTSTATUS CloseCall(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);

	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return irp->IoStatus.Status;
}

NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);

	DbgPrint("[+] Device control called.\n");

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

	auto request = reinterpret_cast<info_t*>(irp->AssociatedIrp.SystemBuffer);

	PEPROCESS target_process = nullptr;

	ULONG controlCode = stack_irp->Parameters.DeviceIoControl.IoControlCode;

	switch (controlCode) {
		case IOCTL_OPEN_PROCESS:

			OBJECT_ATTRIBUTES attr;
			InitializeObjectAttributes(&attr, NULL, 0, NULL, NULL);

			status = PsLookupProcessByProcessId(request->SourceProcess, &target_process);

			if (!NT_SUCCESS(status))
				break;

			CLIENT_ID client_id = { 0 };
			client_id.UniqueProcess = request->SourceProcess;

			HANDLE hProcess = nullptr;

			status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &attr, &client_id);

			ObDereferenceObject(target_process);

			if (!NT_SUCCESS(status))
				break;

			request->access_handle = hProcess;

			break;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = sizeof(info_t);

	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L"\\Device\\easyopen");

	PDEVICE_OBJECT device_object = nullptr;
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN,
									 FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	
	if (status != STATUS_SUCCESS) {
		DbgPrint("[-] Failed to create driver device.\n");
		return status;
	}

	DbgPrint("[+] Driver device successfuly created.\n");

	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\easyopen");

	status = IoCreateSymbolicLink(&symbolic_link, &device_name);

	if (status != STATUS_SUCCESS) {
		DbgPrint("[-] Failed to estabilish symbolic link.\n");
		return status;
	}

	DbgPrint("[+] Driver symbolic link successfuly estabilished.\n");

	SetFlag(driver_object->Flags, DO_BUFFERED_IO);

	driver_object->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = device_control;

	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

	DbgPrint("[+] Driver initialized succesfully.\n");

	return status;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	UNICODE_STRING driver_name = {};
	RtlInitUnicodeString(&driver_name, L"\\Driver\\easyopen");

	return IoCreateDriver(&driver_name, &driver_main);
}