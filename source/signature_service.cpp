#include "signature_service.hpp"
#include "common_types.hpp"
#include "parsers.hpp"
#include "file_utils.hpp"
#include "time_utils.hpp"
#include "document_hasher.hpp"
#include "message_service.hpp"
#include "pdf_stamper.hpp"
#include "logger.hpp"
#include "html_template_processor.hpp"
#include "html2pdf_converter.hpp"
#include "wkhtml2pdf_wrapper.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

SignatureService::SignatureService(const std::string& config_path) 
{   
    // Инициализация конфигурации
    config_ = parsers::parseIniFileToConfig(config_path);

    // Создаем необходимые директории
    file_utils::ensureDirectoryExists(config_.temp_dir);
    file_utils::ensureDirectoryExists(config_.output_pdf_dir);

    // Инициализация подсервисов
    message_service_ = std::make_unique<MessageService>(config_.env_file_path, config_.message_template_path);
    document_hasher_ = std::make_unique<DocumentHasher>();
    logger_ = std::make_unique<LoggerService>(config_.log_file_path);
}

SignatureService::SignatureService(const Config& config) {
    // Инициализация конфигурации
    config_ = config;

    // Создаем необходимые директории
    file_utils::ensureDirectoryExists(config_.temp_dir);
    file_utils::ensureDirectoryExists(config_.output_pdf_dir);

    // Инициализация подсервисов
    message_service_ = std::make_unique<MessageService>(config_.env_file_path, config_.message_template_path);
    document_hasher_ = std::make_unique<DocumentHasher>();
    logger_ = std::make_unique<LoggerService>(config_.log_file_path);
}

SignatureService::~SignatureService() {
    // WkHtmlToPdfWrapper::getInstance().shutdown(true);
}

Result<bool> SignatureService::setMessageServiceCreds(const std::string& auth_token,
                                                      const std::string& login, 
                                                      const std::string& password) {
    auto auth_result = isAuthTokenValid(auth_token);
    if (auth_result.isError()) {
        return Result<bool>::error(auth_result.getErrorCode(), auth_result.getErrorMessage());
    }

    std::lock_guard<std::mutex> lock(services_mutex_);
    
    if (!message_service_->setServiceCredentionals(login, password)) {
        return Result<bool>::error(
            ErrorCode::CREDENTIALS_ERROR, 
            "The message service credentials could not be set");
    }
    
    return Result<bool>::success(true);
}

Config SignatureService::getConfig() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

Result<> SignatureService::updateConfig(const std::string& auth_token, const Config& new_config) {
    auto auth_result = isAuthTokenValid(auth_token);
    if (auth_result.isError()) {
        return Result<>::error(auth_result.getErrorCode(), auth_result.getErrorMessage());
    }
    
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = new_config;
    
    return Result<>::success();
}

Result<std::string> SignatureService::generateConfirmationCode(const std::string& auth_token, int length) {
    auto auth_result = isAuthTokenValid(auth_token);
    if (auth_result.isError()) {
        return Result<std::string>::error(auth_result.getErrorCode(), auth_result.getErrorMessage());
    }

    std::lock_guard<std::mutex> lock(services_mutex_);
    std::string code = MessageService::generateConfirmationCode(length);
    return Result<std::string>::success(code);
}

Result<bool> SignatureService::isAuthTokenValid(const std::string& auth_token) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    if (config_.auth_token != auth_token) {
        return Result<bool>::error(
            ErrorCode::INVALID_AUTH_TOKEN, 
            "Invalid authorization token");
    }

    return Result<bool>::success(true);
}

Result<bool> SignatureService::isUserIdentityHasAllFields(const UserIdentity& user) {
    if (user.first_name.empty() ||
        user.middle_name.empty() ||
        user.last_name.empty() ||
        user.passport_series.empty() ||
        user.passport_number.empty() ||
        user.passport_issued_by.empty() ||
        user.passport_issued_date.empty() ||
        user.passport_unite_code.empty() ||
        user.passport_birthday_date.empty() ||
        user.passport_birthday_place.empty() ||
        user.passport_registration_address.empty() ||
        user.passport_registration_date.empty() ||
        user.email.empty() ||
        user.phone_number.empty()) {
        return Result<bool>::error(ErrorCode::INVALID_USER_DATA, "Invalid user data");
    }
    
    return Result<bool>::success(true);
}

