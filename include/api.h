#pragma once

#include <base.h>
#include <functional>
#include <libguile.h>
#include <log.h>
#include <polyfill.h>

struct Minecraft;

extern "C" {
void mcpelauncher_exec_command(std::string cmd, std::function<void(std::string)> callback);
void mcpelauncher_server_thread(std::function<void()> callback);
const char *mcpelauncher_property_get(const char *name, const char *def);

Minecraft *support_get_minecraft();

void script_preload(std::function<void()>);
};

static auto init_guile_module(void (*fn)(void *), const char *name) {
  return [=]() { scm_c_define_module(name, fn, (void *)name); };
}

#define PRELOAD_MODULE(name)                                                                                                                         \
  static void init_module(void *);                                                                                                                   \
  extern "C" void mod_init() { script_preload(init_guile_module(init_module, name)); }                                                               \
  void init_module(void *)

struct temp_string {
  char *data;
  temp_string(char *data)
      : data(data) {}
  temp_string(temp_string &&rhs)
      : data(rhs.data) {
    rhs.data = nullptr;
  }
  void operator=(const temp_string &rhs) = delete;
  std::string get() const { return { data }; }
  operator char *() const { return data; }
  operator std::string() const { return { data }; }
  ~temp_string() {
    if (data) free(data);
  }
};

struct gc_string {
  char *data;
  gc_string(char *data) {
    this->data = (char *)scm_gc_malloc_pointerless(strlen(data) + 1, "string");
    strcpy(this->data, data);
  }
  operator char *() const { return data; }
};

template <typename T, typename F> decltype(auto) operator<<=(T t, F f) { return t(std::forward<F>(f)); }

