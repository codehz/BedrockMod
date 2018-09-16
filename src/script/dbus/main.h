#include <api.h>

struct sd_bus_vtable {
  uint8_t type : 8;
  uint64_t flags : 56;

protected:
  sd_bus_vtable(uint8_t type, uint64_t flags)
      : type(type)
      , flags(flags) {}
};

struct sd_bus_start : sd_bus_vtable {
  size_t element_size;
  sd_bus_start(uint64_t flags)
      : sd_bus_vtable('<', flags)
      , element_size(56) {}
};

struct sd_bus_end : sd_bus_vtable {
  sd_bus_end()
      : sd_bus_vtable('>', 0) {}
};

struct sd_bus_method : sd_bus_vtable {
  sd_bus_method(int64_t flags, const char *member, const char *signature, const char *result, void *handler, int32_t pos)
      : sd_bus_vtable('M', flags)
      , member(member)
      , signature(signature)
      , result(result)
      , handler(handler) {}
  const char *member, *signature, *result;
  void *handler;
  int32_t offset;
};

struct sd_bus_signal : sd_bus_vtable {
  sd_bus_signal(int64_t flags, const char *member, const char *signature)
      : sd_bus_vtable('S', flags)
      , member(member)
      , signature(signature) {}
  const char *member, *signature;
};

struct sd_bus_property : sd_bus_vtable {
  sd_bus_property(int64_t flags, const char *member, const char *signature, void *getter, int32_t offset)
      : sd_bus_vtable('P', flags)
      , member(member)
      , signature(signature)
      , getter(getter)
      , setter(nullptr)
      , offset(offset) {}
  sd_bus_property(int64_t flags, const char *member, const char *signature, void *getter, void *setter, int32_t offset)
      : sd_bus_vtable('W', flags)
      , member(member)
      , signature(signature)
      , getter(getter)
      , setter(setter)
      , offset(offset) {}
  const char *member, *signature;
  void *getter, *setter;
  int32_t offset;
};

struct sd_bus_fill : sd_bus_vtable {
  char filler[56 - sizeof(sd_bus_vtable)];
};

struct sd_bus_vtable_list {
  uint64_t cap;
  uint64_t size;
  sd_bus_fill *list;

  template <typename T, typename... P> void append(P... ps) {
    if (size == cap) {
      list = (sd_bus_fill *)scm_gc_realloc(list, sizeof(sd_bus_fill) * cap, sizeof(sd_bus_fill) * cap * 2, "Resize sd-bus slots");
      cap *= 2;
    }
    new (&list[size++]) T(ps...);
  }
};

namespace scm {
template <> struct convertible<sd_bus_vtable_list> : foreign_object_is_convertible<sd_bus_vtable_list> {
};
} // namespace scm