Result<bool> SignatureService::isUserIdentityHasReqFields(const UserIdentity& user) {
    if (user.first_name.empty() ||
        user.middle_name.empty() ||
        user.last_name.empty() ||
        user.phone_number.empty()) {
        return Result<bool>::error(ErrorCode::INVALID_USER_DATA, "Invalid user data");
    }
    
    return Result<bool>::success(true);
}

Result<DocumentSigningResult> SignatureService::signDocument(const std::string& auth_token,
                                                             bool test_mode,
                                                             bool need_all_user_data_fields,
                                                             const UserIdentity& user_identity,
                                                             const std::string& confirmation_code) {
    try {
        // 0. Валидация запроса
        auto auth_result = isAuthTokenValid(auth_token);
        if (auth_result.isError()) {
            logger_->logError("SignatureService: authentication failed");
            return Result<DocumentSigningResult>::error(
                auth_result.getErrorCode(), 
                auth_result.getErrorMessage());
        }

        if (confirmation_code == "LOG_ON") {
            logger_->enable();
            logger_->logInfo("SignatureService: logger was started");
            return Result<DocumentSigningResult>::error(
            ErrorCode::UNKNOWN_ERROR, 
                std::string("Unexpected error in signDocument: ")
            );
        } else if (confirmation_code == "LOG_OFF") {
            logger_->logInfo("SignatureService: logger was stoped");
            logger_->disable();
            return Result<DocumentSigningResult>::error(
            ErrorCode::UNKNOWN_ERROR, 
                std::string("Unexpected error in signDocument: ")
            );
        }

        // 1. Валидация данных пользователя 
        // (проверяется наличие всех заполненных полей или только обязательных)
        auto validateUser = [&]() -> Result<bool> {
            if (need_all_user_data_fields) {
                return isUserIdentityHasAllFields(user_identity);
            } else {
                return isUserIdentityHasReqFields(user_identity);
            }
        };

        auto validation_result = validateUser();
        if (validation_result.isError()) {
            logger_->logError("SignatureService: user validation failed for: " + user_identity.phone_number);
            return Result<DocumentSigningResult>::error(
                validation_result.getErrorCode(), 
                validation_result.getErrorMessage());
        }

        // 3. Подготовка документа к подписанию
        logger_->logInfo("SignatureService: preparing document for signing");
        Result<DocumentPreparationResult> prep_result = prepareDocumentForSigning(user_identity);
        if (prep_result.isError()) {
            logger_->logError("SignatureService: document preparation failed");
            return Result<DocumentSigningResult>::error(
                prep_result.getErrorCode(), 
                prep_result.getErrorMessage());
        }

        // 4. Отправка сообщения
        Result<MessageSendingResult> msg_result = sendMessageToPhone(test_mode, user_identity.phone_number, confirmation_code);
        if (msg_result.isError()) {
            logger_->logError("SignatureService: message sending failed");
            return Result<DocumentSigningResult>::error(
                msg_result.getErrorCode(), 
                msg_result.getErrorMessage());
        }

        // 5. Подписание документа
        logger_->logInfo("SignatureService: creating PDF stamp");
        Result<DocumentSigningResult> sign_result = createStampOnPdf(user_identity, prep_result, msg_result);
        if (sign_result.isError()) {
            logger_->logError("SignatureService: PDF stamp creation failed");
            return Result<DocumentSigningResult>::error(
                sign_result.getErrorCode(), 
                sign_result.getErrorMessage());
        }

        logger_->logSuccess("SignatureService: document signed successfully for: " + user_identity.phone_number);
        return sign_result;

    } catch (const std::exception& e) {
        // Глобальная обработка исключений
        logger_->logError("SignatureService: unexpected error in signDocument: " + std::string(e.what()));
        return Result<DocumentSigningResult>::error(
            ErrorCode::UNKNOWN_ERROR, 
            std::string("Unexpected error in signDocument: ") + e.what()
        );
    } catch (...) {
        // Обработка неизвестных исключений
        return Result<DocumentSigningResult>::error(
            ErrorCode::UNKNOWN_ERROR, 
            "Unknown unexpected error in signDocument"
        );
    }
}

