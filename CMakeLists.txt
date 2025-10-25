cmake_minimum_required(VERSION 3.12)
project(DocumentSigner VERSION 1.0.0 LANGUAGES CXX)

# Параллельная сборка с оптимизацией
include(ProcessorCount)
ProcessorCount(NUM_PROCESSORS)
if(NUM_PROCESSORS EQUAL 0)
    set(NUM_PROCESSORS 4)
    message(STATUS "Using default 4 build threads")
else()
    # Оптимально: оставляем 2 ядра для системы
    math(EXPR NUM_PROCESSORS "${NUM_PROCESSORS} - 2")
    if(NUM_PROCESSORS LESS 2)
        set(NUM_PROCESSORS 2)  # минимум 2 потока
    endif()
    message(STATUS "Using ${NUM_PROCESSORS} build threads (optimal)")
endif()

set(CMAKE_BUILD_PARALLEL_LEVEL ${NUM_PROCESSORS} CACHE STRING "Number of parallel jobs")

# Кеширование при сборке для ускорения дальнейшей пересборки проекта
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else()
    message(STATUS "ccache not found, compilation will be slower")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Настройка сборок Debug/Release
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type: Debug or Release")

# Установка выходных директорий
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# Для многоконфигурационных сборок (Debug/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# Настройка путей
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/source)

# Включаем ExternalProject
include(ExternalProject)

# Настройка путей для установки зависимостей
set(DEPS_INSTALL_DIR ${CMAKE_BINARY_DIR}/deps_install)

# ZLIB
ExternalProject_Add(zlib_external
    SOURCE_DIR ${THIRD_PARTY_DIR}/zlib-1.2.11
    CONFIGURE_COMMAND <SOURCE_DIR>/configure 
        --prefix=${DEPS_INSTALL_DIR}
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE TRUE
)

# OpenSSL
ExternalProject_Add(openssl_external
    SOURCE_DIR ${THIRD_PARTY_DIR}/openssl-3.0.2
    CONFIGURE_COMMAND <SOURCE_DIR>/config 
        --prefix=${DEPS_INSTALL_DIR} no-shared
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install_sw
    BUILD_IN_SOURCE TRUE
    DEPENDS zlib_external
)

# libgpg-error
ExternalProject_Add(libgpg_error_external
    SOURCE_DIR ${THIRD_PARTY_DIR}/libgpg-error-1.43
    CONFIGURE_COMMAND <SOURCE_DIR>/configure 
        --prefix=${DEPS_INSTALL_DIR} 
        --enable-static 
        --disable-shared
        --disable-doc
        --disable-languages
        --disable-nls
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE TRUE
    DEPENDS openssl_external
)

# libgcrypt
ExternalProject_Add(libgcrypt_external
    SOURCE_DIR ${THIRD_PARTY_DIR}/libgcrypt-1.9.4
    CONFIGURE_COMMAND <SOURCE_DIR>/configure 
        --prefix=${DEPS_INSTALL_DIR} 
        --enable-static 
        --disable-shared 
        --disable-doc
        --with-libgpg-error-prefix=${DEPS_INSTALL_DIR} 
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE TRUE
    DEPENDS libgpg_error_external openssl_external 
)

# curl
ExternalProject_Add(curl_external
    SOURCE_DIR ${THIRD_PARTY_DIR}/curl-7.81.0
    CONFIGURE_COMMAND <SOURCE_DIR>/configure 
        --prefix=${DEPS_INSTALL_DIR} 
        --with-ssl=${DEPS_INSTALL_DIR} 
        --disable-shared 
        --enable-static
        --without-brotli
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE TRUE
    DEPENDS openssl_external zlib_external
)

# Импортированные библиотеки
add_library(zlib STATIC IMPORTED)
add_dependencies(zlib zlib_external)
set_target_properties(zlib PROPERTIES
    IMPORTED_LOCATION ${DEPS_INSTALL_DIR}/lib/libz.a
)

# OpenSSL
add_library(ssl STATIC IMPORTED)
set_target_properties(ssl PROPERTIES
    IMPORTED_LOCATION ${DEPS_INSTALL_DIR}/lib64/libssl.a  # ИЛИ lib в зависимости от системы
)
add_dependencies(ssl openssl_external)

