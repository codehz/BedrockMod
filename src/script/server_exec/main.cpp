#include <api.h>

#include <StaticHook.h>

const char *onLauncherExec(const char *command) {
  if (scm::sym(R"(%server-exec)")) {
    auto str = scm::call_as<gc_string>(R"(%server-exec)", command);
    if (str.data[0] == 0) return nullptr;
    return str;
  }
  return nullptr;
}

extern "C" {
extern const char *(*mcpelauncher_exec_hook)(const char *);
void mod_exec() { mcpelauncher_exec_hook = &onLauncherExec; }
}
