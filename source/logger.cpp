#include "logger.hpp"
#include "time_utils.hpp"

#include <stdexcept>
#include <iostream>

LoggerService::LoggerService(const std::string& file_path) 
        : file_path_(file_path) {
    // Открываем файл в главном потоке для проверки доступности
    std::ofstream test_file(file_path, std::ios::app);
    if (!test_file.is_open()) {
        throw std::runtime_error("Couldn't open the log 1234file: " + file_path);
    }
    test_file.close();
    
    // Запускаем worker thread (даже если логгер выключен)
    worker_thread_ = std::thread(&LoggerService::workerThread, this);
}

LoggerService::~LoggerService() {
    // Сигнализируем о завершении
    shutdown_.store(true);
    condition_.notify_all();
    
    // Ждем завершения worker thread
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void LoggerService::log(MESSAGE_TYPE message_type, const std::string& message) {
    if (!enabled_) {
        return;
    }

    std::string log_entry = createLogEntry(message_type, message);
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        log_queue_.push(std::move(log_entry));
    }
    
    condition_.notify_one();
}

void LoggerService::logInfo(const std::string& message) {
    log(LoggerService::MESSAGE_TYPE::INFO, message);
}

void LoggerService::logWarning(const std::string& message) {
    log(LoggerService::MESSAGE_TYPE::WARNING, message);
}

void LoggerService::logError(const std::string& message) {
    log(LoggerService::MESSAGE_TYPE::ERROR, message);
}

void LoggerService::logSuccess(const std::string& message) {
    log(LoggerService::MESSAGE_TYPE::SUCCESS, message);
}

void LoggerService::setFilePath(const std::string& new_path) {
    // Проверяем доступность нового файла
    std::ofstream test_file(new_path, std::ios::app);
    if (!test_file.is_open()) {
        throw std::runtime_error("Couldn't open the log file: " + new_path);
    }
    test_file.close();
    
    // Останавливаем worker thread, меняем путь, перезапускаем
    shutdown_ = true;
    condition_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    file_path_ = new_path;
    shutdown_ = false;
    
    worker_thread_ = std::thread(&LoggerService::workerThread, this);
}

std::string LoggerService::getFilePath() const {
    return file_path_;
}

void LoggerService::enable() {
    enabled_.store(true);
}

void LoggerService::disable() {
    enabled_.store(false);
}

bool LoggerService::isEnabled() const {
    return enabled_.load();
}

void LoggerService::workerThread() {
    // Открываем файл в worker thread
    log_file_.open(file_path_, std::ios::app);
    if (!log_file_.is_open()) {
        return;
    }

    while (true) {
        std::queue<std::string> local_queue;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this]() {
                return !log_queue_.empty() || shutdown_.load();
            });
            
            // Забираем всю очередь для минимизации блокировок
            std::swap(local_queue, log_queue_);
        }
        
        // Записываем все сообщения в файл
        while (!local_queue.empty()) {
            log_file_ << local_queue.front() << std::endl;
            local_queue.pop();
        }
        
        // Проверяем завершение
        if (shutdown_.load() && log_queue_.empty()) {
            break;
        }
    }
    
    // Записываем оставшиеся сообщения
    while (!log_queue_.empty()) {
        log_file_ << log_queue_.front() << std::endl;
        log_queue_.pop();
    }
    
    log_file_.flush();
    log_file_.close();
}

std::string LoggerService::messageTypeToString(MESSAGE_TYPE message_type) {
    switch (message_type) {
        case MESSAGE_TYPE::SUCCESS: return "SUCCESS";
        case MESSAGE_TYPE::ERROR: return "ERROR";
        case MESSAGE_TYPE::WARNING: return "WARNING";
        case MESSAGE_TYPE::INFO: return "INFO";
        default: return "UNKNOWN";
    }
}

std::string LoggerService::createLogEntry(MESSAGE_TYPE message_type, const std::string& message) {
    // Получаем ID потока
    std::ostringstream thread_id;
    thread_id << std::this_thread::get_id();
    
    return "[" + time_utils::getCurrentTimeWithTimezone(3, 0) + "] " +
           "[" + messageTypeToString(message_type) + "] " +
           "[Thread:" + thread_id.str() + "] " + 
           message;
}