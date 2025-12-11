#!/usr/bin/env bash
set -euo pipefail

# 🎯 **Ubuntu系统依赖安装脚本（支持GPU）**
# 为 C++ 多目标跟踪项目 (Qt6 + OpenCV4 + ONNX Runtime) 安装所有必要依赖
# 支持CPU和GPU版本的ONNX Runtime

echo "🚀 开始安装多目标跟踪项目的依赖..."
echo "════════════════════════════════════════════════════════════"

# 📝 **GPU支持选项**
echo "🤖 ONNX Runtime GPU支持选项："
echo "1. 🔵 CPU版本 （默认，无需额外依赖）"
echo "2. 🟢 CUDA版本 （需要NVIDIA GPU和CUDA驱动）"
echo "3. 🟡 TensorRT版本 （需要NVIDIA GPU和TensorRT）"
echo ""
read -p "请选择ONNX Runtime版本 [1-3] (默认1): " ONNX_CHOICE
ONNX_CHOICE=${ONNX_CHOICE:-1}

case $ONNX_CHOICE in
    1)
        ONNX_SUFFIX=""
        ONNX_TYPE="CPU"
        echo "✅ 选择CPU版本ONNX Runtime"
        ;;
    2)
        ONNX_SUFFIX="-gpu"
        ONNX_TYPE="CUDA"
        echo "✅ 选择CUDA GPU版本ONNX Runtime"
        ;;
    3)
        ONNX_SUFFIX="-gpu"
        ONNX_TYPE="TensorRT"
        echo "✅ 选择TensorRT GPU版本ONNX Runtime"
        ;;
    *)
        ONNX_SUFFIX=""
        ONNX_TYPE="CPU"
        echo "⚠️  无效选择，使用默认CPU版本"
        ;;
esac

echo ""

# 📦 **1. 更新系统包列表**
echo "📦 更新系统包列表..."
sudo apt-get update

# 🔧 **2. 安装基础构建工具**
echo "🔧 安装基础构建工具..."
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    curl \
    wget \
    git \
    unzip \
    software-properties-common

# 🎨 **3. 安装Qt6开发环境**
echo "🎨 安装Qt6开发环境..."
sudo apt-get install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-l10n-tools

# 📸 **4. 安装OpenCV4开发库**
echo "📸 安装OpenCV4开发库..."
sudo apt-get install -y \
    libopencv-dev \
    libopencv-core-dev \
    libopencv-highgui-dev \
    libopencv-imgproc-dev \
    libopencv-imgcodecs-dev \
    libopencv-videoio-dev \
    libopencv-video-dev \
    libopencv-calib3d-dev \
    libopencv-features2d-dev

# 🤖 **5. 安装ONNX Runtime（根据选择安装不同版本）**
echo "🤖 安装ONNX Runtime $ONNX_TYPE 版本..."

# ONNX Runtime版本
ONNXRUNTIME_VERSION="1.20.0"
ONNXRUNTIME_DIR="/opt/onnxruntime"

echo "📥 下载ONNX Runtime $ONNX_TYPE v${ONNXRUNTIME_VERSION}..."

# 创建临时目录
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

# 根据系统架构下载对应的包
ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    # 根据不同版本选择不同的下载URL
    if [ "$ONNX_TYPE" = "CPU" ]; then
        ONNX_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz"
        echo "🔗 下载CPU版本: $ONNX_URL"
    elif [ "$ONNX_TYPE" = "CUDA" ] || [ "$ONNX_TYPE" = "TensorRT" ]; then
        # CUDA版本（包含CUDA 12支持）
        ONNX_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-gpu-${ONNXRUNTIME_VERSION}.tgz"
        echo "🔗 下载GPU版本: $ONNX_URL"
    fi
elif [ "$ARCH" = "aarch64" ]; then
    # ARM架构
    if [ "$ONNX_TYPE" = "CPU" ]; then
        ONNX_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-aarch64-${ONNXRUNTIME_VERSION}.tgz"
        echo "🔗 下载ARM CPU版本: $ONNX_URL"
    else
        echo "❌ ARM架构暂不支持GPU版本的ONNX Runtime"
        echo "💡 请选择CPU版本或手动编译GPU版本"
        exit 1
    fi
else
    echo "❌ 不支持的架构: $ARCH"
    exit 1
fi

# 下载ONNX Runtime
wget "$ONNX_URL" -O onnxruntime.tgz
tar -xzf onnxruntime.tgz

