#pragma once
#include "memory/memory.hpp"
#include "Robloz/Instance/RBXInstance.hpp"
#include <Windows.h>
#include <string>
#include <regex>
#include <sstream>
#include <locale>
#include <fstream>

class DataModel {
private:
    static DataModel* g_Singleton;
    std::uint64_t data_model;

public:
    DataModel() : data_model(0) {}
    static DataModel* get_singleton() noexcept;
    std::uint64_t get_data_model(ProcessMemory& processMemory);
    std::uint64_t get_render_view(ProcessMemory& processMemory);

};