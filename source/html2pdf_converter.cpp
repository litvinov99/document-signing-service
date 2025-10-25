#include "html2pdf_converter.hpp"
#include <string>
#include <memory>

bool HtmlToPdfConverter::convertFile(const std::string& input_html_path,
                                     const std::string& output_pdf_path,
                                     const Options& opts) {
    // Инициализация wkhtmltopdf
    // if (wkhtmltopdf_init(0) != 1) {
        // return false;
    // }

    // Создаем глобальные настройки
    wkhtmltopdf_global_settings* global_settings = wkhtmltopdf_create_global_settings();
    // if (!global_settings) {
        // wkhtmltopdf_deinit();
        // return false;
    // }

    // Устанавливаем глобальные настройки
    setGlobalSetting(global_settings, "out", output_pdf_path.c_str());
    setGlobalSetting(global_settings, "size.pageSize", opts.page_size.c_str());
    setGlobalSetting(global_settings, "orientation", opts.orientation.c_str());
    setGlobalSetting(global_settings, "dpi", std::to_string(opts.dpi).c_str());
    setGlobalSetting(global_settings, "margin.top", (std::to_string(opts.margin_top) + "mm").c_str());
    setGlobalSetting(global_settings, "margin.bottom", (std::to_string(opts.margin_bottom) + "mm").c_str());
    setGlobalSetting(global_settings, "margin.left", (std::to_string(opts.margin_left) + "mm").c_str());
    setGlobalSetting(global_settings, "margin.right", (std::to_string(opts.margin_right) + "mm").c_str());
    
    if (opts.grayscale) {
        setGlobalSetting(global_settings, "colorMode", "Grayscale");
    }
    if (opts.lowquality) {
        setGlobalSetting(global_settings, "quality", "Low");
    }

    // Создаем конвертер
    wkhtmltopdf_converter* converter = wkhtmltopdf_create_converter(global_settings);
    if (!converter) {
        wkhtmltopdf_deinit();
        return false;
    }

    // Создаем настройки объекта (страницы)
    wkhtmltopdf_object_settings* object_settings = wkhtmltopdf_create_object_settings();
    if (!object_settings) {
        wkhtmltopdf_destroy_converter(converter);
        wkhtmltopdf_deinit();
        return false;
    }

    // Устанавливаем настройки объекта
    setObjectSetting(object_settings, "page", input_html_path.c_str());
    setObjectSetting(object_settings, "web.defaultEncoding", "utf-8");
    setObjectSetting(object_settings, "load.zoomFactor", std::to_string(opts.zoom).c_str());
    setObjectSetting(object_settings, "web.minimumFontSize", std::to_string(opts.minimum_font_size).c_str());
    setObjectSetting(object_settings, "load.disableSmartShrinking", opts.disable_smart_shrinking ? "true" : "false");
    setObjectSetting(object_settings, "web.enableLocalFileAccess", opts.enable_local_file_access ? "true" : "false");

    // Добавляем объект в конвертер
    wkhtmltopdf_add_object(converter, object_settings, nullptr);

    // Выполняем конвертацию
    int conversion_result = wkhtmltopdf_convert(converter);
    
    // if (conversion_result != 1) {
        // wkhtmltopdf_destroy_converter(converter);
        // wkhtmltopdf_deinit();
        // return false;
    // }
    
    // Очищаем ресурсы
    wkhtmltopdf_destroy_converter(converter);
    // wkhtmltopdf_deinit();

    return true;
}

void HtmlToPdfConverter::setGlobalSetting(wkhtmltopdf_global_settings* settings, 
                                        const char* name, const char* value) {
    wkhtmltopdf_set_global_setting(settings, name, value);
}

void HtmlToPdfConverter::setObjectSetting(wkhtmltopdf_object_settings* settings, 
                                        const char* name, const char* value) {
    wkhtmltopdf_set_object_setting(settings, name, value);
}