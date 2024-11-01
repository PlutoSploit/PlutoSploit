#include "RBXInstance.hpp"
#include <iostream>

std::string Instance::ReadRobloxString(uintptr_t address) const {
    if (!IsValid()) return "";

    try {
        uint64_t stringLength = m_memory->ReadMemory<uint64_t>(address + 0x10);
        if (stringLength > MAX_STRING_LENGTH || stringLength <= 0) {
            return "";
        }


        uintptr_t stringAddress = (stringLength > 15) ?
            m_memory->ReadMemory<uintptr_t>(address) : address;

        std::vector<char> buffer(stringLength + 1, 0);
        auto bytes = m_memory->ReadBytes(stringAddress, stringLength);
        std::copy(bytes.begin(), bytes.end(), buffer.begin());

        return std::string(buffer.data(), stringLength);
    }
    catch (const std::exception&) {
        return "";
    }
}

std::string Instance::GetName() const {
    if (!IsValid()) return "";
    try {
        return ReadRobloxString(m_memory->ReadMemory<uintptr_t>(m_address + offsets::NAME_OFFSET));
    }
    catch (const std::exception&) {
        return "";
    }
}

std::string Instance::GetClassName() const {
    if (!IsValid()) return "";
    try {
        uintptr_t classPtr = m_memory->ReadMemory<uintptr_t>(m_address + offsets::CLASSNAME_OFFSET);
        return ReadRobloxString(m_memory->ReadMemory<uintptr_t>(classPtr + 0x8));
    }
    catch (const std::exception&) {
        return "";
    }
}

std::vector<Instance> Instance::GetChildren() const {
    std::vector<Instance> children;
    if (!IsValid()) return children;

    try {
        uintptr_t childrenPtr = m_memory->ReadMemory<uintptr_t>(m_address + offsets::CHILDREN_OFFSET);
        if (childrenPtr == 0) return children;

        uintptr_t childrenStart = m_memory->ReadMemory<uintptr_t>(childrenPtr);
        uintptr_t childrenEnd = m_memory->ReadMemory<uintptr_t>(childrenPtr + 0x8);

        children.reserve((childrenEnd - childrenStart) / 0x10);

        for (uintptr_t addr = childrenStart; addr < childrenEnd; addr += 0x10) {
            uintptr_t childPtr = m_memory->ReadMemory<uintptr_t>(addr);
            if (childPtr != 0) {
                children.emplace_back(childPtr, m_memory);
            }
        }
    }
    catch (const std::exception&) {
        children.clear();
    }

    return children;
}

Instance Instance::FindFirstChild(const std::string& name) const {
    if (!IsValid() || name.empty()) return Instance();

    for (const auto& child : GetChildren()) {
        if (child.GetName() == name) {
            return child;
        }
    }
    return Instance();
}

Instance Instance::FindFirstChildOfClass(const std::string& className) const {
    if (!IsValid() || className.empty()) return Instance();

    for (const auto& child : GetChildren()) {
        if (child.GetClassName() == className) {
            return child;
        }
    }
    return Instance();
}

void Instance::SetBytecode(std::vector<uint8_t> bytes, int bytecode_size) {
    if (this->GetClassNameA() != "LocalScript" && this->GetClassNameA() != "ModuleScript")
        return;

    std::uintptr_t embeddedPtr = m_memory->ReadMemory<std::uintptr_t>(this->GetAddress() + offsets::ModuleScriptEmbedded);

    std::uintptr_t allocatedAddress = m_memory->AllocateMemory(bytecode_size);
    std::cout << allocatedAddress << std::endl;
    if (allocatedAddress == 0)
        return;

    m_memory->WriteBytes(allocatedAddress, bytes);

    m_memory->WriteMemory<std::uintptr_t>(embeddedPtr + 0x10, allocatedAddress);
    m_memory->WriteMemory<std::uint64_t>(embeddedPtr + 0x20, bytecode_size);
}




void Instance::SetModuleBypass() {
     m_memory->WriteMemory<unsigned long long>(this->GetAddress() + offsets::ModuleFlags, 0x100000000);
}

void Instance::Spoof(Instance item) {
    uintptr_t address = GetAddress();
    if (address != 0) {
        m_memory->WriteMemory<unsigned long long>(address + 0x8, item.GetAddress());
    }
}


void Instance::setCoreScript(bool Boolean) {
    m_memory->WriteMemory<unsigned long long>(this->GetAddress() + offsets::IsCoreScript, Boolean);
}

void Instance::setBoolValue(bool num) {
     m_memory->WriteMemory<unsigned long long>(this->GetAddress() + offsets::ObjectValue, num);
    
}

void Instance::UnlockModule(){
    if (this->GetClassNameA() == "ModuleScript") {
        m_memory->WriteMemory<std::uintptr_t>(this->GetAddress() + offsets::ModuleFlags, 0x100000000);
        m_memory->WriteMemory<std::uintptr_t>(this->GetAddress() + offsets::IsCoreScript, 0x1);
    }
}