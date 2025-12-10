# Qt + Zed 工作区
- `scripts/setup_qt_env.sh`：一键检测依赖并通过 Homebrew 安装 Qt、CMake、Ninja。
- `CMakeLists.txt` 与 `src/main.cpp`：最小 Qt Widgets 示例，可直接 `cmake --preset qt-debug` 构建。
- `cmake/FindWrapOpenGL.cmake`：重写 Qt 自带模块，避免 macOS 14+ 缺失 `AGL.framework` 时链接失败。
- `.zed/`：自带 Zed 任务与语言设置，开箱即可调用 clangd 与 CMake Preset。
- `.clangd`：让 clangd 默认包含 Qt 头文件与 Framework 路径。
- `docs/zed-config.md`：解释上述配置及常见拓展玩法。

## 快速开始
1. `bash scripts/setup_qt_env.sh`（如需自定义路径可先导出 `QT_PREFIX`）。
2. `cmake --preset qt-debug && cmake --build --preset qt-debug`。
3. `ln -sf build/debug/compile_commands.json compile_commands.json`，让 Zed/clangd 获得编译命令。
4. `open build/debug/QtZedDemo.app` 验证 GUI 成功启动。