# 删除旧版本（如果存在）
if [ -d "$ONNXRUNTIME_DIR" ]; then
    echo "🗑️  删除旧版本ONNX Runtime..."
    sudo rm -rf "$ONNXRUNTIME_DIR"
fi

# 创建安装目录
sudo mkdir -p "$ONNXRUNTIME_DIR/include"
sudo mkdir -p "$ONNXRUNTIME_DIR/lib"
sudo cp -r onnxruntime-linux-*/include/* "$ONNXRUNTIME_DIR/include/"
sudo cp -r onnxruntime-linux-*/lib/* "$ONNXRUNTIME_DIR/lib/"

# 🔧 **5.1 如果是GPU版本，安装CUDA相关依赖**
if [ "$ONNX_TYPE" = "CUDA" ] || [ "$ONNX_TYPE" = "TensorRT" ]; then
    echo "🔄 安装GPU版本额外依赖..."

    # 检查NVIDIA驱动
    if ! command -v nvidia-smi &> /dev/null; then
        echo "⚠️  NVIDIA驱动未检测到，GPU加速可能不可用"
        echo "💡 请确保已安装："
        echo "   - NVIDIA驱动 (>470)"
        echo "   - CUDA Toolkit (11.6-12.3)"

        # 尝试安装CUDA（可选）
        read -p "是否尝试安装CUDA Toolkit? (y/N): " INSTALL_CUDA
        if [[ "$INSTALL_CUDA" =~ ^[Yy]$ ]]; then
            echo "📦 添加NVIDIA CUDA仓库..."
            wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
            sudo dpkg -i cuda-keyring_1.1-1_all.deb
            sudo apt-get update
            sudo apt-get install -y cuda-toolkit-12-3

            echo "✅ CUDA Toolkit已安装"
            echo "📝 请重启系统或运行: source /etc/bash.bashrc"
        fi
    else
        echo "✅ NVIDIA驱动已安装"
        NVIDIA_VERSION=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader | head -1)
        echo "   🚀 驱动版本: $NVIDIA_VERSION"
    fi

    # TensorRT额外依赖
    if [ "$ONNX_TYPE" = "TensorRT" ]; then
        echo "🧠 TensorRT版本额外说明:"
        echo "💡 需要手动安装TensorRT:"
        echo "   https://developer.nvidia.com/tensorrt"
    fi
fi

# 🔧 **6. 修复ONNX Runtime路径问题**
echo "🔧 修复ONNX Runtime路径问题..."

# 创建lib64符号链接（如果不存在）
if [ ! -e "$ONNXRUNTIME_DIR/lib64" ]; then
    sudo ln -sf "$ONNXRUNTIME_DIR/lib" "$ONNXRUNTIME_DIR/lib64"
    echo "✅ 创建符号链接: $ONNXRUNTIME_DIR/lib64 -> $ONNXRUNTIME_DIR/lib"
fi

# 创建include/onnxruntime子目录（如果CMake需要）
if [ ! -d "$ONNXRUNTIME_DIR/include/onnxruntime" ] && [ -d "$ONNXRUNTIME_DIR/include" ]; then
    sudo ln -sf "$ONNXRUNTIME_DIR/include" "$ONNXRUNTIME_DIR/include/onnxruntime"
    echo "✅ 创建include目录链接: $ONNXRUNTIME_DIR/include/onnxruntime"
fi

# 修复CMake配置文件中的路径（如果存在）
CMAKE_DIR="$ONNXRUNTIME_DIR/lib/cmake/onnxruntime"
if [ -d "$CMAKE_DIR" ]; then
    echo "🛠️  修复CMake配置文件..."
    for cmake_file in "$CMAKE_DIR"/*.cmake; do
        # 修复lib64路径
        if grep -q "/lib64/libonnxruntime" "$cmake_file" 2>/dev/null; then
            sudo sed -i 's|/lib64/libonnxruntime|/lib/libonnxruntime|g' "$cmake_file"
        fi
        # 修复include路径
        if grep -q "/opt/onnxruntime/include/onnxruntime" "$cmake_file" 2>/dev/null; then
            sudo sed -i 's|/opt/onnxruntime/include/onnxruntime|/opt/onnxruntime/include|g' "$cmake_file"
        fi
    done
    echo "✅ CMake配置文件已修复"
fi

# 设置环境变量
echo "🔧 配置环境变量..."
cat << 'EOF' | sudo tee /etc/profile.d/onnxruntime.sh
export ONNXRUNTIME_ROOT="$ONNXRUNTIME_DIR"
export LD_LIBRARY_PATH="$ONNXRUNTIME_DIR/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
export PATH="\$ONNXRUNTIME_DIR/bin:\$PATH"
EOF

# 为当前shell设置环境变量
export ONNXRUNTIME_ROOT="$ONNXRUNTIME_DIR"
export LD_LIBRARY_PATH="$ONNXRUNTIME_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# 📦 **7. 安装其他可选依赖**
echo "📦 安装其他可选依赖..."
sudo apt-get install -y \
    libgtest-dev \
    libgoogle-glog-dev \
    libeigen3-dev \
    libboost-all-dev \
    libtbb-dev

# 🧪 **8. 验证安装**
echo "🧪 验证安装..."

# 检查CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    echo "✅ CMake: $CMAKE_VERSION"
else
    echo "❌ CMake未安装"
fi

# 检查Qt
if dpkg -l | grep -q qt6-base-dev; then
    echo "✅ Qt6开发包已安装"
else
    echo "❌ Qt6开发包未安装"
fi

# 检查OpenCV
if pkg-config --exists opencv4; then
    OPENCV_VERSION=$(pkg-config --modversion opencv4)
    echo "✅ OpenCV: $OPENCV_VERSION"
else
    echo "⚠️  OpenCV未通过pkg-config检测"
fi

# 检查ONNX Runtime
if [ -f "$ONNXRUNTIME_DIR/include/onnxruntime_cxx_api.h" ]; then
    echo "✅ ONNX Runtime $ONNX_TYPE 版本已安装到: $ONNXRUNTIME_DIR"

    # 检查GPU支持（仅限GPU版本）
    if [ "$ONNX_TYPE" != "CPU" ]; then
        echo "🔍 检查GPU支持..."
        if strings "$ONNXRUNTIME_DIR/lib/libonnxruntime.so" 2>/dev/null | grep -q "CUDAExecutionProvider"; then
            echo "✅ ONNX Runtime包含CUDA支持"
        else
            echo "⚠️  ONNX Runtime可能不包含CUDA支持"
        fi
    fi
else
    echo "❌ ONNX Runtime安装失败"
fi

# 🧪 **9. GPU特定检查**
if [ "$ONNX_TYPE" != "CPU" ]; then
    echo ""
    echo "🎮 GPU环境检查..."

    # 检查NVIDIA驱动
    if command -v nvidia-smi &> /dev/null; then
        nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv
        echo "✅ NVIDIA GPU检测成功"
    else
        echo "⚠️  NVIDIA GPU未检测到，CUDA可能无法工作"
        echo "💡 请确保："
        echo "   - 安装NVIDIA驱动: sudo ubuntu-drivers autoinstall"
        echo "   - 重启系统"
    fi

    # 检查CUDA
    if command -v nvcc &> /dev/null || [ -f "/usr/local/cuda/bin/nvcc" ]; then
        CUDA_PATH=$(which nvcc)
        echo "✅ CUDA编译器找到: $CUDA_PATH"
    else
        echo "⚠️  CUDA编译器未找到"
        echo "💡 建议安装CUDA Toolkit 11.6-12.3"
    fi
fi

echo ""
echo "🎉 依赖安装完成！"
echo "════════════════════════════════════════════════════════════"

echo "📋 **后续步骤：**"
echo "1. 重新加载环境变量: source /etc/profile 或重新打开终端"
echo "2. 进入项目目录: cd /path/to/multi-target-tracking"
echo "3. 清理旧构建: rm -rf build/ output/"
echo "4. 重新配置CMake:"

if [ "$ONNX_TYPE" = "CPU" ]; then
    echo "   cmake --preset qt-debug"
else
    echo "   export QT_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6"
    echo "   export ONNXRUNTIME_ROOT=/opt/onnxruntime"
    echo "   cmake --preset qt-debug"
fi

echo "5. 重新编译: cmake --build --preset qt-debug"
echo "6. 验证安装: ./output/QtZedDemo"

echo ""
echo "💡 **GPU使用说明：**"
if [ "$ONNX_TYPE" != "CPU" ]; then
    echo "🔧 在你的C++代码中，添加以下代码启用GPU："
    echo "   Ort::SessionOptions session_options;"
    echo "   OrtCUDAProviderOptions cuda_options{};"
    echo "   session_options.AppendExecutionProvider_CUDA(cuda_options);"
else
    echo "🔧 当前为CPU版本，如需GPU支持请重新运行本脚本选择GPU版本"
fi

# 🧹 **清理临时文件**
cd /
rm -rf "$TEMP_DIR"
echo ""
echo "🧹 临时文件已清理"

exit 0
