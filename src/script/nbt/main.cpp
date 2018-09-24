// Deps: out/script_nbt.so: out/script_base.so
#include "../base/main.h"

#include <StaticHook.h>
#include <api.h>
#include <tags.h>

#define GTYPE(t, name)                                                                                                                               \
  MAKE_FOREIGN_TYPE(t *, name)                                                                                                                       \
  namespace scm {                                                                                                                                    \
  template <> struct convertible<t *> : foreign_object_is_convertible<t *> {};                                                                       \
  }

GTYPE(CompoundTag, "nbt-compound");
GTYPE(ListTag, "nbt-list");
GTYPE(DoubleTag, "nbt-double");
GTYPE(ShortTag, "nbt-short");
GTYPE(Int64Tag, "nbt-int64");
GTYPE(FloatTag, "nbt-float");
GTYPE(IntTag, "nbt-int");
GTYPE(ByteTag, "nbt-byte");
GTYPE(IntArrayTag, "nbt-intarray");
GTYPE(ByteArrayTag, "nbt-bytearray");
GTYPE(StringTag, "nbt-string");

#undef GTYPE

namespace scm {
template <> struct convertible<Tag *> {
  static SCM to_scm(Tag *tag) {
#define CASE(T)                                                                                                                                      \
  if (auto target = dynamic_cast<T *>(tag); target) { return scm::to_scm(target); }
    CASE(CompoundTag);
    CASE(StringTag);
    CASE(ListTag);
    CASE(DoubleTag);
    CASE(ShortTag);
    CASE(Int64Tag);
    CASE(FloatTag);
    CASE(IntTag);
    CASE(ByteTag);
    CASE(IntArrayTag);
    CASE(ByteArrayTag);
#undef CASE
    return SCM_BOOL_F;
  }
  static Tag *from_scm(SCM scm) {
#define TEST(T)                                                                                                                                      \
  if (SCM_IS_A_P(scm, foreign_type_convertible<T *>::type())) return scm::from_scm<T *>(scm)
    TEST(CompoundTag);
    TEST(StringTag);
    TEST(ListTag);
    TEST(DoubleTag);
    TEST(ShortTag);
    TEST(Int64Tag);
    TEST(FloatTag);
    TEST(IntTag);
    TEST(ByteTag);
    TEST(IntArrayTag);
    TEST(ByteArrayTag);
#undef TEST
    scm_misc_error("Tag", "Not a Tag", scm::list(scm));
    return nullptr;
  }
};
} // namespace scm

TInstanceHook(std::string, _ZNK11CompoundTag8toStringB5cxx11Ev, CompoundTag) {
  std::stringstream ss;
  ss << "(Compound";
  for (auto &[k, v] : value) ss << " (" << k << " . " << v->toString() << ")";
  ss << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK7ListTag8toStringB5cxx11Ev, ListTag) {
  std::stringstream ss;
  ss << "(List";
  for (auto &v : value) ss << " " << v->toString();
  ss << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK9DoubleTag8toStringB5cxx11Ev, DoubleTag) {
  std::stringstream ss;
  ss << "(Double " << value << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK8ShortTag8toStringB5cxx11Ev, ShortTag) {
  std::stringstream ss;
  ss << "(Short " << value << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK8Int64Tag8toStringB5cxx11Ev, Int64Tag) {
  std::stringstream ss;
  ss << "(Int64 " << value << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK8FloatTag8toStringB5cxx11Ev, FloatTag) {
  std::stringstream ss;
  ss << "(Float " << value << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK6IntTag8toStringB5cxx11Ev, IntTag) {
  std::stringstream ss;
  ss << "(Int " << value << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK7ByteTag8toStringB5cxx11Ev, ByteTag) {
  std::stringstream ss;
  ss << "(Byte " << (int)value << ")";
  return ss.str();
}

TInstanceHook(std::string, _ZNK11IntArrayTag8toStringB5cxx11Ev, IntArrayTag) {
  std::stringstream ss;
  ss << "(IntArray)";
  return ss.str();
}

TInstanceHook(std::string, _ZNK12ByteArrayTag8toStringB5cxx11Ev, ByteArrayTag) {
  std::stringstream ss;
  ss << "(ByteArray)";
  return ss.str();
}

TInstanceHook(std::string, _ZNK9StringTag8toStringB5cxx11Ev, StringTag) {
  std::stringstream ss;
  ss << "\"";
  for (auto &ch : value) {
    switch (ch) {
    case '\\': ss << "\\\\"; break;
    case '"': ss << "\\\""; break;
    case '/': ss << "\\/"; break;
    case '\b': ss << "\\b"; break;
    case '\f': ss << "\\f"; break;
    case '\n': ss << "\\n"; break;
    case '\r': ss << "\\r"; break;
    case '\t': ss << "\\t"; break;
    default: ss << ch; break;
    }
  }
  ss << "\"";
  return ss.str();
}

