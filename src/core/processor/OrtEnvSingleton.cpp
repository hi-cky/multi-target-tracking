#include "core/processor/OrtEnvSingleton.h"

Ort::Env &GetOrtEnv() {
    static Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "mt-tracking"};
    return env;
}
