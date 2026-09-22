cmake_minimum_required(VERSION 3.24)

project(Proxima
    VERSION 0.1.0
    DESCRIPTION "Proxima Fortnite network diagnostics client"
    LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)

option(PROXIMA_BUILD_DESKTOP "Build the Qt 6 desktop application" ON)
option(PROXIMA_BUILD_TESTS "Build unit and integration tests" ON)
option(PROXIMA_ENABLE_CLANG_TIDY "Run clang-tidy during compilation when available" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(PROXIMA_CORE_SOURCES
    src/core/domain.cpp
    src/diagnostics/statistics.cpp
    src/diagnostics/ping_probe.cpp
    src/diagnostics/network_measurement_engine.cpp
    src/diagnostics/traceroute_probe.cpp
    src/diagnostics/dns_probe.cpp
    src/routing/route_scoring_engine.cpp
    src/routing/route_manager.cpp
    src/transport/direct_transport.cpp
    src/transport/relay_transport_stub.cpp
    src/discovery/fortnite_process_detector.cpp
    src/discovery/endpoint_discovery.cpp
    src/backend/config.cpp
    src/backend/config_cache.cpp
    src/backend/backend_client.cpp
    src/logging/logger.cpp
    src/logging/diagnostic_exporter.cpp
    src/update/update_checker.cpp
    src/platform/windows/windows_process_provider.cpp
    src/platform/windows/windows_network_provider.cpp
    src/platform/windows/windows_system_info.cpp
)

add_library(proxima_core STATIC ${PROXIMA_CORE_SOURCES})
target_include_directories(proxima_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(proxima_core PUBLIC PROXIMA_VERSION="${PROJECT_VERSION}")

if(WIN32)
    target_link_libraries(proxima_core PUBLIC iphlpapi ws2_32 dnsapi)
endif()

if(PROXIMA_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
    if(CLANG_TIDY_EXECUTABLE)
        set_target_properties(proxima_core PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
    endif()
endif()

if(PROXIMA_BUILD_DESKTOP)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets Network Concurrent)

    qt_add_executable(Proxima
        apps/desktop/main.cpp
        apps/desktop/main_window.cpp
        apps/desktop/main_window.hpp
    )
    target_link_libraries(Proxima PRIVATE
        proxima_core
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Concurrent
    )
    target_compile_definitions(Proxima PRIVATE QT_NO_KEYWORDS)
    set_target_properties(Proxima PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "Proxima"
    )

    install(TARGETS Proxima RUNTIME DESTINATION .)
endif()

if(PROXIMA_BUILD_TESTS)
    enable_testing()
    add_executable(proxima_tests tests/unit/test_main.cpp)
    target_link_libraries(proxima_tests PRIVATE proxima_core)
    add_test(NAME proxima_tests COMMAND proxima_tests)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
