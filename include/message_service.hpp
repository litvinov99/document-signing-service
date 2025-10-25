#ifndef MESSAGE_SERVICE_H
#define MESSAGE_SERVICE_H

#include <string>
#include <curl/curl.h>
#include <chrono>
#include <optional>

/// @brief Учетные данные для доступа к SMS API
struct Credentials {
    std::string login;        //< Логин для аутентификации в API
    std::string password;     //< Пароль для аутентификации в API
};

/// @brief Параметры SMS сообщения
struct MessageParams {
    std::string phone;        //< Номер телефона получателя
    std::string text;         //< Текст сообщения
};

/// @brief Результат запроса отправки сообщения
struct ProviderSendingResult {
    std::string status;       //< Статус отправки сообщения
    std::string description;  //< Описание результата отправки (в случае ошибки)
    std::string id;           //< ID сообщения в системе провайдера
};

/// @brief Результат запроса проверки баланса
struct ProviderBalanceResult {
    double balance;          //< Текущий баланс аккаунта
    std::string error;       //< Описание ошибки при запросе баланса
};

class MessageService {
public:
    /**
     * @brief Конструктор с путями к конфигурационным файлам
     * @param env_file_path Путь к файлу с учетными данными
     * @param template_file_path Путь к файлу с шаблоном сообщения
     */
    MessageService(const std::string& env_file_path, const std::string& message_template_path);

    MessageService(const MessageService&) = delete;
    MessageService& operator=(const MessageService&) = delete;
    
    /// @brief Генерирует случайный код подтверждения
    static std::string generateConfirmationCode(int length = 4);

    /// @brief Отправляет код подтверждения с шаблоном текста сообщения
    bool sendConfirmationCode(const std::string& phone, const std::string& code);
    
    /// @brief Отправляет переданный текст на телефон
    ProviderSendingResult sendConfirmationCodeWitnStatus(const std::string& phone, const std::string& text);

    /// @brief Отправляет переданный текст на телефон
    bool sendMessage(const std::string& phone, const std::string& text);

    /// @brief Отправляет переданный текст на телефон и возвращает статус сообщения
    ProviderSendingResult sendMessageWithResponseStatus(const std::string& phone, const std::string& text);

    /// @brief Отправить запрос на получение статуса по отправленному сообщению
    std::string submitCheckMessageStatus(ProviderSendingResult);

    /// @brief Проверяет валидность номера телефона
    static bool validatePhoneNumber(const std::string& phone);

    /// @brief Устанавливает новые log/pass
    bool setServiceCredentionals(const std::string& login, const std::string& password);
    
    /// @brief Устанавливает новый путь к файлу с учетными данными
    bool setEnvFilePath(const std::string& path);
    
    /// @brief Устанавливает новый путь к файлу с шаблоном сообщения
    bool setTemplateFilePath(const std::string& path);
    
private:
    // Конфигурационные параметры
    std::string env_file_path_;           //< Путь к файлу с учетными данными
    std::string message_template_path_;   //< Путь к файлу с шаблоном сообщения
    int timeout_seconds_;                 //< Таймаут HTTP запросов в секундах

    // HTTP клиент
    CURL* curl_handle_ = nullptr;         //< Handle для cURL запросов

    // Кэшированные данные и состояние
    mutable Credentials cached_credentials_;                            //< Кэшированные учетные данные
    mutable std::string cached_message_template_;                       //< Кэшированный шаблон сообщения
    mutable std::chrono::system_clock::time_point last_config_check_;   //< Время последней проверки конфигурации


    void reloadConfigIfNeeded();
    
    /// @brief Загружает учетные данные из файла
    Credentials loadCredentialsFromEnvFile();
    
    /// @brief Загружает шаблон сообщения из файла
    std::string loadMessageTemplate() const;
    
    /// @brief Выполняет HTTP запрос к API
    std::string performHttpRequest(const std::string& url);
    
    /// @brief Экранирует строку для URL
    std::string urlEncode(const std::string& str);

    /// @brief Устанавливает таймаут для HTTP запросов
    void setTimeout(int timeout_seconds);

    /// @brief Создает текст сообщения с кодом подтверждения
    std::string generateMessageWithConfirmationCode(const std::string& code);
    
    /// @brief Отправить сообщение
    bool submitSendMessageRequestQuick(const MessageParams& params);

    /// @brief Отправить запрос на отправку сообщения
    ProviderSendingResult submitSendMessageRequest(const MessageParams& params);

    /// @brief Отправить запрос на проверку баланса
    ProviderBalanceResult submitGetBalanceRequest();
};

#endif // SMS_SERVICE_H