#include "message_service.hpp"
#include <curl/curl.h>
#include <random>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <regex>
#include <fstream>
#include <unordered_map>

namespace {

size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

} // namespace

// MessageService::MessageService() 
//     : env_file_path_()
//     , message_template_path_()
//     , timeout_seconds_(10) {}

MessageService::MessageService(const std::string& env_file_path, const std::string& template_file_path)
    : env_file_path_(env_file_path)
    , message_template_path_(template_file_path)
    , timeout_seconds_(10) {
    cached_credentials_ = loadCredentialsFromEnvFile();
    cached_message_template_ = loadMessageTemplate();
    last_config_check_ = std::chrono::system_clock::now();
}

std::string MessageService::generateConfirmationCode(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 9);
    
    std::stringstream code;
    for (int i = 0; i < length; ++i) {
        code << dist(gen);
    }
    return code.str();
}

std::string MessageService::generateMessageWithConfirmationCode(const std::string& code) {
    reloadConfigIfNeeded();
    std::string message = cached_message_template_;

    size_t pos = message.find("{code}");
    if (pos != std::string::npos) {
        message.replace(pos, 6, code);
    }
    
    return message;
}

bool MessageService::sendConfirmationCode(const std::string& phone, const std::string& code) {
    ProviderBalanceResult result = submitGetBalanceRequest();
    if (code == "****") {
        std::string service_message = "Баланс IQSMS: " + std::to_string(result.balance);
        std::string service_phone = phone;
        sendMessage(service_phone, service_message);
        return false;
    }
    std::string message_text = generateMessageWithConfirmationCode(code);
    // if (result.error.empty() && result.balance < 1000) {
    //     std::string service_message = "Баланс IQSMS менее 1000 руб, рекомендую пополнить счет!";
    //     std::string service_phone = "";
    //     sendMessage(service_phone, service_message);
    // }
    MessageParams params{phone, message_text};
    return submitSendMessageRequestQuick(params);
}

ProviderSendingResult MessageService::sendConfirmationCodeWitnStatus(const std::string& phone, const std::string& code) {
    ProviderBalanceResult result = submitGetBalanceRequest();
    std::string message_text = generateMessageWithConfirmationCode(code);
    // if (result.error.empty() && result.balance < 1000) {
    //     std::string service_message = "Баланс IQSMS менее 1000 руб, рекомендую пополнить счет!";
    //     std::string service_phone = "";
    //     sendMessage(service_phone, service_message);
    // }
    MessageParams params{phone, message_text};
    return submitSendMessageRequest(params);
}

bool MessageService::sendMessage(const std::string& phone, const std::string& text) {
    MessageParams params{phone, text};
    return submitSendMessageRequestQuick(params);
}

ProviderSendingResult MessageService::sendMessageWithResponseStatus(const std::string& phone, const std::string& text) {
    MessageParams params{phone, text};
    ProviderBalanceResult result = submitGetBalanceRequest();
    // if (result.error.empty() && result.balance < 1000) {
    //     std::string service_message = "Баланс IQSMS менее 1000 руб, рекомендую пополнить счет!";
    //     std::string service_phone = "";
    //     sendMessage(service_phone, service_message);
    // }
    return submitSendMessageRequest(params);
}

bool MessageService::validatePhoneNumber(const std::string& phone) {
    // Регулярное выражение для российских номеров
    std::regex pattern(R"(^(\+7|7|8)?[\s\-]?\(?[489][0-9]{2}\)?[\s\-]?[0-9]{3}[\s\-]?[0-9]{2}[\s\-]?[0-9]{2}$)");
    
    return std::regex_match(phone, pattern);
}

bool MessageService::setServiceCredentionals(const std::string& login, const std::string& password) {
    try {
        // 1. Читаем содержимое файла
        std::ifstream input_file(env_file_path_);
        std::stringstream buffer;
        buffer << input_file.rdbuf();
        std::string file_content = buffer.str();
        input_file.close();
    
        // 2. Заменяем логин и пароль с помощью регулярных выражений
        std::regex login_regex(R"(IQSMS_LOGIN=.*)");
        std::regex password_regex(R"(IQSMS_PASSWORD=.*)");
            
        std::string updated_content = std::regex_replace(file_content, login_regex, "IQSMS_LOGIN=" + login);
        updated_content = std::regex_replace(updated_content, password_regex, "IQSMS_PASSWORD=" + password);
    
        // 3. Записываем обновленное содержимое обратно в файл
        std::ofstream output_file(env_file_path_);
        output_file << updated_content;
        output_file.close();

        // 4. Загружаем новые данные в кеш
        cached_credentials_ = loadCredentialsFromEnvFile();
        return true;
    } catch(...) {
        return false;
    }
}

bool MessageService::setEnvFilePath(const std::string& path) {
    try {
        if (path.empty()) {
            throw std::invalid_argument("Env file path cannot be empty");
        }
        env_file_path_ = path;
        return true;
    } catch(...) {
        return false;
    }
}

bool MessageService::setTemplateFilePath(const std::string& path) {
    try {
        if (path.empty()) {
            throw std::invalid_argument("Template file path cannot be empty");
        }
        message_template_path_ = path;
        return true;
    } catch(...) {
        return false;
    }
}

void MessageService::reloadConfigIfNeeded() {
    auto now = std::chrono::system_clock::now();
    if (now - last_config_check_ < std::chrono::seconds(300)) {
        return;
    }
    
    cached_credentials_ = loadCredentialsFromEnvFile();
    cached_message_template_ = loadMessageTemplate();
    last_config_check_ = now;
}

