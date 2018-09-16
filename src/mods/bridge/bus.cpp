#include <chrono>
#include <condition_variable>
#include <functional>
#include <systemd/sd-bus.h>
#include <systemd/sd-journal.h>
#include <unistd.h>

#include <base.h>
#include <log.h>

#include <StaticHook.h>

extern ServerInstance *si __attribute__((visibility("hidden")));

extern "C" const char *bridge_version();
std::string execCommand(std::string line);
extern "C" void mcpelauncher_server_thread(std::function<void()>);
extern void handleStop(int sig);
static sd_bus *bus = nullptr;

extern "C" sd_bus *mcpelauncher_get_dbus() { return bus; }

extern "C" const char *(*mcpelauncher_exec_hook)(const char *);
const char *(*mcpelauncher_exec_hook)(const char *) = nullptr;

static int method_pong(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) { return sd_bus_reply_method_return(m, "s", bridge_version()); }

static int method_exec(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  char *dat;
  sd_bus_message_read(m, "s", &dat);
  auto custom = mcpelauncher_exec_hook ? mcpelauncher_exec_hook(dat) : nullptr;
  auto data   = custom ? custom : execCommand(dat);
  return sd_bus_reply_method_return(m, "s", data.c_str());
}

static int method_stop(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  handleStop(0);
  return sd_bus_reply_method_return(m, "");
}

std::vector<std::string> doComplete(std::string input, unsigned pos);

static int method_complete(sd_bus_message *call, void *userdata, sd_bus_error *ret_error) {
  char *dat;
  unsigned int pos;
  sd_bus_message_read(call, "su", &dat, pos);
  auto list = doComplete(dat, pos);
  sd_bus_message *m;
  sd_bus_message_new_method_return(call, &m);
  sd_bus_message_open_container(m, 'a', "s");
  for (auto const &item : list) { sd_bus_message_append(m, "s", item.c_str()); }
  sd_bus_message_close_container(m);
  auto ret = sd_bus_send(bus, m, nullptr);
  sd_bus_message_unrefp(&m);
  return ret;
}

static int method_list(sd_bus_message *call, void *userdata, sd_bus_error *ret_error) {
  sd_bus_message *m;
  sd_bus_message_new_method_return(call, &m);
  sd_bus_message_open_container(m, 'a', "(sss)");
  ServerCommand::mGame->getLevel().forEachPlayer([&](Player &p) -> bool {
    sd_bus_message_append(m, "(sss)", p.getNameTag().c_str(), p.getUUID().asString().c_str(), p.getXUID().c_str());
    return true;
  });
  sd_bus_message_close_container(m);
  auto ret = sd_bus_send(bus, m, nullptr);
  sd_bus_message_unrefp(&m);
  return ret;
}

static const sd_bus_vtable core_vtable[] = { SD_BUS_VTABLE_START(0),
                                             SD_BUS_METHOD("ping", "", "s", method_pong, SD_BUS_VTABLE_UNPRIVILEGED),
                                             SD_BUS_METHOD("exec", "s", "s", method_exec, SD_BUS_VTABLE_UNPRIVILEGED),
                                             SD_BUS_METHOD("stop", "", "", method_stop, SD_BUS_VTABLE_UNPRIVILEGED),
                                             SD_BUS_METHOD("complete", "su", "as", method_complete, SD_BUS_VTABLE_UNPRIVILEGED),
                                             SD_BUS_METHOD("list", "", "a(sss)", method_list, SD_BUS_VTABLE_UNPRIVILEGED),
                                             SD_BUS_SIGNAL("log", "yss", 0),
                                             SD_BUS_VTABLE_END };

static int priMap[] = { LOG_DEBUG, LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING, LOG_ERR, LOG_EMERG };

void dbus_log(int level, const char *tag, const char *data) {
  sd_journal_print(priMap[level], "[%s] %s", tag, data);
  if (strcmp(tag, "DBUS") == 0 || !bus) return;
  sd_bus_message *m = NULL;
  int r             = sd_bus_message_new_signal(bus, &m, "/one/codehz/bedrockserver", "one.codehz.bedrockserver.core", "log");
  if (r < 0) goto finish;
  sd_bus_message_append(m, "yss", (int8_t)level, tag, data);
  r = sd_bus_send(bus, m, NULL);
finish:
  sd_bus_message_unrefp(&m);
  if (r < 0) Log::error("DBUS", "%s", strerror(-r));
}

sd_bus_slot *slot = NULL;

void dbus_stop() {
  Log::info("DBUS", "Stoping...");
  bus = NULL;
  sd_bus_slot_unref(slot);
  sd_bus_unref(bus);
}

void dbus_init(char const *name) {
  int r = 0;
  r     = getenv("user_dbus") == nullptr ? sd_bus_open_system(&bus) : sd_bus_open_user(&bus);
  if (r < 0) goto dump;
  Log::info("DBUS", "Starting...");
  r = sd_bus_request_name(bus, name, 0);
  if (r < 0) goto finish;
  r = sd_bus_add_object_vtable(bus, &slot, "/one/codehz/bedrockserver", "one.codehz.bedrockserver.core", core_vtable, NULL);
  if (r < 0) goto finish;
  atexit(dbus_stop);
  return;
finish:
  Log::info("DBUS", "Stoping...");
  bus = NULL;
  sd_bus_slot_unref(slot);
  sd_bus_unref(bus);
dump:
  if (r < 0) { Log::error("DBUS", "%s", strerror(-r)); }
  exit(2);
}

static std::condition_variable cv;
static std::mutex mtx;

void dbus_loop() {
  pthread_setname_np(pthread_self(), "DBUS thread");
  int r      = 0;
  bool ready = false;
  while (true) {
    mcpelauncher_server_thread([&] {
      std::unique_lock<std::mutex> lk(mtx);
      r     = sd_bus_process(bus, NULL);
      ready = true;
      lk.unlock();
      cv.notify_one();
    });
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait_for(lk, std::chrono::seconds(1), [&] { return ready; });
    ready = false;
    if (r < 0) break;
    if (r > 0) continue;
    r = sd_bus_wait(bus, (uint64_t)-1);
    if (r < 0) break;
  }
  if (r < 0) { Log::error("DBUS", "%s", strerror(-r)); }
}