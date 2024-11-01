#pragma once
#include "memory/memory.hpp"
#include <string>
#include <vector>
#include <memory>
namespace offsets {
    constexpr std::uint64_t NAME_OFFSET = 0x68;
    constexpr std::uint64_t CHILDREN_OFFSET = 0x70;
    constexpr std::uint64_t CLASSNAME_OFFSET = 0x18;
    constexpr std::uint64_t ModuleScriptEmbedded = 0x168;
    constexpr std::uint64_t LocalScriptEmbedded = 0x1c8;
    constexpr std::uint64_t Bytecode = 0x10;
    constexpr std::uint64_t BytecodeSize = 0x20;
    constexpr std::uint64_t IsCoreScript = 0x1b0;
    constexpr std::uint64_t ModuleFlags = IsCoreScript - 0x4;
    constexpr std::uint64_t LocalPlayer = 0x118;
    constexpr std::uint64_t ObjectValue = 0xc8;
}

class Instance {
private:
    uintptr_t m_address;
    std::shared_ptr<ProcessMemory> m_memory;
    
    static constexpr uint32_t MAX_STRING_LENGTH = 15000;

    std::string ReadRobloxString(uintptr_t address) const;

public:
    Instance() : m_address(0), m_memory(nullptr) {}
    Instance(uintptr_t address, std::shared_ptr<ProcessMemory> memory)
        : m_address(address), m_memory(memory) {}

    uintptr_t GetAddress() const { return m_address; }
    bool IsValid() const { return m_address != 0 && m_memory != nullptr; }

    std::string GetName() const;
    std::string GetClassName() const;

    std::vector<Instance> GetChildren() const;
    Instance FindFirstChild(const std::string& name) const;
    Instance FindFirstChildOfClass(const std::string& className) const;
    void SetBytecode(std::vector<uint8_t> bytes, int bytecode_size);

    void SetModuleBypass();
    void Spoof(Instance item);
    void setCoreScript(bool Boolean);
    void setBoolValue(bool num);
    void UnlockModule();
    bool operator==(const Instance& other) const {
        return m_address == other.m_address;
    }

    bool operator!=(const Instance& other) const {
        return !(*this == other);
    }
};