#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstdint>

class Compiler {
public:
    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;

    static Compiler& get_singleton() noexcept {
        static Compiler instance;
        return instance;
    }

    [[nodiscard]] std::string compress_bytecode(const std::string& uncompressed_bytecode);
    [[nodiscard]] std::string compile_to_bytecode(const std::string& script);
    [[nodiscard]] static std::string decompress(const std::string& source) noexcept;

private:
    Compiler() = default;

    static constexpr uint8_t BYTECODE_MAGIC[4] = { 'R', 'S', 'B', '1' };
    static constexpr uint32_t BYTECODE_HASH_MULTIPLIER = 41;
    static constexpr uint32_t BYTECODE_HASH_SEED = 42;
    static constexpr size_t HEADER_SIZE = 8;
};