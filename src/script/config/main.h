#include "INIReader.h"
#include <api.h>

extern SCM config_type;

namespace scm {
template <> struct convertible<INIReader *> {
  static SCM to_scm(INIReader *reader) { return scm_make_foreign_object_1(config_type, reader); }
  static INIReader *from_scm(SCM reader) { return (INIReader *)scm_foreign_object_ref(reader, 0); }
};
} // namespace scm