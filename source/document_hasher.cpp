#include "document_hasher.hpp"
#include <gcrypt.h>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <mutex>

static std::mutex gcrypt_mutex;

DocumentHasher::DocumentHasher() : is_initialized_(false) {
    initializeLibgcrypt();
}

DocumentHasher::~DocumentHasher() {
    std::lock_guard<std::mutex> lock(gcrypt_mutex);
    if (is_initialized_) {
        gcry_control(GCRYCTL_TERM_SECMEM, 0);
    }
}

void DocumentHasher::initializeLibgcrypt() {
    std::lock_guard<std::mutex> lock(gcrypt_mutex);
    if (is_initialized_) return;
    
    // const char* version = gcry_check_version(GCRYPT_VERSION);
    // if (!version) {
    //     throw std::runtime_error("Libgcrypt version mismatch. Expected: " + 
    //                             std::string(GCRYPT_VERSION) + 
    //                             ", but found different version");
    // }
    
    gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
    
    is_initialized_ = true;
}

std::vector<unsigned char> DocumentHasher::calculateCompositeHash(
    const std::string& file_path,
    const std::string& user_name,
    const std::string& user_phone,
    const std::string& confirmation_code,
    const std::string& signing_time) {

    if (file_path.empty()) {
        throw std::invalid_argument("File path cannot be empty");
    }
    if (user_name.empty() || user_phone.empty() || confirmation_code.empty() || signing_time.empty()) {
        throw std::invalid_argument("All metadata parameters must be non-empty");
    }

    std::lock_guard<std::mutex> lock(gcrypt_mutex);

    if (!is_initialized_) {
        throw std::runtime_error("Libgcrypt not initialized");
    }
    
    gcry_md_hd_t context;
    gcry_error_t error_code;

    // Инициализация контекста хеширования
    error_code = gcry_md_open(&context, GCRY_MD_STRIBOG256, 0);
    if (error_code) {
        throw std::runtime_error("Failed to initialize hash context");
    }

    try {
        // 1. Хеширование содержимого файла
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }

        // Проверяем что файл не пустой
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        if (file_size == 0) {
            throw std::runtime_error("File is empty: " + file_path);
        }
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(HASH_BUFFER_SIZE);
        while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
            gcry_md_write(context, buffer.data(), file.gcount());
        }
        file.close();

        // 2. Добавление метаданных в хеш с разделителями
        const std::string delimiter = "|";
        gcry_md_write(context, user_name.c_str(), user_name.length());
        gcry_md_write(context, delimiter.c_str(), delimiter.length());

        gcry_md_write(context, user_phone.c_str(), user_phone.length());
        gcry_md_write(context, delimiter.c_str(), delimiter.length());

        gcry_md_write(context, confirmation_code.c_str(), confirmation_code.length());
        gcry_md_write(context, delimiter.c_str(), delimiter.length());

        gcry_md_write(context, signing_time.c_str(), signing_time.length());

        // 3. Получение финального хеша
        unsigned char* digest = gcry_md_read(context, GCRY_MD_STRIBOG256);
        if (!digest) {
            throw std::runtime_error("Failed to read hash digest");
        }

        std::vector<unsigned char> hash_result(digest, digest + STRIBOG_HASH_LENGTH);
        gcry_md_close(context);
        
        return hash_result;

    } catch (const std::exception&) {
        gcry_md_close(context);
        throw;
    } catch (...) {
        gcry_md_close(context);
        throw std::runtime_error("Unknown error during hash calculation");
    }
}

std::string DocumentHasher::hashToHex(const std::vector<unsigned char>& hash) {
    if (hash.empty()) {
        throw std::invalid_argument("Hash vector cannot be empty");
    }
    std::string hex_string;
    hex_string.reserve(hash.size() * 2);
    
    for (unsigned char byte : hash) {
        char buffer[3];
        snprintf(buffer, sizeof(buffer), "%02x", byte);
        hex_string.append(buffer);
    }
    
    return hex_string;
}

bool DocumentHasher::verifyCompositeHash(
    const std::string& file_path,
    const std::string& user_name,
    const std::string& user_phone,
    const std::string& verification_code,
    const std::string& signing_time,
    const std::string& expected_hash_hex) {

    if (expected_hash_hex.empty()) {
        throw std::invalid_argument("Expected hash cannot be empty");
    }
    
    try {
        auto computed_hash = calculateCompositeHash(
            file_path, user_name, user_phone, verification_code, signing_time);
        
        std::string computed_hash_hex = hashToHex(computed_hash);
        std::string expected_lower = toLowercase(expected_hash_hex);
        
        return compareHashes(computed_hash_hex, expected_lower);
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Verification failed: " + std::string(e.what()));
    }
}

std::string DocumentHasher::toLowercase(const std::string& str) const {
    if (str.empty()) {
        return str;
    }
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool DocumentHasher::compareHashes(const std::string& hash1, const std::string& hash2) const {
    if (hash1.length() != hash2.length()) {
        return false;
    }
    
    volatile int result = 0;
    for (size_t i = 0; i < hash1.length(); ++i) {
        result |= hash1[i] ^ hash2[i];
    }
    return result == 0;
}