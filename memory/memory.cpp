#include "memory.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>

typedef NTSTATUS(WINAPI* NtReadVirtualMemoryFunc)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG NumberOfBytesRead
    );

typedef NTSTATUS(WINAPI* NtWriteVirtualMemoryFunc)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG NumberOfBytesWritten
    );

ProcessMemory::ProcessMemory() : processHandle(NULL), processId(0), attached(false) {}

ProcessMemory::~ProcessMemory() {
    if (attached) {
        CloseHandle(processHandle);
    }
}

DWORD ProcessMemory::GetProcessIdByName(const wchar_t* processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return processId;
}

bool ProcessMemory::AttachToProcess(const wchar_t* processName) {
    processId = GetProcessIdByName(processName);
    if (processId == 0) {
        std::cerr << "Process not found!" << std::endl;
        return false;
    }

    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (processHandle == NULL) {
        std::cerr << "Failed to open process! Error: " << GetLastError() << std::endl;
        return false;
    }

    attached = true;
    return true;
}

bool ProcessMemory::AttachToProcess(DWORD pid) {
    processId = pid;
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (processHandle == NULL) {
        std::cerr << "Failed to open process! Error: " << GetLastError() << std::endl;
        return false;
    }

    attached = true;
    return true;
}

std::vector<BYTE> ProcessMemory::ReadBytes(uintptr_t address, size_t size) {
    if (!attached) {
        throw std::runtime_error("Process not attached!");
    }

    std::vector<BYTE> buffer(size);
    ULONG bytesRead;
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    NtReadVirtualMemoryFunc NtReadVirtualMemory = (NtReadVirtualMemoryFunc)GetProcAddress(ntdll, "NtReadVirtualMemory");

    NTSTATUS status = NtReadVirtualMemory(processHandle, (PVOID)address, buffer.data(), size, &bytesRead);
    if (status != 0 || bytesRead != size) {
        throw std::runtime_error("Failed to read memory bytes!");
    }
    return buffer;
}

bool ProcessMemory::WriteBytes(uintptr_t address, const std::vector<BYTE>& bytes) {
    if (!attached) {
        throw std::runtime_error("Process not attached!");
    }

    DWORD oldProtection;
    if (!ProtectMemory(address, bytes.size(), PAGE_EXECUTE_READWRITE, oldProtection)) {
        return false;
    }

    ULONG bytesWritten;
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    NtWriteVirtualMemoryFunc NtWriteVirtualMemory = (NtWriteVirtualMemoryFunc)GetProcAddress(ntdll, "NtWriteVirtualMemory");

    NTSTATUS status = NtWriteVirtualMemory(processHandle, (PVOID)address, (PVOID)bytes.data(), bytes.size(), &bytesWritten);
    if (status != 0 || bytesWritten != bytes.size()) {
        return false;
    }

    return true;
}

uintptr_t ProcessMemory::AllocateMemory(size_t size, DWORD protectionFlags) {
    if (!attached) {
        throw std::runtime_error("Process not attached!");
    }

    return (uintptr_t)VirtualAllocEx(processHandle, nullptr, size, MEM_COMMIT | MEM_RESERVE, protectionFlags);
}

bool ProcessMemory::FreeMemory(uintptr_t address) {
    if (!attached) {
        throw std::runtime_error("Process not attached!");
    }

    return VirtualFreeEx(processHandle, (LPVOID)address, 0, MEM_RELEASE);
}

MEMORY_BASIC_INFORMATION ProcessMemory::GetMemoryInfo(uintptr_t address) {
    if (!attached) {
        throw std::runtime_error("Process not attached!");
    }

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)) == 0) {
        throw std::runtime_error("Failed to query memory information!");
    }
    return mbi;
}

uintptr_t ProcessMemory::ScanPattern(const std::vector<BYTE>& pattern, const std::vector<bool>& mask,
    uintptr_t startAddress, size_t searchSize) {
    if (!attached) {
        throw std::runtime_error("Process not attached!");
    }

    std::vector<BYTE> buffer(searchSize);
    ULONG bytesRead;

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    NtReadVirtualMemoryFunc NtReadVirtualMemory = (NtReadVirtualMemoryFunc)GetProcAddress(ntdll, "NtReadVirtualMemory");

    if (NtReadVirtualMemory(processHandle, (PVOID)startAddress, buffer.data(), searchSize, &bytesRead) == 0) {
        for (size_t i = 0; i <= bytesRead - pattern.size(); i++) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); j++) {
                if (mask[j] && buffer[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return startAddress + i;
            }
        }
    }
    return 0;
}

bool ProcessMemory::ProtectMemory(uintptr_t address, size_t size, DWORD newProtection, DWORD& oldProtection) {
    if (!attached) {
        throw std::runtime_error("Process not attached!");
    }

    return VirtualProtectEx(processHandle, (LPVOID)address, size, newProtection, &oldProtection);
}

bool ProcessMemory::IsProcessRunning() {
    if (!attached) return false;
    DWORD exitCode;
    GetExitCodeProcess(processHandle, &exitCode);
    return exitCode == STILL_ACTIVE;
}
