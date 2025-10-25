#ifndef PARSERS_H
#define PARSERS_H

#include <string>
#include "common_types.hpp"

namespace parsers {
    /// @brief Парсит INI файл в структуру Config
    Config parseIniFileToConfig(const std::string& config_path);

    /// @brief Парсит JSON строку в структуру UserIdentity
    UserIdentity parseJsonStringToUserIdentity(const std::string& json_string);

    /// @brief Парсит структуру UserIdentity в JSON строку
    std::string parseUserIdentityToJsonString(const UserIdentity& user, bool pretty = true);
}

#endif // PARSERS_H