SCM_DEFINE_PUBLIC(c_actor_nbt, "actor-nbt", 1, 0, 0, (scm::val<Actor *> act), "Actor's NBT") {
  CompoundTag *tag = new CompoundTag;
  if (!act->save(*tag)) { return SCM_BOOL_F; }
  return scm::to_scm(tag);
}

SCM_DEFINE_PUBLIC(c_nbt_close, "nbt-close", 1, 0, 0, (scm::val<Tag *> tag), "Close NBT instance") {
  delete tag.get();
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(c_with_nbt, "with-nbt", 2, 0, 0, (scm::val<Tag *> tag, scm::callback<SCM, Tag *> cb), "Auto managed NBT") {
  auto ret = cb(tag);
  delete tag;
  return ret;
}

SCM_DEFINE_PUBLIC(nbt_dump, "nbt-dump", 1, 0, 0, (scm::val<Tag *> tag), "Dump nbt") { return scm::to_scm(tag->toString()); }

SCM unbox(Tag *tag) {
#define CASE(T) if (auto target = dynamic_cast<T *>(tag); target)
  CASE(CompoundTag) {
    SCM list = SCM_EOL;
    for (auto &[k, v] : target->value) { list = scm_cons(scm_cons(scm::to_scm(k), unbox(v.get())), list); }
    return scm_reverse(list);
  }
  CASE(ListTag) {
    SCM list = SCM_EOL;
    for (auto &v : target->value) { list = scm_cons(unbox(v.get()), list); }
    return scm_reverse(list);
  }
  CASE(StringTag) { return scm::to_scm(target->value); }
  CASE(DoubleTag) { return scm::to_scm(target->value); }
  CASE(ShortTag) { return scm::to_scm(target->value); }
  CASE(Int64Tag) { return scm::to_scm(target->value); }
  CASE(FloatTag) { return scm::to_scm(target->value); }
  CASE(IntTag) { return scm::to_scm(target->value); }
  CASE(ByteTag) { return scm::to_scm(target->value); }
  CASE(IntArrayTag) {
    scm::vector<int> vec{ target->value.m_size / sizeof(int) };
    vec <<= [&](int *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return vec;
  }
  CASE(ByteArrayTag) {
    scm::vector<unsigned char> vec{ target->value.m_size / sizeof(unsigned char) };
    vec <<= [&](unsigned char *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return vec;
  }
#undef CASE
  return SCM_BOOL_F;
}

SCM_DEFINE_PUBLIC(c_nbt_unbox, "nbt-unbox-rec", 1, 0, 0, (scm::val<Tag *> tag), "Turn NBT to Scheme data(loss some type information)") {
  return unbox(tag);
}

SCM_DEFINE_PUBLIC(c_nbt_compound_keys, "nbt-compound-keys", 1, 0, 0, (scm::val<CompoundTag *> tag), "Get CompoundTag Keys") {
  SCM list = SCM_EOL;
  for (auto &[k, v] : tag->value) { list = scm_cons(scm::to_scm(k), list); }
  return scm_reverse(list);
}

SCM_DEFINE_PUBLIC(c_nbt_compound_ref, "nbt-compound-ref", 2, 0, 0, (scm::val<CompoundTag *> tag, scm::val<std::string> key), "Get CompoundTag ref") {
  return scm::to_scm(tag->value[key].get());
}

SCM_DEFINE_PUBLIC(c_nbt_shadow_unbox, "nbt-unbox", 1, 0, 0, (scm::val<Tag *> tag), "Unbox NBT") {
#define CASE(T) if (auto target = dynamic_cast<T *>(tag.get()); target)
  CASE(CompoundTag) {
    SCM list = SCM_EOL;
    for (auto &[k, v] : target->value) { list = scm_cons(scm_cons(scm::to_scm(k), scm::to_scm(v.get())), list); }
    return scm_reverse(list);
  }
  CASE(ListTag) {
    SCM list = SCM_EOL;
    for (auto &v : target->value) { list = scm_cons(scm::to_scm(v.get()), list); }
    return scm_reverse(list);
  }
  CASE(StringTag) { return scm::to_scm(target->value); }
  CASE(DoubleTag) { return scm::to_scm(target->value); }
  CASE(ShortTag) { return scm::to_scm(target->value); }
  CASE(Int64Tag) { return scm::to_scm(target->value); }
  CASE(FloatTag) { return scm::to_scm(target->value); }
  CASE(IntTag) { return scm::to_scm(target->value); }
  CASE(ByteTag) { return scm::to_scm(target->value); }
  CASE(IntArrayTag) {
    scm::vector<int> vec{ target->value.m_size / sizeof(int) };
    vec <<= [&](int *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return vec;
  }
  CASE(ByteArrayTag) {
    scm::vector<unsigned char> vec{ target->value.m_size / sizeof(unsigned char) };
    vec <<= [&](unsigned char *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return vec;
  }
#undef CASE
  return SCM_BOOL_F;
}

SCM_SYMBOL(compound_tag, "compound");
SCM_SYMBOL(list_tag, "list");
SCM_SYMBOL(string_tag, "string");
SCM_SYMBOL(double_tag, "double");
SCM_SYMBOL(short_tag, "short");
SCM_SYMBOL(int64_tag, "int64");
SCM_SYMBOL(float_tag, "float");
SCM_SYMBOL(int_tag, "int");
SCM_SYMBOL(byte_tag, "byte");
SCM_SYMBOL(intarray_tag, "intarray");
SCM_SYMBOL(bytearray_tag, "bytearray");

SCM unbox_with_tag(Tag *tag) {
#define CASE(T) if (auto target = dynamic_cast<T *>(tag); target)
  CASE(CompoundTag) {
    SCM list = SCM_EOL;
    for (auto &[k, v] : target->value) { list = scm_cons(scm_cons(scm::to_scm(k), unbox_with_tag(v.get())), list); }
    return scm_cons(compound_tag, scm_reverse(list));
  }
  CASE(ListTag) {
    SCM list = SCM_EOL;
    for (auto &v : target->value) { list = scm_cons(unbox_with_tag(v.get()), list); }
    return scm_cons(list_tag, scm_reverse(list));
  }
  CASE(StringTag) { return scm_cons(string_tag, scm::to_scm(target->value)); }
  CASE(DoubleTag) { return scm_cons(double_tag, scm::to_scm(target->value)); }
  CASE(ShortTag) { return scm_cons(short_tag, scm::to_scm(target->value)); }
  CASE(Int64Tag) { return scm_cons(int64_tag, scm::to_scm(target->value)); }
  CASE(FloatTag) { return scm_cons(float_tag, scm::to_scm(target->value)); }
  CASE(IntTag) { return scm_cons(int_tag, scm::to_scm(target->value)); }
  CASE(ByteTag) { return scm_cons(byte_tag, scm::to_scm(target->value)); }
  CASE(IntArrayTag) {
    scm::vector<int> vec{ target->value.m_size / sizeof(int) };
    vec <<= [&](int *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return scm_cons(intarray_tag, vec);
  }
  CASE(ByteArrayTag) {
    scm::vector<unsigned char> vec{ target->value.m_size / sizeof(unsigned char) };
    vec <<= [&](unsigned char *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return scm_cons(bytearray_tag, vec);
  }
#undef CASE
  return SCM_BOOL_F;
}

SCM_DEFINE_PUBLIC(c_nbt_unbox_with_tag, "nbt-unbox-rec/tag", 1, 0, 0, (scm::val<Tag *> tag), "Turn NBT to Scheme data(loss some type information)") {
  return unbox_with_tag(tag);
}

SCM_DEFINE_PUBLIC(c_nbt_shadow_unbox_tag, "nbt-unbox/tag", 1, 0, 0, (scm::val<Tag *> tag), "Unbox NBT") {
#define CASE(T) if (auto target = dynamic_cast<T *>(tag.get()); target)
  CASE(CompoundTag) {
    SCM list = SCM_EOL;
    for (auto &[k, v] : target->value) { list = scm_cons(scm_cons(scm::to_scm(k), scm::to_scm(v.get())), list); }
    return scm_cons(compound_tag, scm_reverse(list));
  }
  CASE(ListTag) {
    SCM list = SCM_EOL;
    for (auto &v : target->value) { list = scm_cons(scm::to_scm(v.get()), list); }
    return scm_cons(list_tag, scm_reverse(list));
  }
  CASE(StringTag) { return scm_cons(string_tag, scm::to_scm(target->value)); }
  CASE(DoubleTag) { return scm_cons(double_tag, scm::to_scm(target->value)); }
  CASE(ShortTag) { return scm_cons(short_tag, scm::to_scm(target->value)); }
  CASE(Int64Tag) { return scm_cons(int64_tag, scm::to_scm(target->value)); }
  CASE(FloatTag) { return scm_cons(float_tag, scm::to_scm(target->value)); }
  CASE(IntTag) { return scm_cons(int_tag, scm::to_scm(target->value)); }
  CASE(ByteTag) { return scm_cons(byte_tag, scm::to_scm(target->value)); }
  CASE(IntArrayTag) {
    scm::vector<int> vec{ target->value.m_size / sizeof(int) };
    vec <<= [&](int *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return scm_cons(intarray_tag, vec);
  }
  CASE(ByteArrayTag) {
    scm::vector<unsigned char> vec{ target->value.m_size / sizeof(unsigned char) };
    vec <<= [&](unsigned char *el, size_t l) { memccpy(el, target->value.data(), 1, target->value.m_size); };
    return scm_cons(bytearray_tag, vec);
  }
#undef CASE
  return SCM_BOOL_F;
}

PRELOAD_MODULE("minecraft nbt") {
#ifndef DIAG
#include "main.x"
#endif
}