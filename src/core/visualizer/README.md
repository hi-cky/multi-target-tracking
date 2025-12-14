# c++_visual：可视化与统计模块（OpenCV）

本目录提供两个独立可复用的 C++ 模块：

- `Visualizer`：把 “原始帧 + 标注数据（bbox + id）” 渲染成带框、带 ID 的图像帧
- `StatsRecorder`：把每帧标注数据写入 CSV（可选额外统计列 `unique_ids_seen`）

同时提供：

- 可直接编译运行的 demo / 测试入口
- 一份可用的 `AppendixA.h`（来自 `require.txt` 附录 A 的数据结构定义）

> 说明：原始需求/设计文档在 `require.txt`；项目说明在 `README.txt`。

---

## 目录结构

- `AppendixA.h`：公共数据结构（`LabeledObject` / `LabeledFrame`）
- `Visualizer.h` / `Visualizer.cpp`：可视化模块实现
- `StatsRecorder.h` / `StatsRecorder.cpp`：统计写 CSV 模块实现
- `test_visualizer.cpp`：可视化模块的“无窗口”自测（输出 PNG）
- `test_main.cpp`：统计模块自测（输出 CSV）
- `demo_visualizer.cpp`：可视化 demo（尝试 `imshow`，同时输出 PNG）
- `run_demo_visualizer.ps1` / `run_demo_visualizer.bat`：Windows 下启动 demo 的便捷脚本（自动设置运行时 DLL/Qt 插件路径）
- `CMakeLists.txt`：可选的 CMake 工程文件（如果本机安装了 CMake）

---

## 数据结构（Appendix A）

`AppendixA.h` 中定义了两种结构体：

- `LabeledObject`：`id`、`bbox`（`cv::Rect`）、`class_id`、`score`
- `LabeledFrame`：`frame_index`、`objs`（`std::vector<LabeledObject>`）

如果你们项目已经有自己的 “附录 A” 头文件：

- 直接用你们的头文件替换/覆盖 `AppendixA.h` 的内容即可
- 或在 `StatsRecorder.h` / `Visualizer.h` 中把 `#include "AppendixA.h"` 改成你们真实的路径

---

## 依赖

- Windows 10/11
- C++17 编译器
- OpenCV（包含 `core/imgproc/imgcodecs/highgui`）

本仓库当前最容易复现的环境是 **MSYS2 ucrt64**：

- 编译器：`D:\msys64\ucrt64\bin\g++.exe`
- OpenCV：`mingw-w64-ucrt-x86_64-opencv`
- GUI（`imshow`）：OpenCV highgui 在此环境通常依赖 **Qt6** 运行时

---

## 快速开始（Windows + MSYS2 ucrt64）

### 1）安装依赖

在 PowerShell 执行：

```powershell
D:\msys64\usr\bin\bash.exe -lc "pacman -S --needed mingw-w64-ucrt-x86_64-opencv"
```

如果你需要运行 `imshow`（demo 会弹窗），还需要 Qt6（避免缺 `Qt6*.dll`）：

```powershell
D:\msys64\usr\bin\bash.exe -lc "pacman -S --needed mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat"
```

### 2）编译（不依赖 CMake）

在 `d:\danzi\c++_visual` 目录执行：

```powershell
New-Item -ItemType Directory -Force build_msys | Out-Null
$cflags = (& D:\msys64\ucrt64\bin\pkg-config.exe --cflags opencv4) -split '\s+'
$libs   = (& D:\msys64\ucrt64\bin\pkg-config.exe --libs opencv4) -split '\s+'

# 统计模块测试
& D:\msys64\ucrt64\bin\g++.exe -std=c++17 -O2 -Wall -Wextra -pedantic `
  -o build_msys\test_stats_recorder.exe StatsRecorder.cpp Visualizer.cpp test_main.cpp @cflags @libs

# 可视化模块测试（无窗口）
& D:\msys64\ucrt64\bin\g++.exe -std=c++17 -O2 -Wall -Wextra -pedantic `
  -o build_msys\test_visualizer.exe StatsRecorder.cpp Visualizer.cpp test_visualizer.cpp @cflags @libs

# 可视化演示（尝试弹窗）
& D:\msys64\ucrt64\bin\g++.exe -std=c++17 -O2 -Wall -Wextra -pedantic `
  -o build_msys\demo_visualizer.exe StatsRecorder.cpp Visualizer.cpp demo_visualizer.cpp @cflags @libs
```

### 3）运行

#### 运行统计模块测试

```powershell
$env:PATH = "D:\msys64\ucrt64\bin;$env:PATH"
.\build_msys\test_stats_recorder.exe
```

输出文件：

- `stats.csv`
- `stats_extra.csv`

#### 运行可视化模块测试（无窗口）

```powershell
$env:PATH = "D:\msys64\ucrt64\bin;$env:PATH"
.\build_msys\test_visualizer.exe
```

输出文件：

- `visualizer_test_output.png`

#### 运行可视化 demo（弹窗 + PNG）

推荐用脚本启动（自动设置 Qt 插件路径）：

- PowerShell：`.\run_demo_visualizer.ps1`
- CMD：`.\run_demo_visualizer.bat`

输出文件：

- `demo_visualizer_output.png`

---

## 可选：使用 CMake

如果你本机安装了 `cmake`，可以用 `CMakeLists.txt` 配置工程（会生成 `test_stats_recorder` / `test_visualizer` / `demo_visualizer` 三个目标）。

> 注意：本项目运行环境中可能没有 `cmake`，因此默认推荐上面的“直接 g++ 编译”方式。

---

## 常见问题

### 1）`demo_visualizer.exe` 运行失败（0xC0000135 / 缺 DLL）

原因通常是缺少 Qt6 运行库或 Qt 平台插件（OpenCV highgui 依赖）。

解决：

- 安装 Qt6：见“安装依赖”小节
- 使用 `run_demo_visualizer.ps1` / `run_demo_visualizer.bat` 启动（脚本会设置 `QT_PLUGIN_PATH` / `QT_QPA_PLATFORM_PLUGIN_PATH`）

### 2）只想验证渲染，不想弹窗

直接运行 `test_visualizer.exe`，它不会 `imshow`，只会写出 `visualizer_test_output.png`。

