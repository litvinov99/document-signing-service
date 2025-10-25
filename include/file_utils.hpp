#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>

namespace file_utils {

/**
 * @brief Создает временную копию файла
 * @param original_path Путь к оригинальному файлу
 * @param prefix Префикс для имени временного файла
 * @param suffix Суффикс для имени временного файла
 * @param temp_dir Директория для временных файлов
 * @return Путь к созданной временной копии
 * @throws std::runtime_error если не удалось создать копию
 */
std::string createTempCopyWithUniqueFilename(const std::string& original_path,
                          const std::string& prefix,
                          const std::string& suffix,
                          const std::string& temp_dir);

/// @brief Генерирует уникальное имя файла
std::string generateUniqueFilename(const std::string& prefix,
                                  const std::string& suffix);

/// @brief Удаляет временные файлы
void cleanupTempFiles(const std::vector<std::string>& files);

/// @brief Проверяет существование файла
bool fileExists(const std::string& path);

/// @brief Создает директорию если она не существует
bool ensureDirectoryExists(const std::string& path);

} // namespace file_utils

#endif