add_library(crypto STATIC IMPORTED)
set_target_properties(crypto PROPERTIES
    IMPORTED_LOCATION ${DEPS_INSTALL_DIR}/lib64/libcrypto.a  # ИЛИ lib в зависимости от системы
)
add_dependencies(crypto openssl_external)

add_library(gpg-error STATIC IMPORTED)
add_dependencies(gpg-error libgpg_error_external)
set_target_properties(gpg-error PROPERTIES
    IMPORTED_LOCATION ${DEPS_INSTALL_DIR}/lib/libgpg-error.a
)

add_library(libgcrypt STATIC IMPORTED)
add_dependencies(libgcrypt libgcrypt_external)
set_target_properties(libgcrypt PROPERTIES
    IMPORTED_LOCATION ${DEPS_INSTALL_DIR}/lib/libgcrypt.a
)

add_library(curl STATIC IMPORTED)
add_dependencies(curl curl_external)
set_target_properties(curl PROPERTIES
    IMPORTED_LOCATION ${DEPS_INSTALL_DIR}/lib/libcurl.a
)

# Header-only библиотеки (остаются как есть)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)   # отключаем json тесты
add_subdirectory(${THIRD_PARTY_DIR}/json-3.9.1)

# PoDoFo: отключаем тесты и примеры
set(PODOFO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PODOFO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${THIRD_PARTY_DIR}/podofo)

if(CMAKE_BUILD_PARALLEL_LEVEL GREATER 4)
    set_property(DIRECTORY ${THIRD_PARTY_DIR}/podofo 
                PROPERTY EXCLUDE_FROM_ALL TRUE)
    set(CMAKE_BUILD_PARALLEL_LEVEL 4 CACHE STRING "Reduce for PoDoFo" FORCE)
endif()

# add_subdirectory(${THIRD_PARTY_DIR}/wkhtmltox)
add_library(wkhtmltox SHARED IMPORTED)
set_target_properties(wkhtmltox PROPERTIES
    IMPORTED_LOCATION ${THIRD_PARTY_DIR}/wkhtmltox/lib/libwkhtmltox.so
    INTERFACE_INCLUDE_DIRECTORIES ${THIRD_PARTY_DIR}/wkhtmltox/include
)

# Исходные файлы
set(SOURCES
    ${SOURCE_DIR}/document_hasher.cpp
    ${SOURCE_DIR}/html_template_processor.cpp
    ${SOURCE_DIR}/html2pdf_converter.cpp
    # ${SOURCE_DIR}/html2pdf_converter_pool.cpp
    ${SOURCE_DIR}/wkhtml2pdf_wrapper.cpp
    ${SOURCE_DIR}/message_service.cpp
    ${SOURCE_DIR}/pdf_stamper.cpp
    ${SOURCE_DIR}/signature_service.cpp
    ${SOURCE_DIR}/time_utils.cpp
    ${SOURCE_DIR}/file_utils.cpp
    ${SOURCE_DIR}/parsers.cpp
    ${SOURCE_DIR}/logger.cpp
    main.cpp
)

# Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# Предкомпилированные заголовки
target_precompile_headers(${PROJECT_NAME} PRIVATE
    <vector>
    <string>
    <memory>
    <iostream>
    <functional>
)

# Настройка целевого исполняемого файла
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${INCLUDE_DIR}
    ${DEPS_INSTALL_DIR}/include
    ${THIRD_PARTY_DIR}/podofo/include
    # ${THIRD_PARTY_DIR}/wkhtmltox/include
)

# Линковка библиотек
target_link_libraries(${PROJECT_NAME} PRIVATE
    podofo
    curl
    ssl
    crypto
    libgcrypt
    gpg-error
    nlohmann_json::nlohmann_json
    wkhtmltox
    zlib
    # std::filesystem
)

# Ждем сборку зависимостей
add_dependencies(${PROJECT_NAME} openssl_external zlib_external libgpg_error_external libgcrypt_external curl_external)

# Оптимизация линковки
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(${PROJECT_NAME} PRIVATE
        -Wl,--no-as-needed
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(${PROJECT_NAME} PRIVATE
            -Wl,--strip-all
            -Wl,--gc-sections
        )
    endif()
endif()