#include <api.h>

extern SCM uuid_type;
extern SCM actor_type;
extern SCM player_type;

namespace scm {
template <> struct convertible<mce::UUID> {
  static SCM to_scm(mce::UUID const &temp) { return scm_make_foreign_object_n(uuid_type, 4, (void **)(void *)&temp); }
  static mce::UUID from_scm(SCM uuid) {
    scm_assert_foreign_object_type(uuid_type, uuid);
    void *auuid[] = { scm_foreign_object_ref(uuid, 0), scm_foreign_object_ref(uuid, 1), scm_foreign_object_ref(uuid, 2),
                      scm_foreign_object_ref(uuid, 3) };
    auto ruuid    = (mce::UUID *)auuid;
    return *ruuid;
  }
};
template <> struct convertible<Actor *> {
  static SCM to_scm(Actor *temp) { return scm_make_foreign_object_1(actor_type, temp); }
  static Actor *from_scm(SCM act) {
    if (SCM_IS_A_P(act, player_type)) { return (Actor *)scm_foreign_object_ref(act, 0); }
    scm_assert_foreign_object_type(actor_type, act);
    return (Actor *)scm_foreign_object_ref(act, 0);
  }
};
template <> struct convertible<ServerPlayer *> {
  static SCM to_scm(ServerPlayer *temp) { return scm_make_foreign_object_1(player_type, temp); }
  static ServerPlayer *from_scm(SCM act) {
    scm_assert_foreign_object_type(player_type, act);
    return (ServerPlayer *)scm_foreign_object_ref(act, 0);
  }
};
} // namespace scm