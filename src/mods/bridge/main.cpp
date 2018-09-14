#include <cstdlib>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

#include <StaticHook.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-journal.h>

#include "bus.h"

struct BedrockLog {
  static void log(uint area, uint level, char const *tag, int prip, char const *content, ...);
  static void _log_va(unsigned int, unsigned int, char const *, int, char const *, va_list);
  static void updateLogSetting(std::string const &, bool);
  static bool gLogFileCreated, gTrace;
};

TStaticHook(void, _ZN10BedrockLog16updateLogSettingERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEb, BedrockLog, std::string const &str,
            bool flag) {
  return;
}

char lvc[] = { 'T', 'D', 'I', 'N', 'W', 'E', 'F' };

THook(void, mcpelauncher_log, int level, char const *tag, char const *content) {
  dbus_log(level, tag, content);
  printf("%c [%s] %s\n", level[lvc], tag, content);
}

TStaticHook(void, _ZN10BedrockLog7_log_vaEjjPKciS1_P13__va_list_tag, BedrockLog, unsigned int a0, unsigned int a1, char const *s0, int i0,
            char const *s1, va_list tg) {
  char buffer[4096];
  vsnprintf(buffer, sizeof(buffer), s1, tg);
  auto len = strlen(buffer);
  if (len < 4095 && buffer[len - 1] != '\n') {
    buffer[len]     = '\n';
    buffer[len + 1] = 0;
  }
  dbus_log(a1 > 6 ? 6 : a1, s0, buffer);
  printf("M [%s] %s", s0, buffer);
}

extern "C" const char *bridge_version() { return "0.2.1"; }

struct DedicatedServer {
  void stop();
};

struct ServerInstance {
  void *vt, *filler;
  DedicatedServer *server;
};

extern ServerInstance *si __attribute__((visibility("hidden")));
static std::thread *dbus_thread;

std::string execCommand(std::string line);

__attribute__((visibility("hidden"))) void handleStop(int sig) { si->server->stop(); }

extern "C" void mod_set_server(ServerInstance *instance) {
  si          = instance;
  dbus_thread = new std::thread(dbus_loop);
}

__always_inline static std::string getProfile() {
  auto p = std::getenv("profile");
  return p ? std::string(p) : "";
}

const static std::string profile = getProfile();

extern "C" const char *mcpelauncher_get_profile() { return profile.c_str(); }

extern "C" void mod_init() {
  dbus_init(("one.codehz.bedrockserver." + profile).c_str());
  signal(SIGINT, handleStop);
}

TClasslessInstanceHook(void, _ZN18PropertiesSettingsC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, std::string const &name) {
  original(this, "profiles/" + profile + "/" + name);
}

TClasslessInstanceHook(void, _ZN13WhitelistFileC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, std::string const &name) {
  original(this, "profiles/" + profile + "/" + name);
}

TClasslessInstanceHook(void, _ZN15PermissionsFileC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, std::string const &name) {
  original(this, "profiles/" + profile + "/" + name);
}

TClasslessInstanceHook(std::string, _ZNK15FilePathManager13getWorldsPathB5cxx11Ev) { return "profiles/" + profile + "/worlds/"; }

// Disable console input
TClasslessInstanceHook(void, _ZN18ConsoleInputReaderC2Ev) {}
TClasslessInstanceHook(void, _ZN18ConsoleInputReaderD2Ev) {}
TClasslessInstanceHook(bool, _ZN18ConsoleInputReader7getLineERNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, std::string &str) {
  return false;
}

// Modify version
THook(std::string, _ZN6Common22getServerVersionStringB5cxx11Ev) { return original() + " modded " + bridge_version(); }