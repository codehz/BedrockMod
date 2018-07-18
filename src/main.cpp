#include <polyfill.h>

#include <chaiscript/chaiscript.hpp>
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

extern "C" void mod_init() {
  DIR *dir;
  dirent *ent;
  if ((dir = opendir("user/chai/modules")) != nullptr) {
    while ((ent = readdir(dir)) != nullptr) {
      std::string name = ent->d_name;
      std::string path = cwd + "/user/chai/modules/" + name;
      if (!hasPrefix(name, ".") && hasSuffix(name, ".mod")) {
        Log::info("ChaiSupport", "Load Module: %s", path.c_str());
        try {
          chai.load_module(name.substr(0, name.length() - 4), path);
        } catch (const chaiscript::exception::load_module_error &e) { Log::error("ChaiSupport", "Failed to load module: %s", e.what()); }
      }
    }
  } else {
    Log::warn("ChaiSupport", "Failed to open the modules directory %s", "user/chai/modules");
  }
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