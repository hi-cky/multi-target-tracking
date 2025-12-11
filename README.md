# 多目标跟踪项目 (C++ Qt6 + OpenCV + ONNX Runtime)

## 📋 平台支持
- **macOS**: 使用 Homebrew 安装依赖
- **Ubuntu**: 使用 apt 安装依赖
- **其他Linux**: 手动安装所需包

## 📁 项目文件说明

### 脚本文件
- `scripts/setup_qt_env.sh`：macOS 一键检测依赖并通过 Homebrew 安装 Qt、CMake、Ninja。
- `scripts/ubuntu_setup.sh`：Ubuntu/Linux 一键安装所有系统依赖（Qt6、OpenCV4、ONNX Runtime）。
- `scripts/verify_deps.sh`：验证所有依赖是否已正确安装。

### 核心构建文件
- `CMakeLists.txt`：项目主构建文件，配置 Qt6、OpenCV、ONNX Runtime 依赖。
- `CMakePresets.json`：CMake 预设配置，支持 Qt Debug 和 Release 构建。
- `src/main.cpp`：程序入口点。

### 平台特定文件
- `cmake/FindWrapOpenGL.cmake`：重写 Qt 自带模块，避免 macOS 14+ 缺失 `AGL.framework` 时链接失败。

### 开发工具配置
- `.zed/`：自带 Zed 任务与语言设置，开箱即可调用 clangd 与 CMake Preset。
- `.clangd`：让 clangd 默认包含 Qt 头文件与 Framework 路径。
- `docs/zed-config.md`：解释上述配置及常见拓展玩法。


## 🚀 快速开始

### 对于 macOS 用户
1. `bash scripts/setup_qt_env.sh`（如需自定义路径可先导出 `QT_PREFIX`）。
2. `cmake --preset qt-debug && cmake --build --preset qt-debug`。
3. `ln -sf build/debug/compile_commands.json compile_commands.json`，让 Zed/clangd 获得编译命令。
4. `open build/debug/QtZedDemo.app` 验证 GUI 成功启动。

### 对于 Ubuntu/Linux 用户

**第一步：安装系统依赖**
```bash
# 安装所有必需依赖（需要sudo权限）
sudo bash scripts/ubuntu_setup.sh

# 安装完成后，重新加载环境变量
source /etc/profile
```

**第二步：验证安装**
```bash
# 运行验证脚本检查所有依赖
bash scripts/verify_deps.sh
```

**第三步：构建项目**
```bash
# 创建构建目录
mkdir -p build && cd build

# 配置项目（x86架构）
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 ..

# 对于ARM架构（如树莓派、NVIDIA Jetson）
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/lib/aarch64-linux-gnu/cmake/Qt6 ..

# 编译项目
make -j$(nproc)

# 或者使用CMake Presets（需要先设置QT_PATH环境变量）
export QT_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake --preset qt-cuda-debug && cmake --build --preset qt-cuda-debug
```

**第四步：运行程序**
```bash
# 运行可执行文件
./output/QtZedDemo
```

