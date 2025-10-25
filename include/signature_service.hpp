#ifndef SIGNATURE_SERVICE_H
#define SIGNATURE_SERVICE_H

#include "common_types.hpp"
#include "message_service.hpp"
#include "document_hasher.hpp"
#include "logger.hpp"

#include <string>
#include <memory>
#include <mutex>

/**
 * @brief Сервис для подписания документов с подтверждением через SMS.
 * 
 * Класс предоставляет синхронный и асинхронный API для подписания PDF-документов
 * с наложением цифрового штампа, содержащего информацию о подписывающем лице
 * и хеш документа. Подтверждение подписи осуществляется через SMS-код.
 * 
 * @threadsafe Класс потокобезопасен.
 */
class SignatureService {
public:
    /// @brief Конструктор сервиса подписания
    SignatureService(const std::string& config_path);
    SignatureService(const Config& config);

    ~SignatureService();

    // Удаляем копирование и присваивание
    SignatureService(const SignatureService&) = delete;
    SignatureService& operator=(const SignatureService&) = delete;
    
    /// @brief Генерирует случайный код подтверждения (потокобезопасно)
    Result<std::string> generateConfirmationCode(const std::string& auth_token, 
                                                 int length = 4);

    /// @brief Подписание документа
    Result<DocumentSigningResult> signDocument(const std::string& auth_token,
                                               bool test_mode,
                                               bool need_all_user_data_fields,
                                               const UserIdentity& user_identity,
                                               const std::string& confirmation_code);
    
    /// @brief Отправка сообщения через IQSMS
    Result<ProviderSendingResult> sendMessageViaIqSms(const std::string& auth_token,
                                                      bool test_mode,
                                                      const std::string& phone_number, 
                                                      const std::string& message_text);

    /// @brief Устанавливает новые log/pass для MessageSerivce c сохранением в файл (потокобезопасно)
    Result<bool> setMessageServiceCreds(const std::string& auth_token,
                                        const std::string& login, 
                                        const std::string& password);
    
    /// @brief Обновление конфигурации (потокобезопасно)
    Result<> updateConfig(const std::string& auth_token, 
                          const Config& new_config);

private:
    /// @brief Генерация и отправка кода подтверждения
    // Result<MessageSendingResult> sendConfirmationCode(const std::string& phone_number, const std::string& confirmation_code);
    
    /// @brief Проверка валидности токена авторизации
    Result<bool> isAuthTokenValid(const std::string& auth_token);
    
    /// @brief Проверка валидности структуры (наличие всех полей)
    Result<bool> isUserIdentityHasAllFields(const UserIdentity& user_identity);

    /// @brief Проверка валидности структуры (наличие обязательных)
    Result<bool> isUserIdentityHasReqFields(const UserIdentity& user_identity);

    /// @brief Получение текущей конфигурации (потокобезопасно)
    Config getConfig();

    // ------------------- ВНУТРЕННИЕ МЕТОДЫ ПОДПИСАНИЯ ДОКУМЕНТА --------------------

    /// @brief Подготовка документа к подписанию
    Result<DocumentPreparationResult> prepareDocumentForSigning(const UserIdentity& user_identity);

    /// @brief Установка штампа на переданный пдф документ
    Result<DocumentSigningResult> createStampOnPdf(const UserIdentity& user_identity,
                                                   const Result<DocumentPreparationResult>& document_preparation_result, 
                                                   const Result<MessageSendingResult>& message_sending_result);

    /// @brief Отправка текста на номер телефона
    Result<MessageSendingResult> sendMessageToPhone(bool test_mode,
                                                    const std::string& phone_number, 
                                                    const std::string& message_text);

    // ------------------------------------------------------------------------------

    Config config_;                                     //< Текущая конфигурация сервиса
    std::mutex config_mutex_;                           //< Мьютекс для защиты доступа к конфигурации

    mutable std::mutex services_mutex_;                 //< Мьютекс для защиты доступа к подсервисам
    std::unique_ptr<MessageService> message_service_;   //< Сервис для отправки SMS сообщений
    std::unique_ptr<DocumentHasher> document_hasher_;   //< Сервис для генерации хешей документов
    std::unique_ptr<LoggerService> logger_;             //< Cервис для логирования
};

#endif // SIGNATURE_SERVICE_H
