#include <api.h>

#include <StaticHook.h>

MAKE_HOOK(server_exec, "server-exec", char const *)
MAKE_FLUID(gc_string, exec_result, "exec-result")

const char *onLauncherExec(const char *command) {
  auto ret = exec_result()[nullptr] <<= [=] { server_exec(command); };
  return ret;
}

extern "C" {
extern const char *(*mcpelauncher_exec_hook)(const char *);
}

PRELOAD_MODULE("minecraft chat") {
  mcpelauncher_exec_hook = onLauncherExec;
#ifndef DIAG
#include "main.x"
#endif
}
