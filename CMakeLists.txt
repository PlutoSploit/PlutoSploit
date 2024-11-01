cmake_minimum_required(VERSION 3.28)
project(RobloxExploit)

set(CMAKE_CXX_STANDARD 20)

find_package(xxhash CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)


add_executable(RobloxExploit main.cpp "memory/memory.cpp" "Bytecode/RBXBytecode/RBXBytecode.cpp"  "Robloz/DataModel/RBXDataModel.cpp" "Robloz/Instance/RBXInstance.cpp" "Bytecode/InitScript/Bytecode.h")
target_include_directories(RobloxExploit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")



add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Bytecode/Luau")

target_link_libraries(RobloxExploit
    PRIVATE
    zstd::libzstd
    Luau.Compiler
    Luau.Ast
    Luau.VM
    Luau.VM.Internals
    xxHash::xxhash
)