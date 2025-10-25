#include "signature_service.hpp"
#include "wkhtml2pdf_wrapper.hpp"
#include "parsers.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

/**
 * @brief Печатает инструкцию по использованию программы
 */
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <config_file> <json_file>" << std::endl;
    std::cout << "  config_file - Path to configuration file (.ini)" << std::endl;
    std::cout << "  json_file   - Path to JSON file with user data" << std::endl;
}

/**
 * @brief Читает содержимое файла в строку
 */
std::string readFileToString(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief Выводит информацию о результате операции
 */
template<typename T>
void printResult(const Result<T>& result) {
    if (result.isSuccess()) {
        std::cout << "✓ Operation completed successfully" << std::endl;
        
        // Выводим дополнительную информацию в зависимости от типа результата
        if constexpr (std::is_same_v<T, DocumentPreparationResult>) {
            std::cout << "  Temporary PDF: " << result.getValue().temp_pdf_path << std::endl;
        }
        else if constexpr (std::is_same_v<T, MessageSendingResult>) {
            std::cout << "  Phone: " << result.getValue().phone_number << std::endl;
            std::cout << "  Code: " << result.getValue().confirmation_code << std::endl;
        }
        else if constexpr (std::is_same_v<T, DocumentSigningResult>) {
            std::cout << "  Signed PDF: " << result.getValue().signed_pdf_path << std::endl;
            std::cout << "  Document Hash: " << result.getValue().document_hash << std::endl;
            std::cout << "  Signing Time: " << result.getValue().signing_time << std::endl;
        }
    } else {
        std::cout << "✗ Error: " << result.getErrorMessage() << std::endl;
        std::cout << "  Error code: ";
        
        switch (result.getErrorCode()) {
            case ErrorCode::INVALID_JSON:
                std::cout << "INVALID_JSON"; break;
            case ErrorCode::FILE_IO_ERROR:
                std::cout << "FILE_IO_ERROR"; break;
            case ErrorCode::SMS_SEND_ERROR:
                std::cout << "SMS_SEND_ERROR"; break;
            case ErrorCode::AUTHENTICATION_FAILED:
                std::cout << "AUTHENTICATION_FAILED"; break;
            case ErrorCode::PDF_GENERATION_ERROR:
                std::cout << "PDF_GENERATION_ERROR"; break;
            case ErrorCode::STAMP_APPLICATION_ERROR:
                std::cout << "STAMP_APPLICATION_ERROR"; break;
            case ErrorCode::INVALID_CONFIG:
                std::cout << "INVALID_CONFIG"; break;
            case ErrorCode::SERVICE_SHUTDOWN:
                std::cout << "SERVICE_SHUTDOWN"; break;
            case ErrorCode::UNKNOWN_ERROR:
                std::cout << "UNKNOWN_ERROR"; break;
            default:
                std::cout << "SUCCESS"; break;
        }
        std::cout << std::endl;
    }
}

/**
 * @brief Запрашивает код подтверждения от пользователя
 */
std::string getConfirmationCodeFromUser() {
    std::string code;
    std::cout << "Enter confirmation code from SMS: ";
    std::cin >> code;
    return code;
}

int main(int argc, char* argv[]) {
    // Проверяем количество аргументов
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    // Получаем аргументы командной строки
    std::string config_file = argv[1];
    std::string json_file = argv[2];

    try {
        // Читаем JSON файл и парсим данные пользователя
        std::cout << "Reading JSON file: " << json_file << std::endl;
        std::string json_string = readFileToString(json_file);
        UserIdentity user_identity = parsers::parseJsonStringToUserIdentity(json_string);
        
        std::cout << "User: " << user_identity.first_name << " " 
                  << user_identity.last_name << std::endl;
        std::cout << "Phone: " << user_identity.phone_number << std::endl;

        // Создаем сервис
        std::cout << "Initializing signature service with config: " << config_file << std::endl;
        SignatureService signer(config_file);
        
        auto& converter = WkHtmlToPdfWrapper::getInstance();
        if (!converter.initialize()) {
            std::cerr << "Failed to initialize PDF converter" << std::endl;
            return 1;
        }

        std::string auth_token = "AUTH_TOKEN";
        std::string confirmation_code = "12345";
        
        auto sign_result = signer.signDocument(auth_token, true, false, user_identity, confirmation_code);
        
        printResult(sign_result);
        
        if (sign_result.isSuccess()) {
            std::cout << "\n✅ Document signed successfully!" << std::endl;
            std::cout << "   Output file: " << sign_result.getValue().signed_pdf_path << std::endl;
            converter.shutdown(true);
            return 0;
        } else {
            WkHtmlToPdfWrapper::getInstance().shutdown(false);
            return 1;
        }

        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        WkHtmlToPdfWrapper::getInstance().shutdown(false);
        return 1;
    }
}