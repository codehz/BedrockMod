#include <api.h>

#include <StaticHook.h>

const char *onLauncherExec(const char *command) {
  if (scm::sym(R"(%server-exec)")) {
    auto str = scm::call(R"(%server-exec)", command);
    if (scm_is_false(str)) return nullptr;
    return scm::from_scm<gc_string>(str);
  }
  return nullptr;
}

extern "C" {
extern const char *(*mcpelauncher_exec_hook)(const char *);
void mod_exec() { mcpelauncher_exec_hook = &onLauncherExec; }
}
