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

void script_preload(void (*fn)(void *), const char *name);
};

#define PRELOAD_MODULE(name)                                                                                                                         \
  void init_module(void *);                                                                                                                          \
  extern "C" void mod_init() { script_preload(init_module, name); }                                                                                  \
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
  gc_string()
      : data(nullptr) {}
  gc_string(char *data) {
    if (data == nullptr) {
      this->data = nullptr;
      return;
    }
    this->data = (char *)scm_gc_malloc_pointerless(strlen(data) + 1, "string");
    strcpy(this->data, data);
  }
  operator char *() const { return data; }
};

template <typename T, typename F> auto operator<<=(T t, F f) -> decltype(t(f)) { return t(std::forward<F>(f)); }

namespace scm {

template <typename T, typename = void> struct convertible {
  static SCM to_scm(T t) { return t.scm; } // namespace scm
  static T from_scm(SCM scm) {
    T t;
    t.scm = scm;
    return t;
  }
};

template <typename T> struct foreign_type_convertible { static SCM &type(); };

#define MAKE_FOREIGN_TYPE(t, ...)                                                                                                                    \
  SCM_SNARF_HERE(template <> SCM & ::scm::foreign_type_convertible<t>::type() {                                                                      \
    static SCM value;                                                                                                                                \
    return value;                                                                                                                                    \
  };)                                                                                                                                                \
  SCM_SNARF_INIT(::scm::foreign_type<t>(__VA_ARGS__);)

template <typename T> struct foreign_object_is_convertible : foreign_type_convertible<T> {
  using ft = foreign_type_convertible<T>;
  static SCM to_scm(T const &t) { return scm_make_foreign_object_n(ft::type(), sizeof(T) / sizeof(void *), (void **)(void *)&t); }
  static T from_scm(SCM scm) {
    scm_assert_foreign_object_type(ft::type(), scm);
    return *(T *)SCM_STRUCT_DATA(scm);
  }
  static void set_scm(SCM scm, T const &temp) { *(T *)SCM_STRUCT_DATA(scm) = temp; }
};

template <typename T> struct foreign_object_is_convertible<T *> : foreign_type_convertible<T *> {
  using ft = foreign_type_convertible<T *>;
  static SCM to_scm(T *t) { return scm_make_foreign_object_1(ft::type(), t); }
  static T *from_scm(SCM scm) {
    scm_assert_foreign_object_type(ft::type(), scm);
    return (T *)scm_foreign_object_ref(scm, 0);
  }
};

template <typename T> struct convertible<T &> {
  static SCM to_scm(T &p) { return convertible<T *>::to_scm(&p); }
  static auto to_scm(SCM p) { return *convertible<T *>::from_scm(p); }
};

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
  static SCM to_scm(gc_string s) {
    if (s.data == nullptr) return SCM_BOOL_F;
    return scm_from_utf8_string(s.data);
  }

