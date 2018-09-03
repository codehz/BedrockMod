#include <StaticHook.h>
#include <api.h>
#include <base.h>

static SCM uuid_type;
static SCM actor_type;
static SCM player_type;

#ifndef DIAG
static_assert(sizeof(void *) == 4, "Only works in 32bit");
#endif

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
template <> struct convertible<Actor*> {
  static SCM to_scm(Actor *temp) { return scm_make_foreign_object_1(actor_type, temp); }
  static Actor *from_scm(SCM act) {
    if (SCM_IS_A_P(act, player_type)) { return (Actor *)scm_foreign_object_ref(act, 0); }
    scm_assert_foreign_object_type(actor_type, act);
    return (Actor *)scm_foreign_object_ref(act, 0);
  }
};
template <> struct convertible<ServerPlayer *> {
  static SCM to_scm(ServerPlayer *temp) { return scm_make_foreign_object_1(actor_type, temp); }
  static ServerPlayer *from_scm(SCM act) {
    scm_assert_foreign_object_type(player_type, act);
    return (ServerPlayer *)scm_foreign_object_ref(act, 0);
  }
};
} // namespace scm

SCM_DEFINE(c_make_uuid, "uuid", 1, 0, 0, (scm::val<char *> name), "Create UUID") { return scm::to_scm(mce::UUID::fromString((temp_string)name)); }

SCM_DEFINE(c_uuid_to_string, "uuid->string", 1, 0, 0, (scm::val<mce::UUID> uuid), "UUID to string") {
  return scm::to_scm(uuid.get().asString());
}

SCM_DEFINE(c_actor_name, "actor-name", 1, 0, 0, (scm::val<Actor *> act), "Return Actor's name") { return scm::to_scm(act->getNameTag()); }

struct Whitelist {};

TInstanceHook(bool, _ZNK9Whitelist9isAllowedERKN3mce4UUIDERKSs, Whitelist, mce::UUID &uuid, std::string const &msg) {
  if (scm::sym(R"(%player-login)")) return scm::safe<bool> <<= [&] { return scm::call(R"(%player-login)", uuid); };
  return true;
}

static void init_guile() {
  scm::sym sym_uuid{ "uuid" }, sym_actor{ "actor" }, sym_player{ "player" };
  SCM uuid_slots = SCM_LIST4(scm::sym("t0"), scm::sym("t1"), scm::sym("t2"), scm::sym("t3"));
  uuid_type      = scm_make_foreign_object_type(sym_uuid, uuid_slots, nullptr);

  SCM data_slot = SCM_LIST1(scm::sym("ptr"));
  actor_type    = scm_make_foreign_object_type(sym_actor, data_slot, nullptr);
  player_type   = scm_make_foreign_object_type(sym_player, data_slot, nullptr);

  onPlayerAdded <<= [=](ServerPlayer &player) {
    if (scm::sym(R"(%player-added)")) scm::safe<void, void> <<= [&] { scm::call(R"(%player-added)", &player); };
  };

  onPlayerJoined <<= [=](ServerPlayer &player) {
    if (scm::sym(R"(%player-joined)")) scm::safe<void, void> <<= [&] { scm::call(R"(%player-joined)", &player); };
  };

  onPlayerLeft <<= [=](ServerPlayer &player) {
    if (scm::sym(R"(%player-left)")) scm::safe<void, void> <<= [&] { scm::call(R"(%player-left)", &player); };
  };

#ifndef DIAG
#include "main.x"
#endif
}

extern "C" void mod_init() { script_preload(init_guile); }

extern "C" void mod_set_server(void *v) { support_get_minecraft()->activateWhitelist(); }

extern "C" void mod_exec() {

  // if (scm::from_scm<bool>(scm_defined_p(scm_from_utf8_symbol(R"(%simple-hook)"), scm_current_module()))) {
  //   scm_call_1(scm_variable_ref(scm_c_lookup(R"(%simple-hook)")), scm::to_scm(mce::UUID::fromString("107d46e0-4d59-4e51-97ab-6585fe429d94")));
  // }
}