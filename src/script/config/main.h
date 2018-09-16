#include "INIReader.h"
#include <api.h>

namespace scm {
template <> struct convertible<INIReader *> : foreign_object_is_convertible<INIReader *> {};
} // namespace scm