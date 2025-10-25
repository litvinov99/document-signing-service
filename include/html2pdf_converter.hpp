#ifndef HTML2PDF_CONVERTER_H
#define HTML2PDF_CONVERTER_H

#include <string>
#include <wkhtmltox/pdf.h>

class HtmlToPdfConverter {
public:
    /**
     * @brief Настройки конвертации HTML в PDF
     * 
     * Структура содержит все параметры для настройки процесса конвертации,
     * включая размер страницы, ориентацию, поля, качество и другие опции.
     * Значения по умолчанию оптимизированы для большинства случаев использования.
     */
    struct Options {
        std::string page_size;              ///< Размер страницы (A4, Letter, Legal, etc.)
        std::string orientation;            ///< Ориентация (Portrait, Landscape)
        int dpi;                            ///< Разрешение DPI для изображений
        int margin_top;                     ///< Верхнее поле в мм
        int margin_bottom;                  ///< Нижнее поле в мм
        int margin_left;                    ///< Левое поле в мм
        int margin_right;                   ///< Правое поле в мм
        double zoom;                        ///< Коэффициент масштабирования
        int minimum_font_size;              ///< Минимальный размер шрифта
        bool disable_smart_shrinking;       ///< Отключить умное сжатие
        bool enable_local_file_access;      ///< Разрешить доступ к локальным файлам
        bool grayscale;                     ///< Чёрно-белый режим
        bool lowquality;                    ///< Режим низкого качества

        /**
         * @brief Конструктор с значениями по умолчанию
         * 
         * Инициализирует настройки значениями, подходящими для большинства
         * документов: A4, портретная ориентация, стандартные поля.
         */
        Options() : 
            page_size("A4"),
            orientation("Portrait"),
            dpi(96),
            margin_top(10),
            margin_bottom(45),
            margin_left(5),    
            margin_right(5),   
            zoom(1.2),
            minimum_font_size(11),
            disable_smart_shrinking(true),
            enable_local_file_access(true),
            grayscale(false),
            lowquality(false) {}
    };

    /**
     * @brief Конвертирует HTML файл в PDF
     * @param input_html_path Путь к входному HTML файлу
     * @param output_pdf_path Путь для сохранения выходного PDF файла
     * @param opts Настройки конвертации (опционально)
     * @return true если конвертация прошла успешно, false в случае ошибки
     */
    static bool convertFile(const std::string& input_html_path,
                           const std::string& output_pdf_path,
                           const Options& opts = Options());

private:
    /**
     * @brief Устанавливает глобальную настройку для конвертера
     * @param settings Указатель на настройки
     * @param name Имя параметра
     * @param value Значение параметра
     */
    static void setGlobalSetting(wkhtmltopdf_global_settings* settings, 
                                const char* name, const char* value);

    /**
     * @brief Устанавливает настройку объекта (страницы) для конвертера
     * @param settings Указатель на настройки объекта
     * @param name Имя параметра
     * @param value Значение параметра
     */
    static void setObjectSetting(wkhtmltopdf_object_settings* settings, 
                                const char* name, const char* value);
};

#endif // HTML2PDF_CONVERTER_H