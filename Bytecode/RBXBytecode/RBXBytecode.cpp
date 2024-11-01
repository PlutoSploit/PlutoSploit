#include "RBXBytecode.hpp"
#include "Luau/BytecodeBuilder.h"
#include "Luau/BytecodeUtils.h"
#include "Luau/Compiler.h"
#include "zstd.h"
#include "xxhash.h"
#include <cstring>

namespace {
    class BytecodeEncoder final : public Luau::BytecodeEncoder {
        void encode(uint32_t* data, size_t count) override {
            for (size_t i = 0; i < count;) {
                auto& opcode = *reinterpret_cast<uint8_t*>(data + i);
                i += Luau::getOpLength(static_cast<LuauOpcode>(opcode));
                opcode *= 227;
            }
        }
    };

    thread_local BytecodeEncoder g_encoder;
}

std::string Compiler::compress_bytecode(const std::string& uncompressed_bytecode) {
    const auto data_size = uncompressed_bytecode.size();
    const auto max_size = ZSTD_compressBound(data_size);

    std::vector<char> buffer(max_size + HEADER_SIZE);

    std::memcpy(buffer.data(), BYTECODE_MAGIC, sizeof(BYTECODE_MAGIC));
    std::memcpy(buffer.data() + sizeof(BYTECODE_MAGIC), &data_size, sizeof(data_size));

    const auto compressed_size = ZSTD_compress(
        buffer.data() + HEADER_SIZE,
        max_size,
        uncompressed_bytecode.data(),
        data_size,
        ZSTD_maxCLevel()
    );

    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error("Bytecode compression failed");
    }

    const auto total_size = compressed_size + HEADER_SIZE;
    const auto key = XXH32(buffer.data(), total_size, BYTECODE_HASH_SEED);
    const auto key_bytes = reinterpret_cast<const uint8_t*>(&key);

    for (size_t i = 0; i < total_size; ++i) {
        buffer[i] ^= key_bytes[i & 3] + i * BYTECODE_HASH_MULTIPLIER;
    }

    return std::string(buffer.data(), total_size);
}

std::string Compiler::compile_to_bytecode(const std::string& script) {
    auto compiled = Luau::compile(script, {}, {}, &g_encoder);
    return compress_bytecode(compiled);
}

std::string Compiler::decompress(const std::string& source) noexcept {
    try {
        if (source.size() < HEADER_SIZE) {
            throw std::runtime_error("Invalid bytecode size");
        }

        std::vector<uint8_t> buffer(source.begin(), source.end());
        std::vector<uint8_t> hash_bytes(4);

        for (size_t i = 0; i < 4; ++i) {
            hash_bytes[i] = buffer[i] ^ BYTECODE_MAGIC[i];
            hash_bytes[i] = static_cast<uint8_t>(hash_bytes[i] - i * BYTECODE_HASH_MULTIPLIER);
        }

        for (size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] ^= static_cast<uint8_t>(hash_bytes[i & 3] + i * BYTECODE_HASH_MULTIPLIER);
        }

        uint32_t hash_value = 0;
        for (size_t i = 0; i < 4; ++i) {
            hash_value |= static_cast<uint32_t>(hash_bytes[i]) << (i * 8);
        }

        const uint32_t rehash = XXH32(buffer.data(), buffer.size(), BYTECODE_HASH_SEED);
        if (rehash != hash_value) {
            throw std::runtime_error("Bytecode hash verification failed");
        }

        uint32_t decompressed_size = 0;
        std::memcpy(&decompressed_size, buffer.data() + 4, sizeof(decompressed_size));

        std::vector<uint8_t> decompressed(decompressed_size);
        const auto actual_size = ZSTD_decompress(
            decompressed.data(),
            decompressed_size,
            buffer.data() + HEADER_SIZE,
            buffer.size() - HEADER_SIZE
        );

        if (ZSTD_isError(actual_size)) {
            throw std::runtime_error("Bytecode decompression failed");
        }

        decompressed.resize(actual_size);
        return { decompressed.begin(), decompressed.end() };
    }
    catch (const std::exception&) {
        return "failed to decompress bytecode";
    }
}