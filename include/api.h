#pragma once

#include <functional>
#include <libguile.h>
#include <log.h>
#include <polyfill.h>

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

static SCM catch_handler(void *handler_data, SCM tag, SCM args) {
  SCM p, stack, frame;

  p = scm_open_output_string();

  scm_simple_format(p, scm_from_utf8_string("~s: ~a"), scm_list_2(tag, args));

  temp_string err{ scm_to_utf8_string(scm_get_output_string(p)) };
  scm_close_output_port(p);
  Log::error("guile", "%s", err.data);
  return SCM_BOOL_F;
}

template <typename T, typename F, typename = std::enable_if_t<std::is_convertible<T, std::function<void(F)>>::value>>
decltype(auto) operator<<=(T t, F f) {
  return t(std::forward<F>(f));
}

namespace scm {

template <typename T, typename = void> struct convertible;

template <> struct convertible<char *> {
  static SCM to_scm(const char *s) { return scm_from_utf8_string(s); }

  static temp_string from_scm(SCM str) {
    SCM_ASSERT(scm_is_string(str), str, 1, "STRING_FROM_SCM");
    return { scm_to_utf8_string(str) };
  }
};

template <> struct convertible<const char *> : convertible<char *> {};

template <> struct convertible<std::string> {
  static SCM to_scm(const std::string s) { return scm_from_utf8_string(s.c_str()); }
  static std::string from_scm(SCM s) {
    return convertible<char*>::from_scm(s);
  }
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
cvt(int8_t, int8);
cvt(int16_t, int16);
cvt(int32_t, int32);
cvt(int64_t, int64);
cvt(uint8_t, uint8);
cvt(uint16_t, uint16);
cvt(uint32_t, uint32);
cvt(uint64_t, uint64);

#undef cvt

template <typename T> auto to_scm(T s) { return convertible<std::remove_const_t<T>>::to_scm(s); }
template <typename T> auto from_scm(const SCM &s) { return convertible<std::remove_const_t<T>>::from_scm(s); }

template <typename T> struct val {
  SCM scm;

  auto get() { return std::move(from_scm<T>(scm)); }

  operator auto() { return std::move(from_scm<T>(scm)); }
};

struct sym {
  SCM scm;
  sym(const char *str)
      : scm(scm_from_utf8_symbol(str)) {}
  operator SCM() { return scm; }
};

namespace {
template <typename R> R f_proxy(std::function<R()> &f) { return f(); }
template <typename R> SCM scm_proxy(std::function<R()> &f) { return to_scm(f()); }
SCM void_proxy(std::function<void()> &f) {
  f();
  return SCM_BOOL_F;
}
} // namespace

template <typename R = void> auto run(std::function<R()> f) -> R { return (R)scm_with_guile((void *(*)(void *))f_proxy<R>, &f); }

template <typename R = SCM> R safe(std::function<R()> f) {
  return from_scm<R>(scm_c_catch(SCM_BOOL_T, (SCM(*)(void *))scm_proxy<R>, &f, catch_handler, nullptr, nullptr, nullptr));
}
template <> SCM safe<SCM>(std::function<SCM()> f) {
  return scm_c_catch(SCM_BOOL_T, (SCM(*)(void *))f_proxy<SCM>, &f, catch_handler, nullptr, nullptr, nullptr);
}
template <> void safe<void>(std::function<void()> f) {
  scm_c_catch(SCM_BOOL_T, (SCM(*)(void *))void_proxy, &f, catch_handler, nullptr, nullptr, nullptr);
}

template <typename R = void> R run_safe(std::function<R()> f) {
  return run<R> <<= [f] { return safe<R> <<= f; };
}

struct definer {
  const char *name;
  definer(const char *name)
      : name(name) {}
  template <typename T, typename = decltype(convertible<T>::to_scm(T{}))> void operator=(const T &v) { scm_c_define(name, to_scm(v)); }
};

} // namespace scm

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