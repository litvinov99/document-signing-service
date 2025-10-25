#include "html_template_processor.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <utility>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

bool readFileToString(const std::string& file_path, std::string& content) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    
    return true;
}

bool writeStringToFile(const std::string& file_path, const std::string& content) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(content.c_str(), content.size());
    return file.good();
}

std::string replacePlaceholdersInContent(const json& data, const std::string& html_content) {
    std::string result = html_content;
    
    // Проходим по всем ключам JSON и заменяем плейсхолдеры
    for (auto& [key, value] : data.items()) {
        std::string placeholder = key;
        std::string replacement = value.is_string() ? value.get<std::string>() : value.dump();
        
        // Простая замена всех вхождений
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }
    
    return result;
}
    // std::string result;
    // result.reserve(html_content.size() * 2);
    
    // // Предварительно готовим replacements
    // std::vector<std::pair<std::string, std::string>> replacements;
    // for (auto& [key, value] : data.items()) {
    //     replacements.emplace_back(
    //         key,
    //         value.is_string() ? value.get<std::string>() : value.dump()
    //     );
    // }
    
    // // Сортируем по убыванию длины для приоритета длинных плейсхолдеров
    // std::sort(replacements.begin(), replacements.end(),
    //     [](const auto& a, const auto& b) {
    //         return a.first.size() > b.first.size();
    //     });
    
    // size_t pos = 0;
    // const size_t content_size = html_content.size();
    
    // while (pos < content_size) {
    //     bool replaced = false;
        
    //     // Проверяем все плейсхолдеры от самых длинных
    //     for (const auto& [placeholder, replacement] : replacements) {
    //         const size_t placeholder_size = placeholder.size();
    //         if (pos + placeholder_size <= content_size &&
    //             html_content.compare(pos, placeholder_size, placeholder) == 0) {
                
    //             result.append(replacement);
    //             pos += placeholder_size;
    //             replaced = true;
    //             break;
    //         }
    //     }
        
    //     if (!replaced) {
    //         result += html_content[pos++];
    //     }
    // }
    
    // return result;
// }

} // namespace

// Статические переменные
std::unordered_map<std::string, std::string> HtmlTemplateProcessor::template_cache_;
std::mutex HtmlTemplateProcessor::cache_mutex_;
size_t HtmlTemplateProcessor::max_cache_size_ = 100;

bool HtmlTemplateProcessor::getCachedFileContent(const std::string& file_path, 
                                               std::string& content, 
                                               bool use_cache) {
    if (!use_cache) {
        return readFileToString(file_path, content);
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = template_cache_.find(file_path);
    if (it != template_cache_.end()) {
        content = it->second;
        return true;
    }

    if (!readFileToString(file_path, content)) {
        return false;
    }

    // Управление размером кэша
    if (template_cache_.size() >= max_cache_size_) {
        template_cache_.erase(template_cache_.begin());
    }
    
    template_cache_[file_path] = content;
    return true;
}

void HtmlTemplateProcessor::clearTemplateCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    template_cache_.clear();
}

void HtmlTemplateProcessor::setTemplateCacheSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    max_cache_size_ = max_size;
    
    while (template_cache_.size() > max_cache_size_) {
        template_cache_.erase(template_cache_.begin());
    }
}

bool HtmlTemplateProcessor::replacePlaceholdersFromJsonFile(const std::string& json_file_path, 
                                                          const std::string& html_file_path,
                                                          bool use_cache) {
    if (!fileExists(json_file_path) || !fileExists(html_file_path)) {
        return false;
    }

    std::string json_content;
    if (!readFileToString(json_file_path, json_content)) {
        return false;
    }

    json data;
    try {
        data = json::parse(json_content);
    } catch (const json::parse_error&) {
        return false;
    }

    std::string html_content;
    if (!getCachedFileContent(html_file_path, html_content, use_cache)) {
        return false;
    }

    std::string processed_content = replacePlaceholdersInContent(data, html_content);
    return writeStringToFile(html_file_path, processed_content);
}

bool HtmlTemplateProcessor::replacePlaceholdersFromJsonData(const std::string& json_data, 
                                                            const std::string& html_file_path) {
    if (!fileExists(html_file_path)) {
        return false;
    }

    json data;
    try {
        data = json::parse(json_data);
    } catch (const json::parse_error&) {
        return false;
    }

    std::string html_content;
    if (!readFileToString(html_file_path, html_content)) {
        return false;
    }

    std::string processed_content = replacePlaceholdersInContent(data, html_content);
    return writeStringToFile(html_file_path, processed_content);
}

bool HtmlTemplateProcessor::processTemplateToNewFile(const std::string& json_file_path,
                                                   const std::string& input_html_file_path,
                                                   const std::string& output_html_file_path,
                                                   bool use_cache) {
    if (!fileExists(json_file_path) || !fileExists(input_html_file_path)) {
        return false;
    }

    std::string json_content;
    if (!readFileToString(json_file_path, json_content)) {
        return false;
    }

    json data;
    try {
        data = json::parse(json_content);
    } catch (const json::parse_error&) {
        return false;
    }

    std::string html_content;
    if (!getCachedFileContent(input_html_file_path, html_content, use_cache)) {
        return false;
    }

    std::string processed_content = replacePlaceholdersInContent(data, html_content);
    return writeStringToFile(output_html_file_path, processed_content);
}

std::string HtmlTemplateProcessor::processHtmlString(const std::string& json_data, 
                                                   const std::string& html_content) {
    json data;
    try {
        data = json::parse(json_data);
    } catch (const json::parse_error&) {
        throw std::runtime_error("JSON parse error");
    }
    
    return replacePlaceholdersInContent(data, html_content);
}

std::string HtmlTemplateProcessor::processHtmlStringFromFile(const std::string& json_file_path,
                                                           const std::string& html_content) {
    if (!fileExists(json_file_path)) {
        throw std::runtime_error("JSON file not found");
    }

    std::string json_content;
    if (!readFileToString(json_file_path, json_content)) {
        throw std::runtime_error("Failed to read JSON file");
    }

    json data;
    try {
        data = json::parse(json_content);
    } catch (const json::parse_error&) {
        throw std::runtime_error("JSON parse error");
    }
    
    return replacePlaceholdersInContent(data, html_content);
}

bool HtmlTemplateProcessor::fileExists(const std::string& file_path) {
    return fs::exists(file_path) && fs::is_regular_file(file_path);
}

std::string HtmlTemplateProcessor::getFileExtension(const std::string& file_path) {
    fs::path path(file_path);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}

bool HtmlTemplateProcessor::readFileToString(const std::string& file_path, std::string& content) {
    return ::readFileToString(file_path, content);
}

bool HtmlTemplateProcessor::writeStringToFile(const std::string& file_path, const std::string& content) {
    return ::writeStringToFile(file_path, content);
}

std::string HtmlTemplateProcessor::replacePlaceholdersInContent(const json& data, 
                                                                const std::string& html_content) {
    return ::replacePlaceholdersInContent(data, html_content);
}