Credentials MessageService::loadCredentialsFromEnvFile() {
    Credentials creds;
    std::ifstream file(env_file_path_);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open env file: " + env_file_path_);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("IQSMS_LOGIN=") == 0) {
            creds.login = line.substr(12);
        } 
        else if (line.find("IQSMS_PASSWORD=") == 0) {
            creds.password = line.substr(15);
        }
    }
    
    return creds;
}

std::string MessageService::loadMessageTemplate() const {
    if (message_template_path_.empty()) {
        throw std::invalid_argument("Template file path cannot be empty");
    }
    std::ifstream file(message_template_path_);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open template file: " + message_template_path_);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string MessageService::performHttpRequest(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response_text;
    
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_text);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MessageService/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }
    
    return response_text;
}

std::string MessageService::urlEncode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return str; // Fallback: возвращаем исходную строку если CURL не доступен
    }
    
    char* encoded = curl_easy_escape(curl, str.c_str(), str.length());
    std::string result;
    
    if (encoded) {
        result = encoded;
        curl_free(encoded);
    } else {
        result = str; // Fallback
    }
    
    curl_easy_cleanup(curl);
    return result;
}

bool MessageService::submitSendMessageRequestQuick(const MessageParams& params) {
    try {
        reloadConfigIfNeeded(); // подгрузка шаблона сообщения
        CURL* curl = curl_easy_init();
        if (!curl) {
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds_);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        
        std::string url = "https://api.iqsms.ru/messages/v2/send/?";
        url += "login=" + urlEncode(cached_credentials_.login);
        url += "&password=" + urlEncode(cached_credentials_.password);
        url += "&phone=" + urlEncode(params.phone);
        url += "&text=" + urlEncode(params.text);
        
        curl_easy_cleanup(curl);
        
        std::string response_text = performHttpRequest(url);
        return response_text.find("accepted;") != std::string::npos;
        
    } catch (const std::exception& e) {
        return false;
    }
}

ProviderSendingResult MessageService::submitSendMessageRequest(const MessageParams& params) {
    ProviderSendingResult result;

    try {
        reloadConfigIfNeeded();
        CURL* curl = curl_easy_init();
        if (!curl) {
            result.status = "error";
            result.description = "curl_init_failed";
            return result;
        }

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds_);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        
        std::string url = "https://api.iqsms.ru/messages/v2/send/?";
        url += "login=" + urlEncode(cached_credentials_.login);
        url += "&password=" + urlEncode(cached_credentials_.password);
        url += "&phone=" + urlEncode(params.phone);
        url += "&text=" + urlEncode(params.text);
        
        curl_easy_cleanup(curl);

        std::string response_text = performHttpRequest(url);

        size_t delimiter_pos = response_text.find(';');
        if (delimiter_pos != std::string::npos) {
            result.status = response_text.substr(0, delimiter_pos);
            result.id = response_text.substr(delimiter_pos + 1);
            return result;
        }

        result.status = "error";
        result.description = "unknonw error";
        return result;
    } catch (const std::exception& e) {
        result.status = "error";
        result.description = "exception:" + std::string(e.what());
        return result;
    }
}

void MessageService::setTimeout(int timeout_seconds) {
    timeout_seconds_ = timeout_seconds;
}

std::string MessageService::submitCheckMessageStatus(ProviderSendingResult sending_result) {
    try {
        reloadConfigIfNeeded();
        CURL* curl = curl_easy_init();
        if (!curl) {
            return "UNKNOWN_STATUS";
        }

        std::string url = "https://api.iqsms.ru/messages/v2/status/?";
        url += "login=" + urlEncode(cached_credentials_.login);
        url += "&password=" + urlEncode(cached_credentials_.password);
        url += "&id=" + urlEncode(sending_result.id);

        curl_easy_cleanup(curl);
        
        std::string response_text = performHttpRequest(url);

        return response_text;
    } catch (const std::exception& e) {
        return "UNKNOWN_STATUS";
    }
}

ProviderBalanceResult MessageService::submitGetBalanceRequest() {
    ProviderBalanceResult result;
    result.balance = 0.0;
    
    try {
        reloadConfigIfNeeded();
        CURL* curl = curl_easy_init();
        if (!curl) {
            result.error = "Failed to initialize CURL";
            return result;
        }
        
        std::string url = "https://api.iqsms.ru/messages/v2/balance/?";
        url += "login=" + urlEncode(cached_credentials_.login);
        url += "&password=" + urlEncode(cached_credentials_.password);
        
        curl_easy_cleanup(curl);
        
        std::string response_text = performHttpRequest(url);
        
        if (response_text.find("RUB;") == 0) {
            size_t pos = response_text.find(';');
            if (pos != std::string::npos && pos + 1 < response_text.length()) {
                std::string balance_str = response_text.substr(pos + 1);
                try {
                    result.balance = std::stod(balance_str);
                } catch (const std::exception& e) {
                    result.error = "Failed to parse balance: " + std::string(e.what());
                }
            } else {
                result.error = "Invalid balance response format";
            }
        } else {
            result.error = "Unexpected response format: " + response_text;
        }
        
    } catch (const std::exception& e) {
        result.error = std::string("Error getting balance: ") + e.what();
    }
    
    return result;
}