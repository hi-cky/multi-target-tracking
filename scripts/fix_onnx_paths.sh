#!/usr/bin/env bash
set -euo pipefail

# 🔧 **ONNX Runtime路径修复脚本**
# 修复CMake配置文件中硬编码的lib64路径问题

echo "🔧 开始修复ONNX Runtime的路径问题..."

ONNXRUNTIME_DIR="${ONNXRUNTIME_ROOT:-/opt/onnxruntime}"
CMAKE_DIR="$ONNXRUNTIME_DIR/lib/cmake/onnxruntime"

if [ ! -d "$CMAKE_DIR" ]; then
    echo "❌ ONNX Runtime的CMake配置目录不存在: $CMAKE_DIR"
    exit 1
fi

echo "📁 检查CMake配置文件..."

# 修复所有CMake配置文件中的lib64路径
FIXED_FILES=0
for cmake_file in "$CMAKE_DIR"/*.cmake; do
    if grep -q "lib64/libonnxruntime" "$cmake_file"; then
        echo "🛠️  修复文件: $(basename "$cmake_file")"
        sudo sed -i 's|/lib64/libonnxruntime|/lib/libonnxruntime|g' "$cmake_file"
        FIXED_FILES=$((FIXED_FILES + 1))
    fi

    # 修复include路径，从/opt/onnxruntime/include/onnxruntime改为/opt/onnxruntime/include
    if grep -q "/opt/onnxruntime/include/onnxruntime" "$cmake_file"; then
        echo "🛠️  修复include路径: $(basename "$cmake_file")"
        sudo sed -i 's|/opt/onnxruntime/include/onnxruntime|/opt/onnxruntime/include|g' "$cmake_file"
        FIXED_FILES=$((FIXED_FILES + 1))
    fi
done

if [ $FIXED_FILES -eq 0 ]; then
    echo "✅ 没有发现需要修复的lib64路径，CMake配置看起来正常"
else
    echo "✅ 已修复 $FIXED_FILES 个CMake配置文件"
fi

# 创建lib64符号链接作为备选方案
echo "🔗 创建lib64符号链接..."
if [ ! -e "$ONNXRUNTIME_DIR/lib64" ]; then
    sudo ln -sf "$ONNXRUNTIME_DIR/lib" "$ONNXRUNTIME_DIR/lib64"
    echo "✅ 已创建符号链接: $ONNXRUNTIME_DIR/lib64 -> $ONNXRUNTIME_DIR/lib"
else
    echo "ℹ️  lib64链接已存在，跳过创建"
fi

# 创建onnxruntime include子目录链接（如果CMake配置需要）
echo "🔗 检查include目录结构..."
if [ ! -d "$ONNXRUNTIME_DIR/include/onnxruntime" ] && [ -d "$ONNXRUNTIME_DIR/include" ]; then
    # 如果CMake配置还是需要/onnxruntime子目录，创建符号链接
    if grep -r "include/onnxruntime" "$CMAKE_DIR" 2>/dev/null | grep -q "INTERFACE_INCLUDE_DIRECTORIES"; then
        echo "🛠️  创建onnxruntime include子目录链接..."
        sudo ln -sf "$ONNXRUNTIME_DIR/include" "$ONNXRUNTIME_DIR/include/onnxruntime"
        echo "✅ 已创建include目录链接: $ONNXRUNTIME_DIR/include/onnxruntime -> $ONNXRUNTIME_DIR/include"
    else
        echo "ℹ️  CMake配置不再需要onnxruntime子目录，跳过创建"
    fi
fi

echo ""
echo "🎉 ONNX Runtime路径修复完成！"
echo ""
echo "📋 **验证修复：**"
echo "1. 检查CMake配置内容："
echo "   grep -r 'lib/libonnxruntime' $CMAKE_DIR || true"
echo ""
echo "2. 验证符号链接："
echo "   ls -la $ONNXRUNTIME_DIR/lib64"
echo ""
echo "🚀 现在可以重新运行CMake配置："
echo "   cmake --preset qt-debug"

exit 0
