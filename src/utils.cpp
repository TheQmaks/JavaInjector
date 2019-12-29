#include <windows.h>
#include "utils.h"

size_t wlindexof(const wchar_t* str, size_t len, wchar_t c) {
	for (size_t i = len - 1; i != (size_t)(-1); --i) {
		if (str[i] == c)
			return i;
	}
	return -1;
}

HMODULE GetModuleHandlePeb(LPCWSTR name) {
#ifdef _AMD64_
	NTDEFINES::PPEB peb = reinterpret_cast<NTDEFINES::PPEB>(__readgsqword(0x60));
#else
	NTDEFINES::PPEB peb = reinterpret_cast<NTDEFINES::PPEB>(__readfsdword(0x30));
#endif

	NTDEFINES::PPEB_LDR_DATA LdrData = reinterpret_cast<NTDEFINES::PPEB_LDR_DATA>(peb->Ldr);
	NTDEFINES::PLDR_MODULE ListEntry = reinterpret_cast<NTDEFINES::PLDR_MODULE>(LdrData->InLoadOrderModuleList.Flink);
	while (ListEntry && ListEntry->BaseAddress) {
		size_t lastDot = wlindexof(ListEntry->BaseDllName.Buffer, ListEntry->BaseDllName.Length, L'.');
		size_t cmpResult = lastDot != -1
			? wcsncmp(ListEntry->BaseDllName.Buffer, name, lastDot)
			: wcscmp(ListEntry->BaseDllName.Buffer, name);

		if (!cmpResult)
			return reinterpret_cast<HMODULE>(ListEntry->BaseAddress);

		ListEntry = reinterpret_cast<NTDEFINES::PLDR_MODULE>(ListEntry->InLoadOrderModuleList.Flink);
	}

	return NULL;
}

PVOID GetProcAddressPeb(HMODULE hModule, LPCSTR name) {
	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
	PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<PBYTE>(hModule) + dosHeader->e_lfanew);
	IMAGE_OPTIONAL_HEADER optionalHeader = ntHeaders->OptionalHeader;

	IMAGE_DATA_DIRECTORY exportDir = optionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!exportDir.Size)
		return NULL;

	PIMAGE_EXPORT_DIRECTORY exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<PBYTE>(hModule) + exportDir.VirtualAddress);
	PDWORD functions = reinterpret_cast<PDWORD>(reinterpret_cast<PBYTE>(hModule) + exports->AddressOfFunctions);
	PDWORD names = reinterpret_cast<PDWORD>(reinterpret_cast<PBYTE>(hModule) + exports->AddressOfNames);

	for (size_t i = 0; i < exports->NumberOfFunctions; i++) {
		DWORD rva = *(functions + i);
		LPCSTR szName = reinterpret_cast<LPCSTR>(hModule) + *(names + i);
		if (!strcmp(name, szName))
			return reinterpret_cast<PBYTE>(hModule) + rva;
	}

	return NULL;
}