#pragma once
#include "memory/memory.hpp"
#include <string>
#include <vector>
#include <memory>

class Instance {
private:
    uintptr_t m_address;
    std::shared_ptr<ProcessMemory> m_memory;

    static constexpr uint32_t NAME_OFFSET = 0x68;
    static constexpr uint32_t CHILDREN_OFFSET = 0x70;
    static constexpr uint32_t CLASSNAME_OFFSET = 0x18;
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

    bool operator==(const Instance& other) const {
        return m_address == other.m_address;
    }

    bool operator!=(const Instance& other) const {
        return !(*this == other);
    }
};