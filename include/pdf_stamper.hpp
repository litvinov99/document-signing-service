#ifndef PDF_STAMPER_H
#define PDF_STAMPER_H

#include "common_types.hpp"
#include <string>
#include <podofo/podofo.h>

class PdfStamper {
public:
    // static SignerData loadSignerDataFromJsonFile(const std::string& json_file_path);
    // static SignerData loadSignerDataFromJsonData(const std::string& json_data);

    /**
     * @brief Применяет штамп ко всем страницам PDF документа
     * @param inputFile Путь к исходному PDF файлу
     * @param outputFile Путь для сохранения подписанного PDF
     * @param signerData Данные подписанта для штампа
     * @return true если операция успешна, false в случае ошибки
     */
    static bool applyStamp(const std::string& input_file, 
                           const std::string& output_file,
                           const SignerData& signer_data);

private:

    /**
     * @brief Применяет штамп с данными подписанта ко всем страницам PDF документа
     * @param document Ссылка на объект PDF документа (должен быть уже загружен)
     * @param data Данные подписанта для отображения в штампе
     * @note Функция модифицирует переданный документ, добавляя штамп на каждую страницу
     * @warning Документ должен быть валидным и содержать хотя бы одну страницу
     * @throws PdfError в случае ошибок работы с PDF (рисование, шрифты и т.д.)
     * @throws std::runtime_error в случае ошибок обработки данных
     */
    static void createStamp(PoDoFo::PdfMemDocument& document, const SignerData& data);
};

#endif // PDF_STAMPER_H