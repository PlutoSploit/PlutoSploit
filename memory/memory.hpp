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
        if (!ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(T), nullptr)) {
            throw std::runtime_error("Failed to read memory!");
        }
        return value;
    }

    template<typename T>
    bool WriteMemory(uintptr_t address, const T& value) {
        if (!attached) {
            throw std::runtime_error("Process not attached!");
        }

        return WriteProcessMemory(processHandle, (LPVOID)address, &value, sizeof(T), nullptr);
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