#include "Robloz/Instance/RBXInstance.hpp"
#include "Robloz/DataModel/RBXDataModel.hpp"
#include <iostream>

int main() {
    ProcessMemory processMemory;
    if (!processMemory.AttachToProcess(L"RobloxPlayerBeta.exe")) {
        std::cout << "Failed to attach to Roblox process!" << std::endl;
        return 1;
    }

    auto dataModel = DataModel::get_singleton();
    uintptr_t dmAddress = dataModel->get_data_model(processMemory);
    std::cout << "DataModel: " << dmAddress  << std::endl;
    Instance dmInstance(dmAddress, std::make_shared<ProcessMemory>(processMemory));


    if (dmInstance.GetAddress() != 0) {
        std::cout << "DataModel class name: " << dmInstance.GetClassNameA() << std::endl;
    }
    else {
        std::cout << "Failed to get DataModel instance!" << std::endl;
    }
    auto CoreGui = dmInstance.FindFirstChildOfClass("CoreGui");
    std::cout << "CoreGui Address: " << CoreGui.GetAddress() << std::endl;

    std::cout << "CoreGui Class: " << CoreGui.GetClassNameA() << std::endl;

    auto RobloxGui = CoreGui.FindFirstChild("RobloxGui");
    std::cout << "RobloxGui Address: " << RobloxGui.GetAddress() << std::endl;

    std::cout << "RobloxGui Class: " << RobloxGui.GetClassNameA() << std::endl;

    auto Modules = RobloxGui.FindFirstChild("Modules");

    std::cout << "Modules Address: " << Modules.GetAddress() << std::endl;

    std::cout << "Modules Class: " << Modules.GetClassNameA() << std::endl;
    return 0;
}