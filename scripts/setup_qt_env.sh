#!/usr/bin/env bash
set -euo pipefail

# 定义默认 Qt 版本，安装路径稍后根据 Homebrew 自动探测
QT_VERSION="${QT_VERSION:-6.9.0}"

# 确保 Xcode Command Line Tools 就绪
if ! xcode-select -p >/dev/null 2>&1; then
    echo "[动作] 触发 xcode-select --install 来安装命令行工具"
    xcode-select --install || true
else
    echo "[信息] Xcode Command Line Tools 已安装"
fi

# 检查 Homebrew，并提示通过官方网站安装
if ! command -v brew >/dev/null 2>&1; then
    echo "[缺失] 未检测到 Homebrew，请参考 https://brew.sh 安装"
    exit 1
fi

# 安装 CMake 与 Ninja 以配合 Zed 构建
brew install cmake ninja || true

# 安装 Qt CLI 版本，供 CMakePresets 使用
brew install qt@6 || true

# 根据 brew --prefix 自动探测 Qt 安装路径，允许用户通过 QT_PREFIX 覆盖
BREW_QT_PREFIX="$(brew --prefix qt@6 2>/dev/null || true)"
if [[ -z "${BREW_QT_PREFIX}" ]]; then
    BREW_QT_PREFIX="/opt/homebrew/opt/qt"
fi
QT_PREFIX="${QT_PREFIX:-$BREW_QT_PREFIX}"
export QT_PATH="$QT_PREFIX"

# 如果 ~/.zprofile 尚未包含当前路径，则追加一行，避免重复
PROFILE_LINE="export QT_PATH=$QT_PREFIX"
if ! grep -Fq "$PROFILE_LINE" "$HOME/.zprofile" 2>/dev/null; then
    echo "$PROFILE_LINE" >> "$HOME/.zprofile"
fi

echo "[完成] Qt ${QT_VERSION} CLI 环境准备完毕，可在 Zed 内运行 'cmake --preset qt-debug'"
