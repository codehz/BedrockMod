#include "main.h"
#include <api.h>
#include <base.h>

#include <condition_variable>
#include <sys/mman.h>

struct sd_bus;
struct sd_bus_slot {};

extern "C" sd_bus *mcpelauncher_get_dbus();

SCM_DEFINE(c_get_dbus, "dbus-handle", 0, 0, 0, (), "Get dbus instance") { return scm_from_pointer(mcpelauncher_get_dbus(), nullptr); }

SCM_DEFINE(c_make_dbus_vtable, "make-dbus-vtable", 0, 0, 0, (), "Create new dbus vtable") {
  auto mem = (sd_bus_fill *)scm_gc_malloc(sizeof(sd_bus_fill) * 16, "dbus-slots");
  memset(mem, 0, sizeof(sd_bus_fill) * 16);
  new (&mem[0]) sd_bus_start(0);
  return scm::to_scm<sd_bus_vtable_list>({ 16, 1, mem });
}

SCM_DEFINE(c_define_dbus_method, "define-dbus-method-internal", 6, 0, 0,
           (scm::val<sd_bus_vtable_list> vt, scm::val<int64_t> flags, scm::val<gc_string> name, scm::val<gc_string> signature,
            scm::val<gc_string> result, scm::val<void *> handler),
           "Append method to dbus vtable") {
  vt <<= [&](sd_bus_vtable_list &n) { n.append<sd_bus_method>(flags, name.get(), signature.get(), result.get(), handler.get(), 0); };
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(c_define_dbus_signal, "define-dbus-signal", 4, 0, 0,
                  (scm::val<sd_bus_vtable_list> vt, scm::val<int64_t> flags, scm::val<gc_string> name, scm::val<gc_string> signature),
                  "Append signal to dbus vtable") {
  vt <<= [&](sd_bus_vtable_list &n) { n.append<sd_bus_signal>(flags, name.get(), signature.get()); };
  return SCM_UNSPECIFIED;
}

extern "C" {
int sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata);
int sd_bus_message_read_basic(void *m, char type, void *p);
};
template <char v> struct dbus_message_container_parent { static constexpr char value = v; };
template <char v> struct dbus_message_container {};
template <> struct dbus_message_container<'y'> : dbus_message_container_parent<'y'> { using type = unsigned char; };
template <> struct dbus_message_container<'b'> : dbus_message_container_parent<'b'> { using type = bool; };
template <> struct dbus_message_container<'n'> : dbus_message_container_parent<'n'> { using type = int16_t; };
template <> struct dbus_message_container<'q'> : dbus_message_container_parent<'q'> { using type = uint16_t; };
template <> struct dbus_message_container<'i'> : dbus_message_container_parent<'i'> { using type = int32_t; };
template <> struct dbus_message_container<'u'> : dbus_message_container_parent<'u'> { using type = uint32_t; };
template <> struct dbus_message_container<'x'> : dbus_message_container_parent<'x'> { using type = int64_t; };
template <> struct dbus_message_container<'t'> : dbus_message_container_parent<'t'> { using type = uint64_t; };
template <> struct dbus_message_container<'d'> : dbus_message_container_parent<'d'> { using type = double; };
template <> struct dbus_message_container<'s'> : dbus_message_container_parent<'s'> { using type = void *; };

template <char t> SCM read_dbus_message(void *msg) {
  typename dbus_message_container<t>::type buffer;
  sd_bus_message_read_basic(msg, t, &buffer);
  return scm::to_scm(buffer);
}

#define CASE(n)                                                                                                                                      \
  case n: return read_dbus_message<n>(msg)

SCM_DEFINE_PUBLIC(c_read_dbus_message, "dbus-read", 2, 0, 0, (scm::val<void *> msg, SCM type), "Read dbus message") {
  switch (scm_to_uint8(scm_char_to_integer(type))) {
    CASE('y');
    CASE('b');
    CASE('n');
    CASE('q');
    CASE('i');
    CASE('u');
    CASE('x');
    CASE('t');
    CASE('d');
    CASE('s');
  default: return SCM_UNSPECIFIED;
  }
}

#undef CASE

SCM_DEFINE(c_sd_bus_add_object_vtable, "add-obj-vtable", 3, 0, 0,
           (scm::val<const char *> path, scm::val<const char *> name, scm::val<sd_bus_vtable_list> vtable), "Systemd add object vtable") {
  sd_bus_slot *slot = NULL;
  auto vlist        = vtable.get();
  vlist.append<sd_bus_end>();
  auto r = sd_bus_add_object_vtable(mcpelauncher_get_dbus(), &slot, path.get(), name.get(), vlist.list, nullptr);
  return scm::to_scm(r);
}

LOADFILE(preload, "src/script/dbus/preload.scm");

PRELOAD_MODULE("minecraft dbus") {
  scm::foreign_type<sd_bus_vtable_list>("dbus-vtable-list", { "cap", "size", "list" }, nullptr);
#ifndef DIAG
#include "main.x"
#include "preload.scm.z"
#endif
  scm_c_eval_string(&file_preload_start);
}
