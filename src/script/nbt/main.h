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