Result<DocumentPreparationResult> SignatureService::prepareDocumentForSigning(const UserIdentity& user_identity) {
    DocumentPreparationResult result;
    
    try {
        Config current_config = getConfig();

        // 1. Создаем временную копию c уникальным именем HTML шаблона
        result.temp_html_path = file_utils::createTempCopyWithUniqueFilename(current_config.html_template_path, 
                                                                                "template_", ".html", current_config.temp_dir);
        
        // 2. Заполняем плейсхолдеры данными пользователя
        if(!HtmlTemplateProcessor::replacePlaceholdersFromJsonData(parsers::parseUserIdentityToJsonString(user_identity), 
                                                                   result.temp_html_path)) {
            file_utils::cleanupTempFiles({result.temp_html_path});
            logger_->logError("SignatureService: HTML template processing failed");
            return Result<DocumentPreparationResult>::error(
                ErrorCode::HTML_REPLACE_ERROR, 
                "Failed to replace user identity in HTML-template"
            );                                                          
        }
        
        // 3. Генерируем имя для PDF 
        result.temp_pdf_path = current_config.temp_dir + "/" + file_utils::generateUniqueFilename("temp_document_", ".pdf");
        
        // 4. Конверация в PDF из HTML
        if (!WkHtmlToPdfWrapper::getInstance().convertSync(result.temp_html_path, result.temp_pdf_path)) {
            file_utils::cleanupTempFiles({result.temp_html_path});
            logger_->logError("SignatureService: HTML to PDF conversion failed");
            return Result<DocumentPreparationResult>::error(
                ErrorCode::PDF_GENERATION_ERROR, 
                "Failed to convert HTML to PDF"
            );
        }

        logger_->logInfo("SignatureService: document prepared successfully");

    } catch (const std::exception& e) {
        file_utils::cleanupTempFiles({result.temp_html_path, result.temp_pdf_path});
        logger_->logError("SignatureService: document preparation exception: " + std::string(e.what()));
        return Result<DocumentPreparationResult>::error(
            ErrorCode::UNKNOWN_ERROR, 
            std::string("Unknown preparation failed: ") + e.what()
        );
    }
    
    return Result<DocumentPreparationResult>::success(result);
}

Result<MessageSendingResult> SignatureService::sendMessageToPhone(bool test_mode,
                                                                  const std::string& phone_number, 
                                                                  const std::string& message_text) {    
    MessageSendingResult result;

    try{
        // 0. Защищаем доступ к message_service_
        std::lock_guard<std::mutex> lock(services_mutex_);

        // 1. Запись данных результат
        result.phone_number = phone_number;
        result.message_text = message_text; // здесь поле содержит 4-х значный код подтверждения

        // 2. Отправка текста
        if (!test_mode) {
            ProviderSendingResult sending_result = message_service_->sendConfirmationCodeWitnStatus(result.phone_number, result.message_text);
            logger_->logInfo("SignatureService: sending SMS to: " + phone_number);
            logger_->logInfo("SignatureService: Responcse from IQSMS: " + sending_result.status + ";" + sending_result.id);
            std::string sending_status = message_service_->submitCheckMessageStatus(sending_result);
            logger_->logInfo("SignatureService: SMS status from IQSMS: " + sending_status);
            if (sending_result.status != "accepted") {
                logger_->logError("SignatureService: SMS sending failed to: " + phone_number);
                return Result<MessageSendingResult>::error(
                    ErrorCode::SMS_SEND_ERROR, 
                    "Message sending failed!"
                    );
            }
            // if (!message_service_->sendConfirmationCode(result.phone_number, result.message_text)) {
            //     logger_->logError("SignatureService: SMS sending failed to: " + phone_number);
            //     return Result<MessageSendingResult>::error(
            //         ErrorCode::SMS_SEND_ERROR, 
            //         "Message sending failed!"
            //         );
            // }

        } else {
            logger_->logInfo("SignatureService: test mode - SMS not sent to: " + phone_number);
        }

    } catch (const std::exception& e) {
        logger_->logError("SignatureService: SMS sending exception: " + std::string(e.what()));
        return Result<MessageSendingResult>::error(
            ErrorCode::UNKNOWN_ERROR, 
            std::string("Unknown message sending failed: ") + e.what()
        );
    }

    return Result<MessageSendingResult>::success(result);
}