  static gc_string from_scm(SCM str) {
    if (scm_is_false(str)) return { nullptr };
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

struct as_scm {
  SCM scm;
  operator SCM() const { return scm; }
};

struct sym : as_scm {
  sym() = default;
  sym(const char *str) { scm = scm_from_utf8_symbol(str); }
  bool defined() const { return scm_is_true(scm_defined_p(scm, scm_current_module())); }
  operator bool() const { return defined(); }
  operator temp_string() const { return from_scm<char *>(scm_symbol_to_string(scm)); }
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

template <typename R = void, typename... T> struct callback : as_scm {
  auto operator()(T... t) const { return from_scm<R>(call(scm, t...)); }
  void setInvalid() { scm = SCM_BOOL_F; }
};

template <typename... T> struct callback<void, T...> : as_scm {
  void operator()(T... t) const { call(scm, t...); }
  void setInvalid() { scm = SCM_BOOL_F; }
};

template <typename X, typename Y> struct pair : as_scm {
  template <size_t I> auto get() {
    if constexpr (I == 0) {
      return from_scm<X>(scm_car(scm));
    } else if constexpr (I == 1) {
      return from_scm<Y>(scm_cdr(scm));
    }
  }
};

struct list : as_scm {
  template <typename... T> list(T... t) { scm = scm_list_trait(to_scm(t)...); }

  auto operator[](int n) const { return scm_list_ref(scm, to_scm(n)); }
  template <typename T> T at(int n) const { return from_scm<T>(scm_list_ref(scm, to_scm(n))); }

  struct iterator : as_scm {
    iterator(SCM scm) { this->scm = scm; }
    bool operator!=(iterator const &rhs) { return !SCM_NILP(scm); }
    void operator++() { scm = scm_cdr(scm); }
    SCM operator*() { return scm_car(scm); }
  };

  auto begin() { return iterator(scm); }
  auto end() { return iterator(SCM_EOL); }
};

template <typename T> struct slist : as_scm {
  struct iterator : as_scm {
    iterator(SCM scm) { this->scm = scm; }
    bool operator!=(iterator const &rhs) { return !scm_is_null_or_nil(scm); }
    void operator++() { scm = scm_cdr(scm); }
    T operator*() { return from_scm<T>(scm_car(scm)); }
  };

  auto begin() { return iterator(scm); }
  auto end() { return iterator(SCM_EOL); }
};

struct sym_list : as_scm {
  template <typename... T> sym_list(T... t) { scm = scm_list_trait(scm_from_utf8_symbol(t)...); }
};

template <typename T> struct foreign_type : as_scm {
  foreign_type(std::string name, sym_list const &slots, scm_t_struct_finalize finalizer = nullptr) {
    scm = scm_make_foreign_object_type(scm_from_utf8_symbol(name.c_str()), slots, finalizer);
    scm_c_module_define(scm_current_module(), ("<" + name + ">").c_str(), scm);
    scm::convertible<T>::type() = scm;
  }
};

template <typename T> struct foreign_type<T *> : as_scm {
  foreign_type(std::string name, scm_t_struct_finalize finalizer = nullptr) {
    scm = scm_make_foreign_object_type(scm_from_utf8_symbol(name.c_str()), sym_list{ "ptr" }, finalizer);
    scm_c_module_define(scm_current_module(), ("<" + name + "-ptr>").c_str(), scm);
    scm::convertible<T *>::type() = scm;
  }
};

struct definer {
  const char *name;
  definer(const char *name)
      : name(name) {}
  template <typename T, typename = decltype(convertible<T>::to_scm(T{}))> void operator=(const T &v) {
    scm_c_module_define(scm_current_module(), name, to_scm(v));
  }

  void operator=(const as_scm &v) { scm_c_module_define(scm_current_module(), name, v); }
};

template <typename... T> std::function<void(T...)> make_hook(const char *name) {
  auto hook = scm_make_hook(to_scm(sizeof...(T)));
  scm_c_module_define(scm_current_module(), name, hook);
  scm_c_export(name);
  return [=](T... ts) { scm_c_run_hook(hook, list(to_scm<T>(ts)...)); };
}

#define MAKE_HOOK(name, sname, ...)                                                                                                                  \
  SCM_SNARF_HERE(static std::function<void(__VA_ARGS__)> name;)                                                                                      \
  SCM_SNARF_INIT(name = (::scm::make_hook<__VA_ARGS__>(sname));)

template <typename T = SCM> struct fluid : as_scm {
  fluid() { scm = scm_make_fluid(); }
  fluid(T t) { scm = scm_make_fluid(scm::to_scm(t)); }

  struct accessor : as_scm {
    fluid<T> self;
    operator T() { return from_scm<T>(scm); }
    void operator=(T t) { scm_fluid_set_x(scm, to_scm(t)); }
  };
  accessor operator*() { return { scm_fluid_ref(scm), *this }; }
  auto operator*() const { return from_scm<T>(scm_fluid_ref(scm)); }
  auto operator-> () const { return from_scm<T>(scm_fluid_ref(scm)); }

  struct proxy : as_scm {
    fluid<T> self;
    template <typename F> T operator()(F f) {
      auto tp = std::make_tuple(f, self);
      return from_scm<T>(scm_c_with_fluid(self, scm, (SCM(*)(void *))handler<F>, &tp));
    }

    template <typename F> static SCM handler(std::tuple<F, fluid<T>> *tp) {
      auto [f, self] = *tp;
      f();
      return to_scm(*self);
    }
  };
  proxy operator[](T def) { return { to_scm(def), *this }; }
};

#define MAKE_FLUID(type, name, sname)                                                                                                                \
  SCM_SNARF_HERE(scm::fluid<type> &name() {                                                                                                          \
    static scm::fluid<type> value;                                                                                                                   \
    return value;                                                                                                                                    \
  })                                                                                                                                                 \
  SCM_SNARF_INIT(scm_c_define(sname, call("fluid->parameter", name())); scm_c_export(sname);)

template <typename T> struct vector;

#define IMPVEC(ctype, stype)                                                                                                                         \
  template <> struct vector<ctype> : as_scm {                                                                                                        \
    vector(size_t length) { scm = scm_make_##stype##vector(scm_from_size_t(length), to_scm(0)); }                                                    \
    vector(SCM scm) { scm = scm; }                                                                                                                   \
    size_t size() { return scm_to_size_t(scm_##stype##vector_length(scm)); }                                                                         \
    template <typename F> auto operator()(F f) {                                                                                                     \
      scm_t_array_handle h;                                                                                                                          \
      size_t len;                                                                                                                                    \
      ssize_t inc;                                                                                                                                   \
      ctype *vl = scm_##stype##vector_writable_elements(scm, &h, nullptr, nullptr);                                                                  \
      if constexpr (!std::is_same_v<void, decltype(f(vl, len))>) {                                                                                   \
        auto ret = f(vl, len);                                                                                                                       \
        scm_array_handle_release(&h);                                                                                                                \
        return ret;                                                                                                                                  \
      } else {                                                                                                                                       \
        f(vl, len);                                                                                                                                  \
        scm_array_handle_release(&h);                                                                                                                \
      }                                                                                                                                              \
    }                                                                                                                                                \
  }

// scm_make_f32vector

IMPVEC(int8_t, s8);
IMPVEC(uint8_t, u8);
IMPVEC(int16_t, s16);
IMPVEC(uint16_t, u16);
IMPVEC(int32_t, s32);
IMPVEC(uint32_t, u32);
IMPVEC(int64_t, s64);
IMPVEC(uint64_t, u64);
IMPVEC(float, f32);
IMPVEC(double, f64);

#undef IMPVEC

template <> struct convertible<Vec3> {
  static SCM to_scm(Vec3 v) {
    auto vec = vector<float>(3);
    vec([&](float *el, size_t l) { *((Vec3 *)el) = v; });
    return vec;
  }
  static Vec3 from_scm(SCM vec) {
    SCM_ASSERT_TYPE(scm_is_true(scm_f32vector_p(vec)) && scm_to_int(scm_f32vector_length(vec)) == 3, vec, 0, "c:f32vector->vec3",
                    "Cannot convert to vec3");
    return vector<float>(vec)([](float const *el, size_t l) { return *((Vec3 *)el); });
  }
};

template <> struct convertible<BlockPos> {
  static SCM to_scm(BlockPos v) {
    auto vec = vector<int>(3);
    vec([&](int *el, size_t l) { *((BlockPos *)el) = v; });
    return vec;
  }
  static BlockPos from_scm(SCM vec) {
    SCM_ASSERT_TYPE(scm_is_true(scm_f32vector_p(vec)) && scm_to_int(scm_f32vector_length(vec)) == 3, vec, 0, "c:f32vector->vec3",
                    "Cannot convert to vec3");
    return vector<int>(vec)([](int const *el, size_t l) { return *((BlockPos *)el); });
  }
};
} // namespace scm

namespace std {
template <typename A, typename B> struct tuple_size<scm::pair<A, B>> : std::integral_constant<size_t, 2> {};
template <typename A, typename B> struct tuple_element<0, scm::pair<A, B>> { using type = A; };
template <typename A, typename B> struct tuple_element<1, scm::pair<A, B>> { using type = B; };
} // namespace std

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
