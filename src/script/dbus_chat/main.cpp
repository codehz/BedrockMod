#include <api.h>
#include <log.h>

#include <systemd/sd-bus.h>

extern "C" extern "C" sd_bus* mcpelauncher_get_dbus();

static sd_bus *bus = mcpelauncher_get_dbus();

std::function<void (std::string)> callback;

static int method_say(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  char *dat;
  sd_bus_message_read(m, "s", &dat);
  sd_bus_reply_method_return(m, "");
}

static const sd_bus_vtable chat_vtable[] = { SD_BUS_VTABLE_START(0), SD_BUS_METHOD("say", "s", "", &method_say, SD_BUS_VTABLE_UNPRIVILEGED),
                                             SD_BUS_SIGNAL("log", "ss", 0), SD_BUS_VTABLE_END };

void dbus_log(std::string sender, std::string message) {
  sd_bus_message *m = NULL;

  int r = sd_bus_message_new_signal(bus, &m, "/one/codehz/bedrockserver", "one.codehz.bedrockserver.chat", "log");
  if (r < 0) goto finish;
  sd_bus_message_append(m, "ss", sender.c_str(), message.c_str());
  r = sd_bus_send(bus, m, NULL);

finish:
  sd_bus_message_unrefp(&m);
  if (r < 0) Log::error("DBUS", "%s", strerror(-r));
}

void start_chat_bus(std::function<void (std::string)> _callback) {
  callback = _callback;
  sd_bus_slot *slot = NULL;

  int r = sd_bus_add_object_vtable(bus, &slot, "/", "bedrockserver.chat", chat_vtable, NULL);
  if (r < 0) Log::error("DBUS", "%s", strerror(-r));
}

extern "C" void mod_init() {
  Log::info("test", "size: %d", chat_vtable[0].x.start.element_size);
  // ModulePtr m(new Module());
  // m->add(fun(start_chat_bus), "dbus_chat_start");
  // m->add(fun(dbus_log), "dbus_chat_log");
  // loadModule(m);
}