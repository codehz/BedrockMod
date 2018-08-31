#include <api.h>
#include <dirent.h>
#include <libguile.h>
#include <log.h>
#include <scm/scm.hpp>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

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

chaiscript::ChaiScript chai({ cwd + "/user/chai/modules/" }, { cwd + "/user/chai/scripts/" });

struct LogForChai {
  template <LogLevel Level> void log(std::string tag, std::string content) const { Log::log(Level, ("chai::" + tag).c_str(), "%s", content.c_str()); }
};

template <LogLevel Level> void Slog(std::string tag, std::string content) { Log::log(Level, ("guile::" + tag).c_str(), "%s", content.c_str()); }

static void doLog(int level, std::string tag, std::string content) { Log::log((LogLevel)level, tag.c_str(), content.c_str()); }

static LogForChai LogInstance;

extern "C" void loadModule(chaiscript::ModulePtr ptr) { chai.add(ptr); }
extern "C" chaiscript::ChaiScript &getChai() { return chai; }

struct temp_string {
  char *data;
  std::string get() { return { data }; }
  ~temp_string() { free(data); }
};

static std::string makeString(scm::val val) { return temp_string{ scm_to_utf8_string(val) }.get(); }

static std::string concatString(scm::args xs) {
  std::stringstream ss;
  static char buffer[0x1000];
  for (auto x : xs) {
    if (scm_is_string(x))
      ss << temp_string{ scm_to_utf8_string(x) }.get();
  }
  return ss.str();
}

static scm::val unwrapString(std::string str) {
  return scm_from_utf8_string(str.c_str());
}

extern "C" void mod_init() {
  scm_init_guile();
  // scm_catch_with_pre_unwind_handler()
  scm::type<std::string>("str").constructor(&makeString);
  scm::group("str").define("concat", &concatString);
  scm::group().define("unstr", &unwrapString);
  scm::group("log")
      .define("log", &doLog)
      .define("trace", &Slog<LogLevel::LOG_TRACE>)
      .define("info", &Slog<LogLevel::LOG_INFO>)
      .define("notice", &Slog<LogLevel::LOG_NOTICE>)
      .define("debug", &Slog<LogLevel::LOG_DEBUG>)
      .define("warn", &Slog<LogLevel::LOG_WARN>)
      .define("error", &Slog<LogLevel::LOG_ERROR>)
      .define("fatal", &Slog<LogLevel::LOG_FATAL>);
  chai.add(chaiscript::user_type<LogForChai>(), "LogTy");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_TRACE>), "trace");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_INFO>), "info");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_NOTICE>), "notice");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_DEBUG>), "debug");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_WARN>), "warn");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_ERROR>), "error");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_FATAL>), "fatal");
  chai.add_global_const(chaiscript::const_var(LogInstance), "Log");
  chai.add(chaiscript::bootstrap::standard_library::map_type<std::map<std::string, std::string>>("StringMap"));
}

extern "C" {
const char *mcpelauncher_property_get(const char *name, const char *def);
}

extern "C" void mod_exec() {
  if (exists("user/scm/init.scm")) scm_c_primitive_load("user/scm/init.scm");
  try {
    chai.use("init.chai");
    chai.eval_file("worlds/" + std::string(mcpelauncher_property_get("level-dir", "world")) + "/init.chai");
  } catch (const chaiscript::exception::eval_error &e) {
    auto print = e.pretty_print();
    Log::error("ChaiSupport", "EvalError: %s", print.c_str());
  } catch (const std::exception &e) { Log::error("ChaiSupport", "Failed to execute script: %s", e.what()); }
}