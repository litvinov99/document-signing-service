#ifndef HTML_TEMPLATE_PROCESSOR_H
#define HTML_TEMPLATE_PROCESSOR_H

#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <mutex>

class HtmlTemplateProcessor {
public:
    /**
     * @brief Заменяет плейсхолдеры в HTML файле значениями из JSON файла
     * @param json_file_path Путь к JSON файлу с данными
     * @param html_file_path Путь к HTML файлу с плейсхолдерами
     * @return true если замена прошла успешно
     */
    static bool replacePlaceholdersFromJsonFile(const std::string& json_file_path, 
                                               const std::string& html_file_path);

    /**
     * @brief Заменяет плейсхолдеры в HTML файле значениями из JSON строки
     * @param json_data JSON данные в виде строки
     * @param html_file_path Путь к HTML файлу с плейсхолдерами
     * @return true если замена прошла успешно
     */
    static bool replacePlaceholdersFromJsonData(const std::string& json_data, 
                                               const std::string& html_file_path);

    /**
     * @brief Заменяет плейсхолдеры и сохраняет результат в новый файл
     * @param json_file_path Путь к JSON файлу с данными
     * @param input_html_file_path Путь к исходному HTML файлу
     * @param output_html_file_path Путь для сохранения обработанного HTML
     * @return true если операция прошла успешно
     */
    static bool processTemplateToNewFile(const std::string& json_file_path,
                                       const std::string& input_html_file_path,
                                       const std::string& output_html_file_path);

    /**
     * @brief Заменяет плейсхолдеры в HTML строке значениями из JSON
     * @param json_data JSON данные в виде строки
     * @param html_content HTML содержимое с плейсхолдерами
     * @return Обработанная HTML строка
     */
    static std::string processHtmlString(const std::string& json_data, 
                                       const std::string& html_content);

    /**
     * @brief Заменяет плейсхолдеры в HTML строке значениями из JSON файла
     * @param json_file_path Путь к JSON файлу с данными
     * @param html_content HTML содержимое с плейсхолдерами
     * @return Обработанная HTML строка
     */
    static std::string processHtmlStringFromFile(const std::string& json_file_path,
                                               const std::string& html_content);

    /**
     * @brief Проверяет существование файла
     * @param file_path Путь к файлу
     * @return true если файл существует и доступен для чтения
     */
    static bool fileExists(const std::string& file_path);

    /**
     * @brief Получает расширение файла
     * @param file_path Путь к файлу
     * @return Расширение файла в нижнем регистре
     */
    static std::string getFileExtension(const std::string& file_path);

    /**
     * @brief Очищает кэш шаблонов
     */
    static void clearTemplateCache();

    /**
     * @brief Устанавливает максимальный размер кэша шаблонов
     * @param max_size Максимальное количество шаблонов в кэше
     */
    static void setTemplateCacheSize(size_t max_size);

    /**
     * @brief Заменяет плейсхолдеры с использованием кэшированного шаблона
     * @param json_file_path Путь к JSON файлу с данными
     * @param html_file_path Путь к HTML файлу с плейсхолдерами
     * @param use_cache Использовать кэш шаблонов (по умолчанию true)
     * @return true если замена прошла успешно
     */
    static bool replacePlaceholdersFromJsonFile(const std::string& json_file_path, 
                                               const std::string& html_file_path,
                                               bool use_cache = true);

    /**
     * @brief Заменяет плейсхолдеры и сохраняет результат в новый файл с кэшированием
     * @param json_file_path Путь к JSON файлу с данными
     * @param input_html_file_path Путь к исходному HTML файлу
     * @param output_html_file_path Путь для сохранения обработанного HTML
     * @param use_cache Использовать кэш шаблонов
     * @return true если операция прошла успешно
     */
    static bool processTemplateToNewFile(const std::string& json_file_path,
                                       const std::string& input_html_file_path,
                                       const std::string& output_html_file_path,
                                       bool use_cache = true);

private:
    // Кэш шаблонов: путь -> содержимое
    static std::unordered_map<std::string, std::string> template_cache_;
    static std::mutex cache_mutex_;
    static size_t max_cache_size_;

    /**
     * @brief Получает содержимое файла с использованием кэша
     * @param file_path Путь к файлу
     * @param content Ссылка для сохранения содержимого
     * @param use_cache Использовать кэш
     * @return true если успешно
     */
    static bool getCachedFileContent(const std::string& file_path, 
                                   std::string& content, 
                                   bool use_cache = true);
    /**
     * @brief Читает содержимое файла в строку
     * @param file_path Путь к файлу
     * @param content Ссылка на строку для записи содержимого
     * @return true если чтение прошло успешно
     */
    static bool readFileToString(const std::string& file_path, std::string& content);
    
    /**
     * @brief Записывает строку в файл
     * @param file_path Путь к файлу
     * @param content Содержимое для записи
     * @return true если запись прошла успешно
     */
    static bool writeStringToFile(const std::string& file_path, const std::string& content);

    /**
     * @brief Выполняет замену плейсхолдеров в HTML содержимом
     * @param data JSON объект с данными
     * @param html_content HTML содержимое для обработки
     * @return Обработанное HTML содержимое
     */
    static std::string replacePlaceholdersInContent(const nlohmann::json& data, 
                                                  const std::string& html_content);
};

#endif // HTML_TEMPLATE_PROCESSOR_H

/* ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ
 1. Замена плейсхолдеров в существующем файле
bool success = HtmlTemplateProcessor::replacePlaceholdersFromJsonFile(
    "user_data.json",     // JSON с данными
    "template.html"       // HTML файл для обработки (будет изменен!)
);

2. Создание нового файла с замененными плейсхолдерами
bool success = HtmlTemplateProcessor::processTemplateToNewFile(
    "user_data.json",     // JSON с данными
    "template.html",      // Исходный HTML шаблон
    "output.html"         // Новый файл с результатом
);

 3. Отключение кэширования (если шаблон меняется)
bool success = HtmlTemplateProcessor::processTemplateToNewFile(
    "user_data.json",
    "template.html", 
    "output.html",
    false  // Не использовать кэш
);

 4. Замена из JSON строки
bool success = HtmlTemplateProcessor::replacePlaceholdersFromJsonData(
    json_data,           // JSON данные как строка
    "template.html"      // Файл для обработки
);

5. Обработка HTML строки
std::string html_content = "<html><body>Hello {{FIRST}} {{LAST}}</body></html>";
std::string result = HtmlTemplateProcessor::processHtmlString(
    json_data,          // JSON данные
    html_content        // HTML содержимое
); */
