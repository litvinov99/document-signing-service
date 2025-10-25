#ifndef WKHTML2PDF_WRAPPER_H
#define WKHTML2PDF_WRAPPER_H

#include <string>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

/**
 * @brief Singleton-обертка для потокобезопасной работы с wkhtmltopdf
 * 
 * Обеспечивает использование wkhtmltopdf только в одном выделенном потоке,
 * так как библиотека не поддерживает многопоточность даже с синхронизацией.
 */
class WkHtmlToPdfWrapper {
public:
    using Callback = std::function<void(bool, const std::string&)>;

    /// @brief Удаляем конструкторы копирования и присваивания
    WkHtmlToPdfWrapper(const WkHtmlToPdfWrapper&) = delete;
    WkHtmlToPdfWrapper& operator=(const WkHtmlToPdfWrapper&) = delete;

    /// @brief Получение единственного экземпляра
    static WkHtmlToPdfWrapper& getInstance();

    /// @brief Инициализация wrapper'а (должна быть вызвана из главного потока)
    bool initialize();

    /// @brief Остановка wrapper'а (должна быть вызвана из главного потока)
    void shutdown(bool wait_for_completion = true);

    /**
     * @brief Асинхронная конвертация HTML в PDF
     * @param input_html_path Путь к входному HTML файлу
     * @param output_pdf_path Путь для сохранения PDF файла
     * @param callback Функция обратного вызова
     * @return true если задача добавлена в очередь, false при ошибке
     */
    bool convertAsync(const std::string& input_html_path,
                      const std::string& output_pdf_path,
                      Callback callback = nullptr);

    /**
     * @brief Синхронная конвертация HTML в PDF
     * @param input_html_path Путь к входному HTML файлу
     * @param output_pdf_path Путь для сохранения PDF файла
     * @return true если конвертация успешна, false в случае ошибки
     */
    bool convertSync(const std::string& input_html_path,
                     const std::string& output_pdf_path);

    /// @brief Проверка инициализации wrapper'а
    bool isInitialized() const { return initialized_; }

    /// @brief Получение статистики работы в формате JSON
    std::string getStats() const;

private:
    /// @brief Задача конвертации
    struct ConversionTask {
        std::string input_path;           //< Путь к входному HTML файлу
        std::string output_path;          //< Путь для сохранения PDF файла
        Callback callback;                //< Функция обратного вызова
        std::promise<bool> promise;       //< Promise для синхронных вызовов
    };

    WkHtmlToPdfWrapper();
    ~WkHtmlToPdfWrapper();

    /// @brief Функция рабочего потока
    void workerThreadFunction();

    /// @brief Выполнение конвертации
    bool performConversion(const std::string& input_path, 
                          const std::string& output_path,
                          std::string& error_message);

    std::thread worker_thread_;                                   //< Рабочий поток для конвертации
    std::queue<std::shared_ptr<ConversionTask>> task_queue_;      //< Очередь задач на конвертацию
    mutable std::mutex queue_mutex_;                              //< Мьютекс для синхронизации доступа к очереди
    std::condition_variable queue_cv_;                            //< Условная переменная для уведомления рабочего потока

    std::atomic<bool> running_{false};                            //< Флаг работы wrapper'а
    std::atomic<bool> initialized_{false};                        //< Флаг инициализации wrapper'а

    mutable std::mutex stats_mutex_;                              //< Мьютекс для синхронизации статистики
    size_t total_tasks_{0};                                       //< Общее количество задач
    size_t completed_tasks_{0};                                   //< Количество успешно выполненных задач
    size_t failed_tasks_{0};                                      //< Количество неудачных задач
    std::string last_error_;                                      //< Последняя ошибка
};

#endif // WKHTMLTOPDF_WRAPPER_H