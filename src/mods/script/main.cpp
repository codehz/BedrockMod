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

struct GuildMod {
  void (*fn)(void *);
  const char *name;
  GuildMod(void (*fn)(void *), const char *name)
      : fn(fn)
      , name(name) {}
};

static std::queue<GuildMod> mod_queue;

void script_preload(void (*fn)(void *), const char *name) { mod_queue.emplace(fn, name); }

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

SCM_DEFINE_PUBLIC(s_log, "log-raw", 3, 0, 0, (scm::val<int> level, scm::val<char *> tag, scm::val<char *> content), "Log function") {
  log(level, (temp_string)tag, (temp_string)content);
  return SCM_UNSPECIFIED;
}

LOADFILE(preload, "src/mods/script/preload.scm");

PRELOAD_MODULE("minecraft") {
  scm::definer("log-level-trace")  = (int)LogLevel::LOG_LEVEL_TRACE;
  scm::definer("log-level-debug")  = (int)LogLevel::LOG_LEVEL_DEBUG;
  scm::definer("log-level-info")   = (int)LogLevel::LOG_LEVEL_INFO;
  scm::definer("log-level-notice") = (int)LogLevel::LOG_LEVEL_NOTICE;
  scm::definer("log-level-warn")   = (int)LogLevel::LOG_LEVEL_WARN;
  scm::definer("log-level-error")  = (int)LogLevel::LOG_LEVEL_ERROR;
  scm::definer("log-level-fatal")  = (int)LogLevel::LOG_LEVEL_FATAL;

#ifndef DIAG
#include "main.x"
#include "preload.scm.z"
#endif

  scm_c_eval_string(&file_preload_start);
}

struct strport {
  SCM stream;
  int pos, len;
};

static void handler_message(char *&errstr, SCM tag, SCM args) {
  SCM p, stack, frame;

  p     = scm_open_output_string();
  stack = scm_make_stack(SCM_BOOL_T, scm_list_1(scm_from_int(2)));
  frame = scm_is_true(stack) ? scm_stack_ref(stack, SCM_INUM0) : SCM_BOOL_F;

  scm_print_exception(p, frame, tag, args);
  errstr = scm_to_utf8_string(scm_get_output_string(p));
  scm_close_output_port(p);
}

static void init_thread() {
  scm_init_guile();
  while (!mod_queue.empty()) {
    auto mod = mod_queue.front();
    Log::debug("reg", "%s", mod.name);
    scm_c_define_module(mod.name, mod.fn, (void *)mod.name);
    mod_queue.pop();
  }
  if (exists("scm/scripts/init.scm")) {
    char *errstr = nullptr;
    scm_c_catch(SCM_BOOL_T, (scm_t_catch_body)scm_c_primitive_load, (void *)"scm/scripts/init.scm", (scm_t_catch_handler)handler_message, &errstr,
                nullptr, nullptr);
    if (errstr) {
      errstr[strlen(errstr) - 1] = 0;
      Log::error("guile", "%s", errstr);
      free(errstr);
    }
  }
}

extern "C" void mod_set_server(void *) {
  setenv("GUILE_SYSTEM_PATH", "scm/modules", 1);
  setenv("GUILE_SYSTEM_COMPILED_PATH", "scm/ccache", 1);
  setenv("GUILE_SYSTEM_EXTENSIONS_PATH", "scm/extensions", 1);
  mcpelauncher_server_thread(init_thread);
}