#include "pdf_stamper.hpp"

#include <podofo/podofo.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace PoDoFo;
using json = nlohmann::json;

void PdfStamper::createStamp(PdfMemDocument& document, const SignerData& data) {
    // Подготовка шрифтов
    PdfFont* font = document.GetFonts().SearchFont("Arial");
    PdfFontSearchParams search_params;
    search_params.Style = PdfFontStyle::Bold;
    PdfFont* bold_font = document.GetFonts().SearchFont("Arial", search_params);
    if (!font) {
        font = &document.GetFonts().GetStandard14Font(PdfStandard14FontType::Helvetica);
    }
    if (!bold_font) {
        bold_font = &document.GetFonts().GetStandard14Font(PdfStandard14FontType::HelveticaBold);
    }

    // Подготовка текста один раз
    std::string full_name = data.identity.first_name + " " + 
                                data.identity.middle_name + " " + 
                                    data.identity.last_name;
	                      
    std::string passport_info;
    if (!data.identity.passport_series.empty() && !data.identity.passport_number.empty()) {
        passport_info = "Паспортные данные: серия " + data.identity.passport_series + 
                               " номер " + data.identity.passport_number;
    }
    
    std::string issued_info;
    if (!data.identity.passport_issued_by.empty()) {
        issued_info = "Выдан: " + data.identity.passport_issued_by;
    }
    
    std::string date_code_info;
    if (!data.identity.passport_issued_date.empty() && !data.identity.passport_unite_code.empty()) {
        date_code_info = data.identity.passport_issued_date + ", " + 
                        "код подразделения " + data.identity.passport_unite_code;
    }

    std::string phone_number = "Номер телефона: " + data.identity.phone_number;

    std::string email;
    if (!data.identity.email.empty()) {
        email = "Эл. почта: " + data.identity.email;
    }
    
    std::string sign_date = "Дата и время подписания по МСК: " + data.signature.signing_time;
    
    std::string sms_text = "CMC-код " + data.signature.confirmation_code + 
                                " и хэш-код по документу (ГОСТ Р34.11-2012)";
	
    size_t page_сount = document.GetPages().GetCount();
    for (size_t page_index = 0; page_index < page_сount; ++page_index) {
        auto& page = document.GetPages().GetPageAt(page_index);
        PdfPainter painter;
        painter.SetCanvas(page);

        // Параметры штампа (начало координат левый нижний угол)
        const double stamp_x = 30;
        const double stamp_y = 15;

        const double stamp_width = 254;
        // const double stamp_height = 85;
        const double corner_radius = 5;
        const double line_height = 10;

        // Подсчитываем количество непустых строк
        size_t line_count = 0;
        line_count++; // "Подписано простой электронной подписью" - всегда есть
        line_count++; // full_name - всегда есть
        if (!passport_info.empty()) line_count++;
        if (!issued_info.empty()) line_count++;
        if (!date_code_info.empty()) line_count++;
        if (!email.empty()) line_count++;
        line_count++; // phone_number - всегда есть
        line_count++; // sign_date и sms_text - всегда есть
        line_count++; // document_hash - всегда есть

        // Вычисляем динамическую высоту рамки
        const double padding_top = 10;
        const double padding_bottom = 5;
        const double stamp_height = padding_top + (line_count * line_height) + padding_bottom;

        // Настраиваем позицию текста
        double text_x = stamp_x + 5;
        double text_y = stamp_y + stamp_height - 10;

        // Рисуем синюю рамку (обязательно первой)
        painter.GraphicsState.SetStrokingColor(PdfColor(0.0, 0.0, 1.0));
        painter.GraphicsState.SetLineWidth(1.5);
        painter.DrawRectangle(stamp_x, stamp_y, stamp_width, stamp_height, 
                              PdfPathDrawMode::Stroke, corner_radius, corner_radius);

        // Первая строка - жирная
        painter.TextState.SetFont(*bold_font, 8);
        painter.DrawText("Подписано простой электронной подписью", text_x, text_y);
        text_y -= line_height;

        // Обязательное поле - фио
        painter.TextState.SetFont(*font, 8);
        painter.DrawText(full_name, text_x, text_y);
        text_y -= line_height;

        // Необязательные поля, которые могут быть пустыми - паспортные данные
        if (!passport_info.empty()) {
            painter.DrawText(passport_info, text_x, text_y);
            text_y -= line_height;
        }
        if (!issued_info.empty()) {
            painter.DrawText(issued_info, text_x, text_y);
            text_y -= line_height;
        }
        if (!date_code_info.empty()) {
            painter.DrawText(date_code_info, text_x, text_y);
            text_y -= line_height;
        }

        // Обазательное поле - номер телефона
        painter.DrawText(phone_number, text_x, text_y);
        text_y -= line_height;

        // Необязательное поле - почта
        if (!email.empty()) {
            painter.DrawText(email, text_x, text_y);
            text_y -= line_height;
        }

        // Обязательные поля - дата подписания, смс-код, хэш
        painter.DrawText(sign_date, text_x, text_y);
        text_y -= line_height;
        painter.DrawText(sms_text, text_x, text_y);
        text_y -= line_height;

        painter.TextState.SetFont(*font, 7);
        painter.DrawText(data.signature.document_hash, text_x, text_y);
        painter.FinishDrawing();
    }
}

// SignerData PdfStamper::loadSignerData(const std::string& jsonFilePath) {
//     SignerData data;
//     try {
//         std::ifstream json_file(jsonFilePath);
//         json json_data = json::parse(json_file);

//         data.name = json_data["signer_name"];
//         data.passport_series = json_data["passport_data"]["series"];
//         data.passport_number = json_data["passport_data"]["number"];
//         data.passport_issued_by = json_data["passport_issued_by"];
//         data.passport_issue_date = json_data["passport_issue_date"];
//         data.confirmation_code = json_data["confirmation_code"];
//         data.document_hash = json_data["document_hash"];

//     } catch (const std::exception& e) {
//     }

//     return data;
// }

bool PdfStamper::applyStamp(const std::string& input_file, 
                            const std::string& output_file,
                            const SignerData& signer_data) {
    try {
        PdfMemDocument document;
        document.Load(input_file);
        if (document.GetPages().GetCount() == 0) {
            return false;
        }
        // Применение штампа ко всем страницам
        createStamp(document, signer_data);
        document.Save(output_file);
        return true;
    } catch (const PoDoFo::PdfError& e) {
        // Логирование ошибки PoDoFo (опционально)
        std::cerr << "PoDoFo error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        // Логирование стандартных исключений (опционально)
        std::cerr << "Standard exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        // Логирование неизвестных ошибок (опционально)
        std::cerr << "Unknown error during PDF processing" << std::endl;
        return false;
    }
}
