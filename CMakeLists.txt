cmake_minimum_required(VERSION 3.10)
project(FlightControlsLauncher)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建类型检查
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# 查找Qt5核心组件
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)

# Qt版本检查
if(Qt5_VERSION VERSION_LESS "5.12.0")
    message(FATAL_ERROR "需要Qt 5.12或更高版本，当前版本: ${Qt5_VERSION}")
endif()

# 设置Qt MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# 定义源文件
set(LAUNCHER_SOURCES
    src/main.cpp
    src/FlightControlsLauncher.cpp
)

set(LAUNCHER_HEADERS
    src/FlightControlsLauncher.h
)

# 浮动启动器可执行文件
add_executable(flight_controls_launcher
    ${LAUNCHER_SOURCES}
    ${LAUNCHER_HEADERS}
)

# 链接Qt5库
target_link_libraries(flight_controls_launcher
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

# 编译选项
if(MSVC)
    target_compile_options(flight_controls_launcher PRIVATE /W4 /WX)
else()
    target_compile_options(flight_controls_launcher PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
        -Wno-unused-parameter
    )
endif()

# Debug模式下的额外选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(flight_controls_launcher PRIVATE QT_QML_DEBUG)
endif()

# Release模式下的优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(flight_controls_launcher PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

# 设置输出属性
set_target_properties(flight_controls_launcher PROPERTIES
    OUTPUT_NAME "flight_controls_launcher"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Windows特殊设置
if(WIN32)
    set_target_properties(flight_controls_launcher PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 安装目标
install(TARGETS flight_controls_launcher
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

# 打包配置
set(CPACK_PACKAGE_NAME "FlightControls_Launcher")
set(CPACK_PACKAGE_VERSION "5.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Flight Controls Floating Launcher - Simple Application Launcher")
set(CPACK_PACKAGE_VENDOR "FlightControls Team")
set(CPACK_PACKAGE_CONTACT "support@flightcontrols.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# 平台特定的打包
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

include(CPack)

# 输出构建信息
message(STATUS "=== FlightControls 浮动启动器构建配置 ===")
message(STATUS "项目: 基于Qt的浮动启动器")
message(STATUS "方案特点: 悬浮显示在屏幕顶部，提供QGC和RVIZ的统一启动按钮")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt5 版本: ${Qt5_VERSION}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "启动应用: QGroundControl, RVIZ")
message(STATUS "跨平台支持: Windows, Linux, macOS")
message(STATUS "输出文件: flight_controls_launcher")
message(STATUS "安装目录: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "============================================") 