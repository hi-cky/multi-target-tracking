#pragma once
#include <onnxruntime_cxx_api.h>

// 全局共享的 Ort::Env，避免重复注册 schema 导致冲突
Ort::Env &GetOrtEnv();
