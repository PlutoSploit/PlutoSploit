#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <stdexcept>

class ProcessMemory {
private:
    HANDLE processHandle;
    DWORD processId;
    bool attached;

    DWORD GetProcessIdByName(const wchar_t* processName);

public:
    ProcessMemory();
    ~ProcessMemory();

    bool AttachToProcess(const wchar_t* processName);
    bool AttachToProcess(DWORD pid);

    template<typename T>
    T ReadMemory(uintptr_t address) {
        if (!attached) {
            throw std::runtime_error("Process not attached!");
        }

        T value;
        ULONG bytesRead;
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        using NtReadVirtualMemoryFunc = NTSTATUS(WINAPI*)(HANDLE, PVOID, PVOID, ULONG, PULONG);
        NtReadVirtualMemoryFunc NtReadVirtualMemory = (NtReadVirtualMemoryFunc)GetProcAddress(ntdll, "NtReadVirtualMemory");

        NTSTATUS status = NtReadVirtualMemory(processHandle, (PVOID)address, &value, sizeof(T), &bytesRead);
        if (status != 0 || bytesRead != sizeof(T)) {
            throw std::runtime_error("Failed to read memory!");
        }
        return value;
    }

    template<typename T>
    bool WriteMemory(uintptr_t address, const T& value) {
        if (!attached) {
            throw std::runtime_error("Process not attached!");
        }

        DWORD oldProtection;
        if (!ProtectMemory(address, sizeof(T), PAGE_EXECUTE_READWRITE, oldProtection)) {
            return false;
        }

        ULONG bytesWritten;
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        using NtWriteVirtualMemoryFunc = NTSTATUS(WINAPI*)(HANDLE, PVOID, PVOID, ULONG, PULONG);
        NtWriteVirtualMemoryFunc NtWriteVirtualMemory = (NtWriteVirtualMemoryFunc)GetProcAddress(ntdll, "NtWriteVirtualMemory");

        NTSTATUS status = NtWriteVirtualMemory(processHandle, (PVOID)address, (PVOID)&value, sizeof(T), &bytesWritten);
        if (status != 0 || bytesWritten != sizeof(T)) {
            return false;
        }

        // Restore original protection if necessary
        ProtectMemory(address, sizeof(T), oldProtection, oldProtection);
        return true;
    }

    std::vector<BYTE> ReadBytes(uintptr_t address, size_t size);
    bool WriteBytes(uintptr_t address, const std::vector<BYTE>& bytes);
    uintptr_t AllocateMemory(size_t size, DWORD protectionFlags = PAGE_EXECUTE_READWRITE);
    bool FreeMemory(uintptr_t address);
    MEMORY_BASIC_INFORMATION GetMemoryInfo(uintptr_t address);
    uintptr_t ScanPattern(const std::vector<BYTE>& pattern, const std::vector<bool>& mask,
        uintptr_t startAddress, size_t searchSize);
    bool ProtectMemory(uintptr_t address, size_t size, DWORD newProtection, DWORD& oldProtection);
    bool IsProcessRunning();

    // Utility getters
    HANDLE GetProcessHandle() const { return processHandle; }
    DWORD GetProcessId() const { return processId; }
    bool IsAttached() const { return attached; }
};
