#include <StaticHook.h>
#include <api.h>
#include <dirent.h>
#include <libguile.h>
#include <log.h>
#include <queue>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static std::queue<std::function<void()>> mod_queue;

void script_preload(std::function<void()> f) { mod_queue.push(f); }

std::string GetCwd() {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
    return cwd;
  else
    return "";
}

std::string cwd = GetCwd();

double function(int i, double j) { return i * j; }

bool hasPrefix(std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(0, ending.length(), ending));
  } else {
    return false;
  }
}

bool hasSuffix(std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
  } else {
    return false;
  }
}

inline bool exists(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

void log(int level, const char *tag, const char *content) { Log::log((LogLevel)level, tag, "%s", content); }

static SCM s_log(scm::val<int> level, scm::val<char *> tag, scm::val<char *> content);
SCM_DEFINE(s_log, "log-raw", 3, 0, 0, (scm::val<int> level, scm::val<char *> tag, scm::val<char *> content), "Log function")
#define FUNC_NAME "s_log"
{
  log(level, (temp_string)tag, (temp_string)content);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

LOADFILE(preload, "src/mods/script/preload.scm");

static void init_guile() {
  scm_init_guile();

  scm::definer("log-level-trace")  = (int)LogLevel::LOG_TRACE;
  scm::definer("log-level-debug")  = (int)LogLevel::LOG_DEBUG;
  scm::definer("log-level-info")   = (int)LogLevel::LOG_INFO;
  scm::definer("log-level-notice") = (int)LogLevel::LOG_NOTICE;
  scm::definer("log-level-warn")   = (int)LogLevel::LOG_WARN;
  scm::definer("log-level-error")  = (int)LogLevel::LOG_ERROR;
  scm::definer("log-level-fatal")  = (int)LogLevel::LOG_FATAL;

#ifndef DIAG
#include "main.x"
#include "preload.scm.z"
#endif

  scm_c_eval_string(&file_preload_start);
}

extern "C" void mod_init() { script_preload(init_guile); }

void *test(void *) {
  scm_c_eval_string(R"((log-trace "test" "test"))");

  Log::trace("thread", "!!");
  return nullptr;
}

extern "C" void mod_set_server(void *) {
  mcpelauncher_server_thread <<= [] {
    while (!mod_queue.empty()) {
      mod_queue.front()();
      mod_queue.pop();
    }
    if (exists("user/scm/scripts/init.scm")) scm_c_primitive_load("user/scm/scripts/init.scm");
  };
}