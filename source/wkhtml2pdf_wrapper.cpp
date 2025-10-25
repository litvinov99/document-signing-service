#include "wkhtml2pdf_wrapper.hpp"
#include "html2pdf_converter.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <future>

using namespace std::chrono_literals;

WkHtmlToPdfWrapper::WkHtmlToPdfWrapper() {
    // Инициализация в конструкторе НЕ выполняется!
    // Она должна быть явно вызвана через initialize() из главного потока
}

WkHtmlToPdfWrapper::~WkHtmlToPdfWrapper() {
    shutdown(false);
}

WkHtmlToPdfWrapper& WkHtmlToPdfWrapper::getInstance() {
    static WkHtmlToPdfWrapper instance;
    return instance;
}

bool WkHtmlToPdfWrapper::initialize() {
    if (initialized_) {
        return true;
    }

    running_ = true;
    worker_thread_ = std::thread(&WkHtmlToPdfWrapper::workerThreadFunction, this);
    // std::cout << "Worker started\n";
    initialized_ = true;

    return true;
}

void WkHtmlToPdfWrapper::shutdown(bool wait_for_completion) {
    if (!initialized_) {
        return;
    }

    running_ = false;
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_cv_.notify_all();
    }

    if (worker_thread_.joinable()) {
        if (wait_for_completion) {
            worker_thread_.join();
        } else {
            worker_thread_.detach();
        }
    }

    initialized_ = false;
}

bool WkHtmlToPdfWrapper::convertAsync(const std::string& input_html_path,
                                    const std::string& output_pdf_path,
                                    Callback callback) {
    if (!initialized_) {
        if (callback) {
            callback(false, "Wrapper not initialized");
        }
        return false;
    }

    auto task = std::make_shared<ConversionTask>();
    task->input_path = input_html_path;
    task->output_path = output_pdf_path;
    task->callback = callback;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(task);
        total_tasks_++;
    }

    queue_cv_.notify_one();
    return true;
}

bool WkHtmlToPdfWrapper::convertSync(const std::string& input_html_path,
                                     const std::string& output_pdf_path) {
    // std::cout << "convertSync was called" << std::endl;
    if (!initialized_) return false;

    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();

    auto task = std::make_shared<ConversionTask>();
    task->input_path = input_html_path;
    task->output_path = output_pdf_path;
    task->promise = std::move(promise);

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(task);
        // std::cout << "Convert task was added to queue" << std::endl;
    }
    // Даем время рабочему потоку начать ожидание
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Будим рабочий поток
    queue_cv_.notify_one();
    // std::cout << "Worker notified" << std::endl;

    // Ждём завершения в том же потоке, где worker-поток будет обрабатывать задачу
    return future.get();  
}

void WkHtmlToPdfWrapper::workerThreadFunction() {
    // 1. Инициализация wkhtmltopdf в worker-потоке
    if (wkhtmltopdf_init(0) != 1) {
        std::cerr << "Failed to initialize wkhtmltopdf" << std::endl;
        return;
    }
    // std::cout << "workerThreadFunction was called" << std::endl;
    running_ = true;

    while (running_) {
        std::shared_ptr<ConversionTask> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]{ return !task_queue_.empty() || !running_; });

            if (!running_ && task_queue_.empty()) break;

            if (task_queue_.empty()) continue;

            task = task_queue_.front();
            task_queue_.pop();
            // std::cout << "task was export from queue" << std::endl;
        }

        std::string error_message;
        bool success = HtmlToPdfConverter::convertFile(task->input_path, task->output_path);
        // std::cout << "file was converted!" << std::endl;

        // Установка promise
        try {
            task->promise.set_value(success);
        } catch (const std::future_error& e) {
            // например, promise уже установлен
            std::cerr << "Promise error: " << e.what() << std::endl;
        }

        if (task->callback) {
            task->callback(success, error_message);
        }
    }

    wkhtmltopdf_deinit();
}

bool WkHtmlToPdfWrapper::performConversion(const std::string& input_path, 
                                         const std::string& output_path,
                                         std::string& error_message) {
    try {
        static std::mutex conversion_mutex;
        std::lock_guard<std::mutex> lock(conversion_mutex);

        bool success = HtmlToPdfConverter::convertFile(input_path, output_path);
        if (!success) {
            error_message = "Conversion failed";
        }
        return success;
    } catch (const std::exception& e) {
        error_message = e.what();
        return false;
    }
}

std::string WkHtmlToPdfWrapper::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    nlohmann::json stats;
    stats["initialized"] = initialized_.load();
    stats["running"] = running_.load();
    stats["total_tasks"] = total_tasks_;
    stats["completed_tasks"] = completed_tasks_;
    stats["failed_tasks"] = failed_tasks_;
    stats["queue_size"] = task_queue_.size();
    stats["last_error"] = last_error_;
    
    return stats.dump(2);
}