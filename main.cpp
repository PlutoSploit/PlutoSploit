#include "Robloz/Instance/RBXInstance.hpp"
#include "Robloz/DataModel/RBXDataModel.hpp"
#include <iostream>
#include "Bytecode/RBXBytecode/RBXBytecode.hpp"
#include <sstream>
#include <iomanip>
#include "Bytecode/InitScript/Bytecode.h"
#include <windows.h>
#include "Luau/BytecodeBuilder.h"
#include "Luau/BytecodeUtils.h"
#include "Luau/Compiler.h"
#include <zstd.h>
#include <xxhash.h>
#include <thread>
#include <chrono>

std::string compress_bytecode(std::string_view bytecode) {
    const auto data_size = bytecode.size();
    const auto max_size = ZSTD_compressBound(data_size);
    auto buffer = std::vector<char>(max_size + 8);

    strcpy_s(&buffer[0], buffer.capacity(), "RSB1");
    memcpy_s(&buffer[4], buffer.capacity(), &data_size, sizeof(data_size));

    const auto compressed_size = ZSTD_compress(&buffer[8], max_size, bytecode.data(), data_size, ZSTD_maxCLevel());
    if (ZSTD_isError(compressed_size))
        throw std::runtime_error("Failed to compress the bytecode.");

    const auto size = compressed_size + 8;
    const auto key = XXH32(buffer.data(), size, 42u);
    const auto bytes = reinterpret_cast<const uint8_t*>(&key);

    for (auto i = 0u; i < size; ++i)
        buffer[i] ^= bytes[i % 4] + i * 41u;

    return std::string(buffer.data(), size);
}

class bytecode_encoder_t : public Luau::BytecodeEncoder {
    inline void encode(uint32_t* data, size_t count) override {
        for (auto i = 0u; i < count;) {
            auto& opcode = *reinterpret_cast<uint8_t*>(data + i);
            i += Luau::getOpLength(LuauOpcode(opcode));
            opcode *= 227;
        }
    }
};


auto dataModel = DataModel::get_singleton();
static auto encoder = bytecode_encoder_t();

std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Execute(std::string src, ProcessMemory processMemory) {
    
    std::cout << "Test" << std::endl;

    std::string newsrc = R"(
        local function execute()
            )" + src + R"(
            return true
        end
        
        return execute 
    )";// this is all my penut brain can think of

    uintptr_t dmAddress = dataModel->get_data_model(processMemory);
    Instance exec_dmInstance(dmAddress, std::make_shared<ProcessMemory>(processMemory));

    std::cout << "DataModel Address: " << std::hex << dmAddress << std::endl;

    auto RobloxReplicatedStorage = exec_dmInstance.FindFirstChild("RobloxReplicatedStorage");
    std::cout << "RobloxReplicatedStorage Address: " << RobloxReplicatedStorage.GetAddress() << std::endl;

    auto Main = RobloxReplicatedStorage.FindFirstChild("Puls");
    std::cout << "Main Address: " << Main.GetAddress() << std::endl;

    auto execState = Main.FindFirstChild("ExecState");
    std::cout << "ExecState Address: " << execState.GetAddress() << std::endl;

    auto modulescript = Main.FindFirstChild("PlutoSploit");
    std::cout << "ModuleScript Address: " << modulescript.GetAddress() << std::endl;

    auto bytecode = Luau::compile(newsrc, {}, {}, &encoder);
    auto compressedSOURCE = compress_bytecode(bytecode);

    std::vector<uint8_t> ScriptBC(compressedSOURCE.begin(), compressedSOURCE.end());


    modulescript.setCoreScript(true);
    modulescript.SetBytecode(ScriptBC, compressedSOURCE.size());

    execState.setBoolValue(true);
}

int main() {
    ProcessMemory processMemory;
    std::cout << "Initializing..." << std::endl;

    if (!processMemory.AttachToProcess(L"RobloxPlayerBeta.exe")) {
        std::cout << "Failed to attach to Roblox process!" << std::endl;
        return 1;
    }

    uintptr_t dmAddress = dataModel->get_data_model(processMemory);
    std::cout << "DataModel: 0x" << std::hex << dmAddress << std::endl;

    Instance dmInstance(dmAddress, std::make_shared<ProcessMemory>(processMemory));

    if (dmInstance.GetAddress() != 0) {
    }
    else {
        std::cout << "Failed to get DataModel instance!" << std::endl;
        return -1;
    }

    auto CoreGui = dmInstance.FindFirstChildOfClass("CoreGui");
    auto RobloxGui = CoreGui.FindFirstChild("RobloxGui");
    auto Modules = RobloxGui.FindFirstChild("Modules");
    auto Common = Modules.FindFirstChild("Common");

    if (dmInstance.GetName() == "App") {
        std::cout << "Homepage" << std::endl;
        auto HomePageScriptLoader = Common.FindFirstChild("PolicyService");

        std::cout << "HomePage ScriptLoader: 0x" << std::hex << HomePageScriptLoader.GetAddress() << std::endl;

        HomePageScriptLoader.SetBytecode(init_script_bytecode, init_script_bytecode.size());
    }
    else {
        std::cout << "Ingame" << std::endl;

        auto CorePackages = dmInstance.FindFirstChild("CorePackages");
        auto JestGlobals = CorePackages.FindFirstChild("JestGlobals");
        auto PlayerList = Modules.FindFirstChild("PlayerList");
        auto PlayerListManager = PlayerList.FindFirstChild("PlayerListManager");
        JestGlobals.SetModuleBypass();

        PlayerListManager.Spoof(JestGlobals);
        JestGlobals.SetBytecode(init_script_bytecode, init_script_bytecode.size());

        HWND hwnd = FindWindow(nullptr, TEXT("Roblox"));

        if (!hwnd) {
            std::cerr << "Roblox window not found!" << std::endl;
        }
        if (SetForegroundWindow(hwnd)) {
            SetFocus(hwnd);
            keybd_event(0, MapVirtualKey(VK_ESCAPE, 0), KEYEVENTF_SCANCODE, 0);
            keybd_event(0, MapVirtualKey(VK_ESCAPE, 0), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0);
        }
        else {
            std::cerr << "Failed to bring the window to the foreground." << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::string userInput;

    std::cout << "Enter commands to execute (type 'exit' to quit):" << std::endl;

    while (true) {
        std::cout << "> ";  
        std::getline(std::cin, userInput);

        if (userInput == "exit") {
            break;
        }

        Execute(userInput, processMemory);
    }

    return 0;
}
