#include "RBXInstance.hpp"

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
        return ReadRobloxString(m_memory->ReadMemory<uintptr_t>(m_address + NAME_OFFSET));
    }
    catch (const std::exception&) {
        return "";
    }
}

std::string Instance::GetClassName() const {
    if (!IsValid()) return "";
    try {
        uintptr_t classPtr = m_memory->ReadMemory<uintptr_t>(m_address + CLASSNAME_OFFSET);
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
        uintptr_t childrenPtr = m_memory->ReadMemory<uintptr_t>(m_address + CHILDREN_OFFSET);
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
