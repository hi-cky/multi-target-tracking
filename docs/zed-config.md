# Zed + Qt 工作流指南

## 1. LSP 设置
仓库自带 `.zed/settings.json`，运行 `source scripts/setup_qt_env.sh` 后 `QT_PATH` 会被写成 `brew --prefix qt@6` 的值（通常是 `/opt/homebrew/opt/qt`），可用 `echo $QT_PATH` 复核：

```jsonc
{
  // Zed 当前不支持在 settings.json 中新增自定义 LSP 名称
  // 官方建议仍然由 clangd 提供 C++ 语义分析
  "languages": {
    "C++": {
      "language_servers": ["clangd"]
    },
    "QML": {
      "language_servers": ["clangd"]
    }
  }
}
```

> ⚠️ 说明：根据 Zed 团队讨论，`"lsp"` 节点暂不接受 `qt-ls` 这类自定义属性，若需要集成 Qt Language Server 只能等待官方插件机制。当前做法是让 clangd 正确读取 `compile_commands.json`，即可识别 Qt 头文件与宏。citeturn0search1turn0search2

同时 `.clangd` 也预置了 Qt `-I/-F` 路径：

```yaml
# 为 clangd 注入 Qt include/Framework 搜索路径
CompileFlags:
  Add:
    - "-I$QT_PATH/include"
    - "-I$QT_PATH/include/QtCore"
    - "-I$QT_PATH/include/QtGui"
    - "-I$QT_PATH/include/QtWidgets"
    - "-F$QT_PATH/lib"
```

## 2. 构建任务
项目根的 `.zed/tasks.json` 已给出可直接运行的模板，命令面板里输入任务名称即可：

```jsonc
{
  // Zed 任务触发 cmake preset，方便一键构建
  "tasks": [
    {
      "label": "Configure Qt Debug",
      "command": "cmake --preset qt-debug",
      "type": "shell"
    },
    {
      "label": "Build Qt Debug",
      "command": "cmake --build --preset qt-debug",
      "type": "shell"
    },
    {
      "label": "Run Qt App",
      "command": "open build/debug/QtZedDemo.app",
      "type": "shell"
    }
  ]
}
```

## 3. 调试建议
1. 在 `settings.json` 中配置 `debug.adapters.lldb`，将 `command` 指向 `/usr/bin/lldb-vscode`。
2. 调试任务中引用 `cmake --build` 产出的可执行文件，例如 `build/debug/QtZedDemo.app/Contents/MacOS/QtZedDemo`。
3. 使用 `lldb` 启动前先导出 `QT_DEBUG_PLUGINS=1`，便于定位插件加载问题。

## 4. 工作流小贴士
- 每次打开 Zed 终端先执行 `source scripts/setup_qt_env.sh` 或者在 shell profile 中保留 `QT_PATH`。
- 脚本会根据 `brew --prefix qt@6` 自动更新 `~/.zprofile`，若切换到 Qt Online Installer 版本，记得手动改写该变量并重新登录 shell。
- 如需多版本 Qt，可在 `CMakePresets.json` 中再添加 `qt68-release` 等预设，并切换 `QT_PATH`。
- `macdeployqt build/release/QtZedDemo.app` 可在 Zed 任务中再添加一个步骤，自动打包。
- `cmake/FindWrapOpenGL.cmake` 已替换 Qt 自带模块，macOS Tahoe 及更新版本无需担心 `AGL.framework` 缺失导致的链接报错。
- 构建后执行 `ln -sf build/debug/compile_commands.json compile_commands.json`（或切换到 release preset 的路径），clangd 才能同步最新的 include 设置。
