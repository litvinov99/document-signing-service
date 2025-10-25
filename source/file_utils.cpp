#include "file_utils.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <random>
#include <mutex>
#include <atomic>
#include <thread>

namespace file_utils {

namespace {
    // Функция для получения мьютекса (решает проблему статической инициализации)
    std::mutex& get_fs_mutex() {
        static std::mutex mutex;
        return mutex;
    }
    
    // Функция для получения атомарного счетчика
    std::atomic<uint64_t>& get_file_counter() {
        static std::atomic<uint64_t> counter{0};
        return counter;
    }
}

std::string generateUniqueFilename(const std::string& prefix,
                                   const std::string& suffix) {
    // Время + поток + атомарный счетчик + случайное число
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    auto counter = get_file_counter().fetch_add(1, std::memory_order_relaxed);
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    auto random = dis(gen);
    
    return prefix + 
           std::to_string(now) + "_" +
           std::to_string(thread_id) + "_" +
           std::to_string(counter) + "_" +
           std::to_string(random) + 
           suffix;
}

std::string createTempCopyWithUniqueFilename(const std::string& original_path,
                           const std::string& prefix,
                           const std::string& suffix,
                           const std::string& temp_dir) {
    std::lock_guard<std::mutex> lock(get_fs_mutex());
    namespace fs = std::filesystem;

    // Создаем директорию если не существует
    if (!ensureDirectoryExists(temp_dir)) {
        throw std::runtime_error("Failed to create temp directory: " + temp_dir);
    }
    
    fs::path abs_original_path = fs::absolute(original_path);
    
    if (!fs::exists(abs_original_path)) {
        throw std::runtime_error("Original file not found: " + abs_original_path.string());
    }
    
    std::string temp_filename = temp_dir + "/" + generateUniqueFilename(prefix, suffix);
    
    std::error_code ec;
    fs::copy_file(abs_original_path, temp_filename, fs::copy_options::overwrite_existing, ec);
    
    if (ec) {
        throw std::runtime_error("Failed to copy file: " + ec.message());
    }
    
    return temp_filename;
}

void cleanupTempFiles(const std::vector<std::string>& files) {
    std::lock_guard<std::mutex> lock(get_fs_mutex());
    namespace fs = std::filesystem;
    
    for (const auto& file_path : files) {
        if (!file_path.empty()) {
            try {
                std::error_code ec;
                if (fs::exists(file_path, ec)) {
                    fs::remove(file_path, ec);
                    // Игнорируем ошибки удаления
                }
            } catch (...) {
                // Игнорируем все исключения при удалении
            }
        }
    }
}

bool fileExists(const std::string& path) {
    // std::lock_guard<std::mutex> lock(get_fs_mutex());
    namespace fs = std::filesystem;
    
    try {
        std::error_code ec;
        return fs::exists(path, ec) && fs::is_regular_file(path, ec);
    } catch (...) {
        return false;
    }
}

bool ensureDirectoryExists(const std::string& path) {
    // std::lock_guard<std::mutex> lock(get_fs_mutex());
    namespace fs = std::filesystem;
    
    try {
        std::error_code ec;
        if (!fs::exists(path, ec)) {
            return fs::create_directories(path, ec);
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace file_utils