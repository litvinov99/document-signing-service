#ifndef SIGNATURE_HASHER_H
#define SIGNATURE_HASHER_H

#include <string>
#include <vector>

class DocumentHasher {
public:
    // не используется
    // enum class HashAlgorithm {
    //     // STRIBOG_256,
    //     // STRIBOG_512,
    //     // SHA_256,
    //     // SHA_512
    // };

    /**
     * @brief Конструктор инициализирует библиотеку криптографии
     * @throws std::runtime_error если инициализация не удалась
     */
    DocumentHasher();

    /**
     * @brief Деструктор освобождает ресурсы libgcrypt
     */
    ~DocumentHasher();

    DocumentHasher(const DocumentHasher&) = delete;
    DocumentHasher& operator=(const DocumentHasher&) = delete;
    
    /**
     * @brief Вычисляет составной хеш документа с метаданными
     * @param file_path Путь к файлу для хеширования
     * @param user_name ФИО пользователя
     * @param user_phone Номер телефона пользователя
     * @param confirmation_code Код подтверждения из SMS
     * @param signing_time Время подписания в формате строки в ISO 8601 format (UTC)
     * @return Бинарный хеш в виде вектора байт
     * @throws std::runtime_error при ошибках чтения файла или вычисления хеша
     */
    std::vector<unsigned char> calculateCompositeHash(
        const std::string& file_path,
        const std::string& user_name,
        const std::string& user_phone,
        const std::string& confirmation_code,
        const std::string& signing_time);
    
    /**
     * @brief Конвертирует бинарный хеш в hex-строку
     * @param hash Бинарный хеш
     * @return Строка с hex-представлением хеша
     */
    static std::string hashToHex(const std::vector<unsigned char>& hash);

     /**
     * @brief Проверяет соответствие составного хеша
     * @param file_path Путь к файлу для проверки
     * @param user_name ФИО пользователя
     * @param user_phone Номер телефона пользователя
     * @param verification_code Код подтверждения для проверки
     * @param signing_time Время подписания
     * @param expected_hash_hex Ожидаемый хеш в hex-формате
     * @return true если хеши совпадают, false если отличаются или ошибка
     */
    bool verifyCompositeHash(
        const std::string& file_path,
        const std::string& user_name,
        const std::string& user_phone,
        const std::string& confirmation_code,
        const std::string& signing_time,
        const std::string& expected_hash_hex);

private:
    static const size_t HASH_BUFFER_SIZE = 16384; // 16KB
    static const size_t STRIBOG_HASH_LENGTH = 32;
    
    /**
     * @brief Инициализирует библиотеку libgcrypt
     * @throws std::runtime_error если инициализация не удалась
     */
    void initializeLibgcrypt();
    bool is_initialized_;
        
    /**
     * @brief Преобразует строку к нижнему регистру
     * @param str Исходная строка
     * @return Строка в нижнем регистре
     */
    std::string toLowercase(const std::string& str) const;

     /**
     * @brief Сравнивает два хеша с постоянным временем выполнения
     * @param hash1 Первый хеш в hex-формате
     * @param hash2 Второй хеш в hex-формате
     * @return true если хеши идентичны, false в противном случае
     */
    bool compareHashes(const std::string& hash1, const std::string& hash2) const;
};

#endif // SIGNATURE_HASHER_H


/* ПРИМЕР ИСПОЛЬЗОВАНИЯ
SignatureHasher hasher;

try {
    // Вычисление хеша
    auto hash = hasher.calculateCompositeHash(
        "document.pdf",
        "Иванов Иван Иванович",
        "+79161234567",
        "1234",
        "2024-01-15 14:30:00"
    );
    
    // Конвертация в hex
    std::string hexHash = SignatureHasher::hashToHex(hash);
    
    // Проверка хеша
    bool isValid = hasher.verifyCompositeHash(
        "document.pdf",
        "Иванов Иван Иванович", 
        "+79161234567",
        "1234",
        "2024-01-15 14:30:00",
        hexHash
    );
    
} catch (const std::exception& e) {
    std::cerr << "Hash error: " << e.what() << std::endl;
} */
