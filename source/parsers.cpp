#include "parsers.hpp"
#include <stdexcept>
// #include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Config parsers::parseIniFileToConfig(const std::string& config_path) {
    Config config;
    std::ifstream file(config_path);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }
    
    std::string line;
    
    while (std::getline(file, line)) {
        // Удаляем возврат каретки (CR) если есть
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Удаляем пробелы в начале и конце
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        auto getValue = [](const std::string& line, size_t prefix_len) {
            std::string value = line.substr(prefix_len);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            return value;
        };
        
        if (line.find("html_template_path=") == 0) {
            config.html_template_path = getValue(line, 19);
        }
        else if (line.find("message_template_path=") == 0) {
            config.message_template_path = getValue(line, 22);
        }
        else if (line.find("env_file_path=") == 0) {
            config.env_file_path = getValue(line, 14);
        } 
        else if (line.find("log_file_path=") == 0) {
            config.log_file_path = getValue(line, 14);
        }
        else if (line.find("temp_dir=") == 0) {
            config.temp_dir = getValue(line, 9);
        }
        else if (line.find("fonts_path=") == 0) {
            config.fonts_dir = getValue(line, 11);
        }
        else if (line.find("output_pdf_dir=") == 0) {
            config.output_pdf_dir = getValue(line, 15);
        }
        else if (line.find("auth_token=") == 0) {
            config.auth_token = getValue(line, 11);
        }
    }
 
    return config;
}

UserIdentity parsers::parseJsonStringToUserIdentity(const std::string& json_string) {
    UserIdentity identity;

    try {
        json json_data = json::parse(json_string);
        
        identity.first_name = json_data.value("first_name", "");
        identity.middle_name = json_data.value("middle_name", "");
        identity.last_name = json_data.value("last_name", "");
        identity.passport_number = json_data.value("passport_number", "");
        identity.passport_series = json_data.value("passport_series", "");
        identity.passport_unite_code = json_data.value("passport_unite_code", "");
        identity.passport_issued_by = json_data.value("passport_issued_by", "");
        identity.passport_issued_date = json_data.value("passport_issued_date", "");
        identity.passport_birthday_date = json_data.value("passport_birthday_date", "");
        identity.passport_birthday_place = json_data.value("passport_birthday_place", "");
        identity.passport_registration_address = json_data.value("passport_registration_address", "");
        identity.passport_registration_date = json_data.value("passport_registration_date", "");
        identity.phone_number = json_data.value("phone_number", "");
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON parsing failed: ") + e.what());
    }

    return identity;
}

std::string parsers::parseUserIdentityToJsonString(const UserIdentity& user, bool pretty) {
    json j;

    j["first_name"] = user.first_name;
    j["middle_name"] = user.middle_name;
    j["last_name"] = user.last_name;
    j["passport_number"] = user.passport_number;
    j["passport_series"] = user.passport_series;
    j["passport_unite_code"] = user.passport_unite_code;
    j["passport_issued_by"] = user.passport_issued_by;
    j["passport_issued_date"] = user.passport_issued_date;
    j["passport_birthday_date"] = user.passport_birthday_date;
    j["passport_birthday_place"] = user.passport_birthday_place;
    j["passport_registration_address"] = user.passport_registration_address;
    j["passport_registration_date"] = user.passport_registration_date;
    j["email"] = user.email;
    j["phone_number"] = user.phone_number;

    return pretty ? j.dump(4) : j.dump();
}