Result<ProviderSendingResult> SignatureService::sendMessageViaIqSms(const std::string& auth_token,
                                                                    bool test_mode,
                                                                    const std::string& phone_number, 
                                                                    const std::string& message_text) {
    ProviderSendingResult result;

    try {
        // 0. Валидация запроса
        auto auth_result = isAuthTokenValid(auth_token);
        if (auth_result.isError()) {
            return Result<ProviderSendingResult>::error(
                auth_result.getErrorCode(), 
                auth_result.getErrorMessage());
        }

        // 1. Защищаем доступ к message_service_
        std::lock_guard<std::mutex> lock(services_mutex_);

        // 2. Отправка сообщения и запись ответа в результат результат
        result = message_service_->sendMessageWithResponseStatus(phone_number, message_text);

    } catch (const std::exception& e) {
        return Result<ProviderSendingResult>::error(
            ErrorCode::UNKNOWN_ERROR, 
            std::string("Message sending failed: ") + e.what()
        );
    }

    return Result<ProviderSendingResult>::success(result);
}

Result<DocumentSigningResult> SignatureService::createStampOnPdf(const UserIdentity& user_identity,
                                const Result<DocumentPreparationResult>& document_preparation_result, 
                                const Result<MessageSendingResult>& message_sending_result) {
    
    DocumentSigningResult result;

    try {
        // 1. Проверяем, что предыдущие результаты успешны
        if (document_preparation_result.isError() || message_sending_result.isError()) {
            return Result<DocumentSigningResult>::error(
                message_sending_result.getErrorCode(),
                message_sending_result.getErrorMessage()
            );
        }

        const auto& prep_result = document_preparation_result.getValue();
        const auto& msg_result = message_sending_result.getValue();

        // 2. Заполняем результат
        result.first_name = user_identity.first_name;
        result.middle_name = user_identity.middle_name;
        result.last_name = user_identity.last_name;
        result.confirmation_code = msg_result.message_text;
        result.phone_number = msg_result.phone_number;
        result.signing_time = time_utils::getCurrentTimeWithTimezone(3, 0);

        // 3. Генерация хеша (защищаем доступ к document_hasher_)
        std::vector<uint8_t> hash;
        {
            std::lock_guard<std::mutex> lock(services_mutex_);
            hash = document_hasher_->calculateCompositeHash(
                prep_result.temp_pdf_path, 
                result.first_name + " " + result.middle_name + " " + result.last_name,
                result.phone_number, 
                result.confirmation_code, 
                result.signing_time
            );
        }
        result.document_hash = DocumentHasher::hashToHex(hash);
        
        // 4. Подготовка данных для штампа
        SignerData signer_data;
        signer_data.identity = user_identity;
        signer_data.signature.confirmation_code = result.confirmation_code;
        signer_data.signature.document_hash = result.document_hash;
        signer_data.signature.signing_time = result.signing_time;

        // 5. Генерируем уникальное имя для подписанного PDF
        result.signed_pdf_path = config_.output_pdf_dir + "/" 
                                + file_utils::generateUniqueFilename("signed_document_", ".pdf");
        
        // 6. Наложение штампа
        if (!PdfStamper::applyStamp(prep_result.temp_pdf_path, result.signed_pdf_path, signer_data)) {
            file_utils::cleanupTempFiles({document_preparation_result.getValue().temp_pdf_path});
            return Result<DocumentSigningResult>::error(
                ErrorCode::STAMP_APPLICATION_ERROR, 
                "Stamp appliacation failed!"
            );
        }
    } catch (const std::exception& e) {
        file_utils::cleanupTempFiles({document_preparation_result.getValue().temp_pdf_path, 
                          document_preparation_result.getValue().temp_html_path});
        return Result<DocumentSigningResult>::error(
            ErrorCode::UNKNOWN_ERROR, 

            std::string("Document signing failed: ") + e.what()
        );
    }

    file_utils::cleanupTempFiles({document_preparation_result.getValue().temp_pdf_path, 
                      document_preparation_result.getValue().temp_html_path});
    return Result<DocumentSigningResult>::success(result);
}
