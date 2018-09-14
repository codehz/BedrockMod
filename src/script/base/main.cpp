#include <StaticHook.h>
#include <api.h>
#include <base.h>

#include "main.h"

SCM uuid_type;
SCM actor_type;
SCM player_type;

#ifndef DIAG
static_assert(sizeof(void *) == 8, "Only works in 64bit");
#endif

SCM_DEFINE_PUBLIC(c_make_uuid, "uuid", 1, 0, 0, (scm::val<std::string> name), "Create UUID") {
  std::string temp = name;
  return scm::to_scm(mce::UUID::fromStringFix(temp));
}

SCM_DEFINE_PUBLIC(c_uuid_to_string, "uuid->string", 1, 0, 0, (scm::val<mce::UUID> uuid), "UUID to string") {
  return scm::to_scm(uuid.get().asString());
}

SCM_DEFINE_PUBLIC(c_actor_name, "actor-name", 1, 0, 0, (scm::val<Actor *> act), "Return Actor's name") { return scm::to_scm(act->getNameTag()); }

SCM_DEFINE_PUBLIC(c_for_each_player, "for-each-player", 1, 0, 0, (scm::callback<bool, ServerPlayer *> callback), "Invoke function for each player") {
  support_get_minecraft()->getLevel().forEachPlayer([=](Player &p) { return callback((ServerPlayer *)&p); });
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(c_player_kick, "player-kick", 1, 0, 0, (scm::val<ServerPlayer *> player), "Kick player from server") {
  kickPlayer(player);
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(c_player_connection, "player-connection-info", 1, 0, 0, (scm::val<ServerPlayer *> player), "Get player connection info") {
  return scm::to_scm(player->getClientId().toString());
}

SCM_DEFINE_PUBLIC(c_player_permission_level, "player-permission-level", 1, 0, 0, (scm::val<ServerPlayer *> player),
                  "Get player's permission level.") {
  return scm::to_scm(player->getCommandPermissionLevel());
}

struct Whitelist {};

TInstanceHook(bool, _ZNK9Whitelist9isAllowedERKN3mce4UUIDERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, Whitelist, mce::UUID &uuid,
              std::string const &msg) {
  if (scm::sym(R"(%player-login)")) return scm::from_scm<bool>(scm::call(R"(%player-login)", uuid));
  return true;
}

LOADFILE(preload, "src/script/base/preload.scm");

PRELOAD_MODULE("minecraft base") {
  scm::sym_list uuid_slots = { "t0", "t1" };
  scm::sym_list data_slots = { "ptr" };

  uuid_type   = scm::foreign_type("uuid", uuid_slots, nullptr);
  actor_type  = scm::foreign_type("actor", data_slots, nullptr);
  player_type = scm::foreign_type("player", data_slots, nullptr);

  onPlayerJoined <<= [](ServerPlayer &player) {
    if (scm::sym(R"(%player-joined)")) scm::call(R"(%player-joined)", &player);
  };

  onPlayerLeft <<= [](ServerPlayer &player) {
    if (scm::sym(R"(%player-left)")) scm::call(R"(%player-left)", &player);
  };

#ifndef DIAG
#include "main.x"
#include "preload.scm.z"
#endif

  scm_c_eval_string(&file_preload_start);
}

extern "C" void mod_set_server(void *v) { support_get_minecraft()->activateWhitelist(); }
