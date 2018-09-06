#include "main.h"
#include <api.h>
#include <base.h>

#include <condition_variable>
#include <sys/mman.h>

SCM sd_bus_vtable_type;

struct sd_bus;
struct sd_bus_slot {};

extern "C" extern "C" sd_bus *mcpelauncher_get_dbus();

SCM_DEFINE(c_get_dbus, "dbus-handle", 0, 0, 0, (), "Get dbus instance") { return scm_from_pointer(mcpelauncher_get_dbus(), nullptr); }

SCM_DEFINE(c_make_dbus_vtable, "make-dbus-vtable", 0, 0, 0, (), "Create new dbus vtable") {
  auto mem = (sd_bus_fill *)scm_gc_malloc(sizeof(sd_bus_fill) * 16, "dbus-slots");
  memset(mem, 0, sizeof(sd_bus_fill) * 16);
  new (&mem[0]) sd_bus_start(0);
  return scm::to_scm<sd_bus_vtable_list>({ 16, 1, mem });
}

template <typename R, typename... PS> struct FunctionWrapper {
  unsigned char push = 0x68;
  std::function<R(PS...)> *func;
  unsigned char call = 0xe8;
  unsigned offset    = ((char *)wrapper - (char *)this - offsetof(FunctionWrapper<int>, offset) - 4);
  unsigned addesp    = 0xc304c483;

  FunctionWrapper(std::function<R(PS...)> func)
      : func(new std::function<R(PS...)>(func)) {}

  auto as_pointer() { return (R(*)(PS...))this; }

  static auto wrapper(std::function<R(PS...)> &func, int s0, PS... ps) { return func(ps...); }
} __attribute__((packed));

void *alloc_executable_memory(size_t size) {
  void *ptr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void *)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

char *buffer = (char *)alloc_executable_memory(200 * sizeof(FunctionWrapper<int>));
size_t pos   = 0;

template <typename R, typename... PS> auto gen_function(std::function<R(PS...)> from) {
  return new (buffer + (sizeof(FunctionWrapper<int>) * pos++)) FunctionWrapper<R, PS...>(from);
}

static std::condition_variable cv;
static std::mutex mtx;

SCM_DEFINE_PUBLIC(c_define_dbus_method, "define-dbus-method", 6, 1, 0,
                  (scm::val<sd_bus_vtable_list> slots, scm::val<int64_t> flags, scm::val<gc_string> name, scm::val<gc_string> signature,
                   scm::val<gc_string> result, scm::callback<void, void *, void *, void *> handler, scm::val<int32_t> offset),
                  "Append method to dbus vtable") {
  int roffset = scm_is_integer(offset.scm) ? offset.get() : 0;
  std::function<int(void *, void *, void *)> f{ [=](void *msg, void *ud, void *err) {
    bool ready = false;
    mcpelauncher_server_thread <<= [&] {
      std::unique_lock<std::mutex> lck(mtx);

      handler(msg, ud, err);

      ready = true;
      cv.notify_all();
    };
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return ready; });
    return 0;
  } };
  slots <<= [&](sd_bus_vtable_list &n) {
    n.append<sd_bus_method>(flags, name.get(), signature.get(), result.get(), (void *)gen_function(f)->as_pointer(), roffset);
  };
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
  Log::debug("err", "%d %s", ((sd_bus_start &)vlist.list[0]).element_size, strerror(-r));
  return SCM_UNSPECIFIED;
}

LOADFILE(preload, "src/script/dbus/preload.scm");

PRELOAD_MODULE("minecraft dbus") {
  sd_bus_vtable_type = scm::foreign_type("dbus-vtable-list", { "cap", "size", "list" }, nullptr);
#ifndef DIAG
#include "main.x"
#include "preload.scm.z"
#endif
  scm_c_eval_string(&file_preload_start);
}
