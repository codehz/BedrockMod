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

static std::string makeString(scm::val val) {
  if (scm_is_string(val)) {
    return temp_string{ scm_to_utf8_string(val) }.get();
  } else {
    return temp_string{ scm_to_utf8_string(scm_simple_format(SCM_BOOL_F, scm::val{ "~a" }, scm_list_1(val))) }.get();
  }
}

static std::string concatString(scm::args xs) {
  std::stringstream ss;
  for (auto x : xs) {
    if (scm_is_string(x))
      ss << temp_string{ scm_to_utf8_string(x) }.get();
    else
      ss << temp_string{ scm_to_utf8_string(scm_simple_format(SCM_BOOL_F, scm::val{ "~a" }, scm_list_1(x))) }.get();
  }
  return ss.str();
}

static std::string formatString(scm::val fmt, scm::args xs) {
  return temp_string{ scm_to_utf8_string(scm_simple_format(SCM_BOOL_F, fmt, xs)) }.get();
}

static scm::val unwrapString(std::string str) { return scm_from_utf8_string(str.c_str()); }

extern "C" void mod_init() {
  scm_init_guile();
  scm::type<std::string>("str").constructor(&makeString);
  scm::group("str").define("concat", &concatString).define("format", &formatString);
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
  scm_c_eval_string(R"((set! %load-hook (lambda (filename) (log-trace (str "loader") (str-format "Loading script ~a" filename)))))");
  scm_c_eval_string(R"((set! %load-path '("user/scm/modules" "user/scm/scripts")))");
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

static void handler_message(void *handler_data, SCM tag, SCM args) {
  SCM p, stack, frame;

  p     = scm_current_error_port();
  stack = scm_make_stack(SCM_BOOL_T, scm_list_1(scm_from_int(2)));
  frame = scm_is_true(stack) ? scm_stack_ref(stack, SCM_INUM0) : SCM_BOOL_F;

  scm_puts("Backtrace:\n", p);
  scm_display_backtrace_with_highlights(stack, p, SCM_BOOL_F, SCM_BOOL_F, SCM_EOL);
  scm_newline(p);

  scm_print_exception(p, frame, tag, args);
}

static SCM catch_handler(void *filename, scm::val key, scm::val xs) {
  handler_message(filename, key, xs);
  return SCM_BOOL_F;
}

extern "C" void mod_exec() {
  if (exists("user/scm/scripts/init.scm"))
    scm_c_catch(SCM_BOOL_T, (scm_t_catch_body)scm_c_primitive_load, (void *)"user/scm/scripts/init.scm", (scm_t_catch_handler)catch_handler,
                (void *)"user/scm/scripts/init.scm", nullptr, nullptr);
  try {
    chai.use("init.chai");
    chai.eval_file("worlds/" + std::string(mcpelauncher_property_get("level-dir", "world")) + "/init.chai");
  } catch (const chaiscript::exception::eval_error &e) {
    auto print = e.pretty_print();
    Log::error("ChaiSupport", "EvalError: %s", print.c_str());
  } catch (const std::exception &e) { Log::error("ChaiSupport", "Failed to execute script: %s", e.what()); }
}