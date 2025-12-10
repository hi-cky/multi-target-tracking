cmake_minimum_required(VERSION 3.22)

# 声明项目名称与版本，并确保统一的 C++20 标准
project(QtZedDemo VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 统一将可执行文件（含 .app bundle）输出到根目录下的 output 便于定位
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output")

# 优先加载项目内自定义的 FindWrapOpenGL 以绕开 macOS 14+ 缺失 AGL.framework 的问题
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 启用 Qt 自动处理 Moc/UIC/RCC，降低 zed 项目维护成本
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找必须的 Qt6 Widgets 组件，若缺失将提示安装 CLI SDK（确保 Qt UI 头文件与库可用）
find_package(Qt6 6.5 COMPONENTS Widgets REQUIRED)

# 查找 OpenCV 4 并列出核心/图像处理/读写模块，保障 mainwindow.h 中的 Mat 等类型能解析（绑定本机 Homebrew 提供的 C++ OpenCV 库）
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs highgui video)
# 仅链接必要的 OpenCV 模块，避免引入 dnn 内置 ONNX 造成 schema 冲突
set(OPENCV_NEEDED_LIBS opencv_core opencv_imgproc opencv_imgcodecs opencv_highgui opencv_video)

# 引入 onnxruntime C++ 包以支撑 YOLO 检测推理
find_package(onnxruntime CONFIG REQUIRED
    HINTS
        $ENV{ONNXRUNTIME_ROOT}
        /opt/homebrew/opt/onnxruntime
        /usr/local/opt/onnxruntime
        /opt/homebrew/Cellar/onnxruntime
)

# 自动递归收集 src 目录下的全部 C/C++ 源文件与 Qt 资源文件（子目录修改也会被自动捕获）
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_SOURCE_DIR}/src/*.mm"
    "${CMAKE_SOURCE_DIR}/src/*.qrc"
)

# 定义主程序并包含所有 UI/业务源文件（确保 MainWindow 等符号参与编译）
qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# 在 macOS 生成 .app bundle，保持 GUI 应用体验
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 将项目 src 目录与 OpenCV 头文件路径注入目标（供 MOC 生成代码找到 wavebutton.h 等本地头，外加 opencv2/*.hpp）
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# 链接 Qt Widgets、OpenCV 与 onnxruntime 库，保证推理/绘制依赖完整
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets ${OPENCV_NEEDED_LIBS} onnxruntime::onnxruntime)

# 添加安装规则，方便后续用 macdeployqt 或 CPack 统一打包
install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION . RUNTIME DESTINATION bin)

# 构建期集成 GoogleTest，确保 detector 模块的核心逻辑可测试
set(BUILD_TESTING OFF CACHE BOOL "是否构建并下载单元测试依赖（默认关闭以避免首次配置时长时间下载）")
include(CTest)
if (BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/tests/*.cc"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    )

    if (UNIT_TEST_SOURCES)
        add_executable(detector_tests ${UNIT_TEST_SOURCES})
        target_sources(detector_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/src/core/processor/model/detector/YoloDetector.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/model/detector/BBox.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/model/feature_extractor/FeatureExtractor.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/model/feature_extractor/Feature.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/matcher/Matcher.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/tracker_manager/Tracker.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/tracker_manager/TrackerManager.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/TrackingEngine.cpp
            ${CMAKE_SOURCE_DIR}/src/core/processor/OrtEnvSingleton.cpp
        )
        target_link_libraries(detector_tests PRIVATE gtest_main Qt6::Widgets ${OPENCV_NEEDED_LIBS} onnxruntime::onnxruntime)
        target_include_directories(detector_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
        target_compile_definitions(detector_tests PRIVATE PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
        add_test(NAME detector_tests COMMAND detector_tests)
    endif ()
endif ()
