#include <api.h>
#include <dirent.h>
#include <log.h>
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
  template <LogLevel Level> void log(std::string tag, std::string content) const { Log::log(Level, ("Chai::" + tag).c_str(), "%s", content.c_str()); }
};

static LogForChai LogInstance;

extern "C" void loadModule(chaiscript::ModulePtr ptr) { chai.add(ptr); }

extern "C" void mod_init() {
  chai.add(chaiscript::user_type<LogForChai>(), "LogTy");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_NOTICE>), "notice");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_INFO>), "info");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_DEBUG>), "debug");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_WARN>), "warn");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_ERROR>), "error");
  chai.add(chaiscript::fun(&LogForChai::log<LogLevel::LOG_FATAL>), "fatal");
  chai.add_global_const(chaiscript::const_var(LogInstance), "Log");
}

extern "C" void mod_exec() {
  try {
    chai.use("init.chai");
  } catch (const std::exception &e) { Log::error("ChaiSupport", "Failed to execute script: %s", e.what()); }
}