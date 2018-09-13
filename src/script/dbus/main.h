#include <api.h>
extern SCM sd_bus_vtable_type;

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
template <> struct convertible<sd_bus_vtable_list> {
  static SCM to_scm(sd_bus_vtable_list const &temp) { return scm_make_foreign_object_n(sd_bus_vtable_type, 3, (void **)(void *)&temp); }
  static sd_bus_vtable_list from_scm(SCM scm) {
    scm_assert_foreign_object_type(sd_bus_vtable_type, scm);
    return { (uint64_t)scm_foreign_object_ref(scm, 0), (uint64_t)scm_foreign_object_ref(scm, 1), (sd_bus_fill *)scm_foreign_object_ref(scm, 2) };
  }
  static void set_scm(SCM scm, sd_bus_vtable_list const &temp) {
    auto buffer = (void **)(void *)&temp;
    scm_foreign_object_set_x(scm, 0, buffer[0]);
    scm_foreign_object_set_x(scm, 1, buffer[1]);
    scm_foreign_object_set_x(scm, 2, buffer[2]);
  }
};
} // namespace scm