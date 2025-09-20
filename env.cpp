#include "env.hpp"
#include <mutex>
#include <fstream>
#include <sstream>

static EnvConfig g_env;
static std::mutex g_env_mutex;

void setGlobalEnv(const EnvConfig& e) {
    std::lock_guard<std::mutex> lk(g_env_mutex);
    g_env = e;
}

EnvConfig getGlobalEnv() {
    std::lock_guard<std::mutex> lk(g_env_mutex);
    return g_env;
}