namespace scm {

template <typename T, typename = void> struct convertible;

template <> struct convertible<void *> {
  static SCM to_scm(void *p) { return scm_from_pointer(p, nullptr); }

  static void *from_scm(SCM p) {
    SCM_ASSERT(SCM_POINTER_P(p), p, 1, "POINTER_FROM_SCM");
    return scm_to_pointer(p);
  }
};
template <> struct convertible<char *> {
  static SCM to_scm(const char *s) { return scm_from_utf8_string(s); }

  static temp_string from_scm(SCM str) {
    SCM_ASSERT(scm_is_string(str), str, 1, "STRING_FROM_SCM");
    return { scm_to_utf8_string(str) };
  }
};
template <> struct convertible<gc_string> {
  static SCM to_scm(gc_string s) { return scm_from_utf8_string(s.data); }

  static gc_string from_scm(SCM str) {
    SCM_ASSERT(scm_is_string(str), str, 1, "STRING_FROM_SCM");
    return { scm_to_utf8_string(str) };
  }
};

template <> struct convertible<SCM> {
  static SCM to_scm(SCM x) { return x; }
  static SCM from_scm(SCM x) { return x; }
};

template <> struct convertible<const char *> : convertible<char *> {};

template <> struct convertible<std::string> {
  static SCM to_scm(const std::string s) { return scm_from_utf8_string(s.c_str()); }
  static std::string from_scm(SCM s) { return convertible<char *>::from_scm(s); }
};

template <> struct convertible<bool> {
  static SCM to_scm(const bool b) { return b ? SCM_BOOL_T : SCM_BOOL_F; }
  static bool from_scm(SCM s) { return scm_is_true(s); }
};

#define cvt(c, s)                                                                                                                                    \
  template <> struct convertible<c> {                                                                                                                \
    static SCM to_scm(c v) { return scm_from_##s(v); }                                                                                               \
    static c from_scm(SCM v) { return scm_to_##s(v); }                                                                                               \
  }

cvt(float, double);
cvt(double, double);
cvt(int8_t, char);
cvt(int16_t, int16);
cvt(int32_t, int32);
cvt(int64_t, int64);
cvt(uint8_t, uchar);
cvt(uint16_t, uint16);
cvt(uint32_t, uint32);
cvt(uint64_t, uint64);

#undef cvt

template <typename T> auto to_scm(T s) { return convertible<std::remove_const_t<T>>::to_scm(s); }
template <typename T> auto from_scm(const SCM &s) { return convertible<std::remove_const_t<T>>::from_scm(s); }
template <typename T> void set_scm(const SCM &s, T v) { return convertible<std::remove_const_t<T>>::set_scm(s, v); }

template <typename T> struct val {
  SCM scm;

  auto get() { return std::move(from_scm<T>(scm)); }

  operator auto() { return std::move(from_scm<T>(scm)); }

  auto operator-> () { return from_scm<T>(scm); }

  void operator=(T t) { set_scm(scm, t); }

  void operator()(std::function<T(T)> access_fn) { set_scm(access_fn(from_scm<T>(scm))); }
  void operator()(std::function<void(T &)> modify_fn) {
    T temp = from_scm<T>(scm);
    modify_fn(temp);
    set_scm(scm, temp);
  }
};

struct as_sym {
  SCM scm;
  operator SCM() const { return scm; }
};

struct sym : as_sym {
  sym(const char *str) { scm = scm_from_utf8_symbol(str); }
  bool defined() const { return scm_is_true(scm_defined_p(scm, scm_current_module())); }
  operator bool() const { return defined(); }
};

SCM var(const char *name) { return scm_variable_ref(scm_c_lookup(name)); }

namespace {
// clang-format off
SCM scm_call_trait(SCM scm) { return scm_call_0(scm); }
SCM scm_call_trait(SCM scm, SCM v1) { return scm_call_1(scm, v1); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2) { return scm_call_2(scm, v1, v2); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2, SCM v3) { return scm_call_3(scm, v1, v2, v3); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2, SCM v3, SCM v4) { return scm_call_4(scm, v1, v2, v3, v4); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2, SCM v3, SCM v4, SCM v5) { return scm_call_5(scm, v1, v2, v3, v4, v5); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6) { return scm_call_6(scm, v1, v2, v3, v4, v5, v6); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6, SCM v7) { return scm_call_7(scm, v1, v2, v3, v4, v5, v6, v7); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6, SCM v7, SCM v8) { return scm_call_8(scm, v1, v2, v3, v4, v5, v6, v7, v8); }
SCM scm_call_trait(SCM scm, SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6, SCM v7, SCM v8, SCM v9) { return scm_call_9(scm, v1, v2, v3, v4, v5, v6, v7, v8, v9); }

SCM scm_list_trait() { return SCM_LIST0; }
SCM scm_list_trait(SCM v1) { return SCM_LIST1(v1); }
SCM scm_list_trait(SCM v1, SCM v2) { return SCM_LIST2(v1, v2); }
SCM scm_list_trait(SCM v1, SCM v2, SCM v3) { return SCM_LIST3(v1, v2, v3); }
SCM scm_list_trait(SCM v1, SCM v2, SCM v3, SCM v4) { return SCM_LIST4(v1, v2, v3, v4); }
SCM scm_list_trait(SCM v1, SCM v2, SCM v3, SCM v4, SCM v5) { return SCM_LIST5(v1, v2, v3, v4, v5); }
SCM scm_list_trait(SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6) { return SCM_LIST6(v1, v2, v3, v4, v5, v6); }
SCM scm_list_trait(SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6, SCM v7) { return SCM_LIST7(v1, v2, v3, v4, v5, v6, v7); }
SCM scm_list_trait(SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6, SCM v7, SCM v8) { return SCM_LIST8(v1, v2, v3, v4, v5, v6, v7, v8); }
SCM scm_list_trait(SCM v1, SCM v2, SCM v3, SCM v4, SCM v5, SCM v6, SCM v7, SCM v8, SCM v9) { return SCM_LIST9(v1, v2, v3, v4, v5, v6, v7, v8, v9); }
// clang-format on
} // namespace

template <typename... T> SCM call(const char *name, const T &... ts) { return scm_call_trait(var(name), to_scm(ts)...); }
template <typename... T> SCM call(SCM scm, const T &... ts) { return scm_call_trait(scm, to_scm(ts)...); }

template <typename R, typename... T> auto call_as(const char *name, const T &... ts) { return from_scm<R>(scm_call_trait(var(name), to_scm(ts)...)); }
template <typename R, typename... T> auto call_as(SCM scm, const T &... ts) { return from_scm<R>(scm_call_trait(scm, to_scm(ts)...)); }

template <typename R = void, typename... T> struct callback : as_sym {
  R operator()(T... t) const { return from_scm<R>(call(scm, t...)); }
  void setInvalid() { scm = SCM_BOOL_F; }
};

template <typename... T> struct callback<void, T...> : as_sym {
  void operator()(T... t) const { call(scm, t...); }
  void setInvalid() { scm = SCM_BOOL_F; }
};

struct list : as_sym {
  template <typename... T> list(T... t) { scm = scm_list_trait(to_scm(t)...); }

  auto operator[](int n) const { return scm_list_ref(scm, to_scm(n)); }
  template <typename T> T at(int n) const { return from_scm<T>(scm_list_ref(scm, to_scm(n))); }
};

struct sym_list : as_sym {
  template <typename... T> sym_list(T... t) { scm = scm_list_trait(scm_from_utf8_symbol(t)...); }
};

struct foreign_type : as_sym {
  foreign_type(std::string name, sym_list const &slots, scm_t_struct_finalize finalizer) {
    scm = scm_make_foreign_object_type(scm_from_utf8_symbol(name.c_str()), slots, finalizer);
    scm_c_module_define(scm_current_module(), ("<" + name + ">").c_str(), scm);
  }
};

struct definer {
  const char *name;
  definer(const char *name)
      : name(name) {}
  template <typename T, typename = decltype(convertible<T>::to_scm(T{}))> void operator=(const T &v) {
    scm_c_module_define(scm_current_module(), name, to_scm(v));
  }

  void operator=(const as_sym &v) { scm_c_module_define(scm_current_module(), name, v); }
};

} // namespace scm

struct Minecraft {
  void init(bool);
  void activateWhitelist();
  // ServerNetworkHandler *getServerNetworkHandler();
  Level *getLevel() const;
};

#define STR2(x) #x
#define STR(x) STR2(x)
// clang-format off
#define LOADFILE(name, file) \
    __asm__(".section .rodata\n" \
            ".global file_" STR(name) "_start\n" \
            ".type file_" STR(name) "_start, @object\n" \
            ".balign 16\n" \
            "file_" STR(name) "_start:\n" \
            ".incbin \"" file "\"\n" \
            \
            ".global file_" STR(name) "_end\n" \
            ".type file_" STR(name) "_end, @object\n" \
            ".balign 1\n" \
            "file_" STR(name) "_end:\n" \
            ".byte 0\n" \
    ); \
    extern const __attribute__((aligned(16))) char file_ ## name ## _start; \
    extern const char file_ ## name ## _end;
// clang-format on
