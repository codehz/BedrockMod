#include <api.h>

namespace scm {
template <> struct convertible<mce::UUID> : foreign_object_is_convertible<mce::UUID> {
};
template <> struct convertible<ServerPlayer *> : foreign_object_is_convertible<ServerPlayer *> {
};
template <> struct convertible<Actor *> : foreign_object_is_convertible<Actor *> {
  static Actor *from_scm(SCM act) {
    if (SCM_IS_A_P(act, convertible<ServerPlayer *>::ft::type())) { return (Actor *)foreign_object_is_convertible<ServerPlayer *>::from_scm(act); }
    return foreign_object_is_convertible<Actor *>::from_scm(act);
  }
};
} // namespace scm