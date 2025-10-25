#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <string>
#include <any>

/// @brief Конфигурация сервиса подписания.
struct Config {
    std::string html_template_path;     //< Путь к HTML-шаблону соглашения
    std::string message_template_path;  //< Путь к txt-шаблону сообщения
    std::string env_file_path;          //< Путь к env-файлу с log/pass для MessageService
    std::string log_file_path;          //< Путь к txt-файлу для логирования
    std::string temp_dir;               //< Директория для промежуточных файлов
    std::string fonts_dir;              //< Путь к директории со шрифтами для работы PoDoFo (не используется)
    std::string output_pdf_dir;         //< Директория для подписанных документов
    std::string auth_token;             //< Токен авторизации для проверки валидности запросов
};

/// @brief Структура содержит результат этапа подготовки документа к подписанию
struct DocumentPreparationResult {
    std::string temp_html_path;         //< Путь к временному HTML файлу
    std::string temp_pdf_path;          //< Путь к временному PDF файлу
};

/// @brief Структура содержит результат этапа отправки сообщения
struct MessageSendingResult {
    std::string phone_number;           //< Номер телефона, на который отправлено сообщение
    std::string message_text;           //< Текс отправленного сообщения
};

/// @brief Структура содержит результат этапа подписания документа
struct DocumentSigningResult {
    std::string first_name;             //< Имя пользователя
    std::string middle_name;            //< Отчество пользователя
    std::string last_name;              //< Фамилия пользователя
    std::string phone_number;           //< Номер телефона, использованный для подтверждения
    std::string confirmation_code;      //< Код подтверждения, введённый пользователем
    std::string signing_time;           //< Время подписания документа в формате "DD.MM.YYYY HH:MM:SS"
    std::string document_hash;          //< Криптографический хеш подписанного документа
    std::string signed_pdf_path;        //< Путь к итоговому PDF файлу с подписью
};

/// @brief Структура содержит идентификационные данные пользователя
struct UserIdentity {
    std::string first_name;                      //< Имя пользователя (обязательное поле)
    std::string middle_name;                     //< Отчество пользователя (обязательное поле)
    std::string last_name;                       //< Фамилия пользователя (обязательное поле)
    std::string passport_number;                 //< Номер паспорта (6 цифр)
    std::string passport_series;                 //< Серия паспорта (4 цифры)
    std::string passport_issued_by;              //< Наименование органа, выдавшего паспорт
    std::string passport_issued_date;            //< Дата выдачи в формате "DD.MM.YYYY"
    std::string passport_unite_code;             //< Код подразделения в формате "XXX-XXX"
    std::string passport_birthday_date;          //< Дата рождения
    std::string passport_birthday_place;         //< Место рождения
    std::string passport_registration_address;   //< Адрес регистрации
    std::string passport_registration_date;      //< Дата регистрации
    std::string email;                           //< почта в формате "mail@mail.ru"
    std::string phone_number;                    //< Номер телефона в формате "+7 XXX XXX-XX-XX" (обязательное поле)
};

/// @brief Структура содержит данные электронной подписи документа
struct DocumentSignature {
    std::string confirmation_code;      //< Код подтверждения из SMS (4 цифры)
    std::string document_hash;          //< Хэш документа по ГОСТ Р34.11-2012 (64 hex символа)
    std::string signing_time;           //< Время подписания в формате "YYYY-MM-DD HH:MM:SS"
};

/// @brief Комплексная структура данных для подписания документа
struct SignerData {
    UserIdentity identity;              //< Данные пользователя для идентификации
    DocumentSignature signature;        //< Данные электронной подписи документа
};

/// @brief Коды ошибок операций сервиса
enum class ErrorCode {
    SUCCESS,                            //< Операция выполнена успешно
    INIT_SERVICE_ERROR,                 //< Ошибка инициализации сервиса
    INVALID_USER_DATA,                  //< Отсутствуют обязательные поля в USER DATA
    INVALID_JSON,                       //< Невалидный JSON формат
    INVALID_AUTH_TOKEN,                 //< Невалидный токен авторизации
    FILE_IO_ERROR,                      //< Ошибка чтения/записи файла
    HTML_REPLACE_ERROR,                 //< Ошибка замены плейсхолдеров в HTML шаблоне
    PDF_GENERATION_ERROR,               //< Ошибка генерации PDF из HTML шаблона
    SMS_SEND_ERROR,                     //< Ошибка отправки SMS сообщения
    AUTHENTICATION_FAILED,              //< Неверный код подтверждения или ошибка аутентификации
    STAMP_APPLICATION_ERROR,            //< Ошибка наложения штампа на документ
    INVALID_CONFIG,                     //< Невалидная конфигурация сервиса
    CREDENTIALS_ERROR,                  //< Невалидная конфигурация сервиса MessageService
    SERVICE_SHUTDOWN,                   //< Сервис остановлен и не принимает новые задачи
    UNKNOWN_ERROR                       //< Неизвестная ошибка
};

/// @brief Type-safe контейнер для операций, которые могут завершиться с ошибкой
template<typename T = void>
class Result {
public:
    /// @brief Создает успешный результат без значения (для void специализации)
    static Result success() {
        return Result();
    }
    
    /// @brief Создает успешный результат со значением
    static Result success(T value) {
        return Result(std::move(value));
    }
    
    /// @brief Создает результат с ошибкой
    static Result error(ErrorCode code, std::string message = "") {
        return Result(code, std::move(message));
    }
    
    /// @brief Проверяет успешность операции
    bool isSuccess() const { 
        return error_code_ == ErrorCode::SUCCESS; 
    }
    
    /// @brief Проверяет наличие ошибки
    bool isError() const { 
        return !isSuccess(); 
    }
    
    /// @brief Получает хранимое значение (бросает исключение если результат содержит ошибку)
    const T& getValue() const {
        if (!isSuccess()) {
            throw std::runtime_error("Попытка получить значение из ошибочного результата: " + error_message_);
        }
        return value_;
    }
    
    /// @brief Получает код ошибки
    ErrorCode getErrorCode() const { 
        return error_code_; 
    }
    
    /// @brief Получает сообщение об ошибке
    const std::string& getErrorMessage() const { 
        return error_message_; 
    }

    // Упрощенная версия без сложного управления памятью
    ~Result() = default;
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

private:
    // Приватные конструкторы
    Result() : error_code_(ErrorCode::SUCCESS) {}
    
    Result(T value) 
        : error_code_(ErrorCode::SUCCESS)
        , value_(std::move(value)) {}
    
    Result(ErrorCode code, std::string message)
        : error_code_(code)
        , error_message_(std::move(message)) {}
    
    ErrorCode error_code_{ErrorCode::SUCCESS};
    std::string error_message_;
    T value_{}; // Простое хранение значения
};

// Специализация для void
template<>
class Result<void> {
public:
    static Result success() { return Result(); }
    static Result error(ErrorCode code, std::string message = "") {
        return Result(code, std::move(message));
    }
    
    bool isSuccess() const { return error_code_ == ErrorCode::SUCCESS; }
    bool isError() const { return !isSuccess(); }
    ErrorCode getErrorCode() const { return error_code_; }
    const std::string& getErrorMessage() const { return error_message_; }

private:
    Result() : error_code_(ErrorCode::SUCCESS) {}
    Result(ErrorCode code, std::string message)
        : error_code_(code), error_message_(std::move(message)) {}
    
    ErrorCode error_code_{ErrorCode::SUCCESS};
    std::string error_message_;
};

#endif // COMMON_TYPES_H