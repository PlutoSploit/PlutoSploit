#include <Windows.h>
#include <string>
#include <regex>
#include <sstream>
#include <locale>
#include <fstream>
#include "RBXDataModel.hpp"
#include "memory/memory.hpp"

DataModel* DataModel::g_Singleton = nullptr;

DataModel* DataModel::get_singleton() noexcept {
    if (g_Singleton == nullptr)
        g_Singleton = new DataModel();
    return g_Singleton;
}

uintptr_t get_view_regex(const std::string& folderPath, const std::string& latestFile) {
    std::regex regexPattern("view\\((\\w+)\\)");
    std::ifstream fileStream(folderPath + "\\" + latestFile);
    if (!fileStream.is_open()) {
        printf("Failed to open log file: %s\n", (folderPath + "\\" + latestFile).c_str());
        return 0;
    }
    std::string line;
    std::smatch match;
    uintptr_t newAddress = 0;
    while (std::getline(fileStream, line)) {
        if (std::regex_search(line, match, regexPattern)) {
            std::istringstream ss(match[1].str());
            ss >> std::hex >> newAddress;
            break;
        }
    }
    return newAddress;
}

std::string get_latest_log_file(const std::string& folderPath) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((folderPath + "\\*").c_str(), &findFileData);
    std::string latestFile;
    FILETIME ftLatest = { 0, 0 };
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (CompareFileTime(&ftLatest, &findFileData.ftLastWriteTime) < 0) {
                ftLatest = findFileData.ftLastWriteTime;
                latestFile = findFileData.cFileName;
            }
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }
    return latestFile;
}

std::uint64_t DataModel::get_render_view(ProcessMemory& processMemory) {
    TCHAR localAppData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("LOCALAPPDATA"), localAppData, MAX_PATH) > 0) {
        std::string logsPath = std::string(localAppData) + "\\Roblox\\logs";
        uintptr_t view_regex = get_view_regex(logsPath, get_latest_log_file(logsPath));
        return view_regex;
    }
    printf("Failed to retrieve LOCALAPPDATA environment variable.\n");
    return 0;
}

std::uint64_t DataModel::get_data_model(ProcessMemory& processMemory) {
    std::uint64_t render_view = this->get_render_view(processMemory);
    if (render_view == 0) {
        printf("Render view address not found.\n");
        return 0;
    }
    uintptr_t data_model = processMemory.ReadMemory<std::uint64_t>(render_view + 0x118);
    if (data_model == 0) {
        printf("Failed to retrieve the data model.\n");
        return 0;
    }
    return processMemory.ReadMemory<std::uint64_t>(data_model + 0x1a8);
}
