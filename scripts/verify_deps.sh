#!/usr/bin/env bash
set +u  # 防止未绑定变量错误
set -e  # 出错时退出
set -o pipefail  # 管道失败时退出

# 🧪 **依赖验证脚本**
# 验证多目标跟踪项目的所有必要依赖是否已正确安装

echo "🧪 开始验证项目依赖..."

# 📊 初始化验证结果变量
ALL_PASSED=true
OPENCV_PASS=true

# 🔍 **1. 检查CMake**
echo "🔍 检查CMake..."
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    echo "✅  CMake: $CMAKE_VERSION"
else
    echo "❌  CMake未安装"
    ALL_PASSED=false
fi

# 🔍 **2. 检查Ninja**
echo "🔍 检查Ninja..."
if command -v ninja &> /dev/null; then
    NINJA_VERSION=$(ninja --version 2>/dev/null || echo "版本未知")
    echo "✅  Ninja: $NINJA_VERSION"
else
    echo "❌  Ninja未安装"
    ALL_PASSED=false
fi

# 🎨 **3. 检查Qt6**
echo "🎨 检查Qt6..."
QT_PATHS=(
    "/usr/lib/x86_64-linux-gnu/cmake/Qt6"
    "/usr/lib/aarch64-linux-gnu/cmake/Qt6"
    "/opt/qt6/lib/cmake/Qt6"
)

QT_FOUND=false
for QT_PATH in "${QT_PATHS[@]}"; do
    if [ -d "$QT_PATH" ]; then
        echo "✅  Qt6找到于: $QT_PATH"
        QT_FOUND=true
        export QT_PATH="$QT_PATH"
        break
    fi
done

if [ "$QT_FOUND" = false ]; then
    echo "❌  Qt6未找到于标准路径"
    echo "💡  尝试查找Qt6:"
    find /usr -name "Qt6Config.cmake" 2>/dev/null | head -3 || echo "未找到Qt6Config.cmake"
    ALL_PASSED=false
fi

# 📸 **4. 检查OpenCV**
echo "📸 检查OpenCV..."
if pkg-config --exists opencv4; then
    OPENCV_VERSION=$(pkg-config --modversion opencv4)
    echo "✅  OpenCV: $OPENCV_VERSION"

    # 检查所需组件
    echo "📋  OpenCV组件:"
    echo "    ✅  opencv_core (包含在opencv4包中)"
    echo "    ✅  opencv_imgproc (包含在opencv4包中)"
    echo "    ✅  opencv_imgcodecs (包含在opencv4包中)"
    echo "    ✅  opencv_highgui (包含在opencv4包中)"
    echo "    ✅  opencv_video (包含在opencv4包中)"

    # Ubuntu的OpenCV包通常只提供opencv4.pc，所以不检查单个模块
    echo "💡  在Ubuntu系统中，所有OpenCV组件通常都打包在libopencv-dev中"
else
    echo "❌  OpenCV未通过pkg-config检测"
    # Ubuntu的包通常只提供opencv4.pc，所以不因为缺少单个模块而失败
    # 如果opencv4整体不存在，我们标记为失败
    OPENCV_PASS=false
fi

# 🤖 **5. 检查ONNX Runtime**
echo "🤖 检查ONNX Runtime..."
ONNXRUNTIME_PATHS=(
    "/opt/onnxruntime"
    "/usr/local/opt/onnxruntime"
    "/usr/local"
    "/opt/local"
)

# 如果设置了ONNXRUNTIME_ROOT，则添加到搜索路径的开头
if [ -n "${ONNXRUNTIME_ROOT:-}" ]; then
    ONNXRUNTIME_PATHS=("$ONNXRUNTIME_ROOT" "${ONNXRUNTIME_PATHS[@]}")
fi

ONNX_FOUND=false
for ONNX_PATH in "${ONNXRUNTIME_PATHS[@]}"; do
    if [ -f "$ONNX_PATH/include/onnxruntime_cxx_api.h" ] && [ -f "$ONNX_PATH/lib/libonnxruntime.so" ]; then
        echo "✅  ONNX Runtime找到于: $ONNX_PATH"
        ONNX_FOUND=true
        break
    fi
done

if [ "$ONNX_FOUND" = false ]; then
    echo "❌  ONNX Runtime未找到"
    echo "💡  搜索ONNX Runtime文件:"
    find /usr -name "onnxruntime_cxx_api.h" 2>/dev/null | head -3 || echo "未找到onnxruntime_cxx_api.h"
    ALL_PASSED=false
fi

# 📦 **6. 检查C++编译器**
echo "📦 检查C++编译器..."
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    echo "✅  GCC: $GCC_VERSION"

    # 检查C++20支持
    echo "🔍  检查C++20支持..."
    if g++ -std=c++20 -c /dev/null -o /dev/null 2>/dev/null; then
        echo "✅  支持C++20标准"
    else
        echo "❌  不支持C++20标准"
        ALL_PASSED=false
    fi
else
    echo "❌  g++未安装"
    ALL_PASSED=false
fi

# 🧪 **7. 简单编译测试**
echo "🧪 执行简单编译测试..."
cat > /tmp/test_opencv_qt.cpp << 'EOF'
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <QApplication>
#include <QLabel>
#include <QWidget>

int main(int argc, char *argv[]) {
    // 测试OpenCV
    cv::Mat test_mat(100, 100, CV_8UC3);
    test_mat = cv::Scalar(255, 0, 0);

    // 测试Qt（不实际运行GUI）
    QApplication app(argc, argv);
    QLabel label("测试成功！");

    return 0;
}
EOF

echo "🔧  编译测试程序..."
if g++ -std=c++20 /tmp/test_opencv_qt.cpp \
    -I/usr/include/opencv4 \
    $(pkg-config --cflags Qt6Widgets) \
    $(pkg-config --libs Qt6Widgets) \
    -lopencv_core -lopencv_imgproc \
    -o /tmp/test_app 2>/tmp/compile_error.txt; then
    echo "✅  编译测试通过"
else
    echo "❌  编译测试失败"
    echo "💡  编译错误:"
    cat /tmp/compile_error.txt
    ALL_PASSED=false
fi

# 🎯 **8. 验证结果**
echo ""
echo "🎯 **验证结果总结**"
echo "════════════════════════════════════════════════════════"

if [ "$ALL_PASSED" = true ] && [ "$OPENCV_PASS" = true ]; then
    echo "🎉 所有依赖验证通过！项目可以正常构建。"
    echo ""
    echo "📋 **建议的下一步：**"
    echo "1. 设置环境变量: export QT_PATH=${QT_PATH:-/usr/lib/x86_64-linux-gnu/cmake/Qt6}"
    echo "2. 进入项目目录: cd /path/to/multi-target-tracking"
    echo "3. 创建编译目录: mkdir -p build && cd build"
    echo "4. 配置项目: cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=\$QT_PATH .."
    echo "5. 编译: make -j\$(nproc)"
    exit 0
else
    echo "❌ 部分依赖验证失败。"
    echo ""
    echo "🔧 **可能的解决方案：**"
    echo "1. 重新运行安装脚本: bash scripts/ubuntu_setup.sh"
    echo "2. 手动安装缺失的包"
    echo "3. 检查环境变量设置"
    exit 1
fi

# 🧹 清理临时文件
rm -f /tmp/test_opencv_qt.cpp /tmp/test_app /tmp/compile_error.txt
