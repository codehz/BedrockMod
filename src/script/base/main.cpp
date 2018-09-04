#include <StaticHook.h>
#include <api.h>
#include <base.h>

SCM uuid_type;
SCM actor_type;
SCM player_type;

#ifndef DIAG
static_assert(sizeof(void *) == 4, "Only works in 32bit");
#endif

SCM_DEFINE(c_make_uuid, "uuid", 1, 0, 0, (scm::val<char *> name), "Create UUID") { return scm::to_scm(mce::UUID::fromString((temp_string)name)); }

SCM_DEFINE(c_uuid_to_string, "uuid->string", 1, 0, 0, (scm::val<mce::UUID> uuid), "UUID to string") { return scm::to_scm(uuid.get().asString()); }

SCM_DEFINE(c_actor_name, "actor-name", 1, 0, 0, (scm::val<Actor *> act), "Return Actor's name") { return scm::to_scm(act->getNameTag()); }

SCM_DEFINE(c_for_each_player, "for-each-player", 1, 0, 0, (scm::callback<bool, ServerPlayer *> callback), "Invoke function for each player") {
  support_get_minecraft()->getLevel()->forEachPlayer([=](Player &p) { return callback((ServerPlayer *)&p); });
  return SCM_UNSPECIFIED;
}

SCM_DEFINE(c_player_kick, "player-kick", 1, 0, 0, (scm::val<ServerPlayer *> player), "Kick player from server") {
  kickPlayer(player);
  return SCM_UNSPECIFIED;
}

SCM_DEFINE(c_player_permission_level, "player-permission-level", 1, 0, 0, (scm::val<ServerPlayer *> player), "Get player's permission level.") {
  return scm::to_scm(player->getCommandPermissionLevel());
}

struct Whitelist {};

TInstanceHook(bool, _ZNK9Whitelist9isAllowedERKN3mce4UUIDERKSs, Whitelist, mce::UUID &uuid, std::string const &msg) {
  if (scm::sym(R"(%player-login)")) return scm::from_scm<bool>(scm::call(R"(%player-login)", uuid));
  return true;
}

static void init_guile() {
  scm::sym sym_uuid{ "uuid" }, sym_actor{ "actor" }, sym_player{ "player" };
  SCM uuid_slots = SCM_LIST4(scm::sym("t0"), scm::sym("t1"), scm::sym("t2"), scm::sym("t3"));
  uuid_type      = scm_make_foreign_object_type(sym_uuid, uuid_slots, nullptr);
  scm_c_define("<uuid>", uuid_type);

  SCM data_slot = SCM_LIST1(scm::sym("ptr"));
  actor_type    = scm_make_foreign_object_type(sym_actor, data_slot, nullptr);
  player_type   = scm_make_foreign_object_type(sym_player, data_slot, nullptr);
  scm_c_define("<actor>", actor_type);
  scm_c_define("<player>", player_type);

  onPlayerAdded <<= [=](ServerPlayer &player) {
    if (scm::sym(R"(%player-added)")) scm::call(R"(%player-added)", &player);
  };

  onPlayerJoined <<= [=](ServerPlayer &player) {
    if (scm::sym(R"(%player-joined)")) scm::call(R"(%player-joined)", &player);
  };

  onPlayerLeft <<= [=](ServerPlayer &player) {
    if (scm::sym(R"(%player-left)")) scm::call(R"(%player-left)", &player);
  };

#ifndef DIAG
#include "main.x"
#endif
}

extern "C" void mod_init() { script_preload(init_guile); }

extern "C" void mod_set_server(void *v) { support_get_minecraft()->activateWhitelist(); }
