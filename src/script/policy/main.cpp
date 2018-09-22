// Deps: out/script_policy.so: out/script_base.so
#include "../base/main.h"
#include <api.h>

#include <StaticHook.h>

MAKE_FLUID(bool, policy_result, "policy-result");
MAKE_FLUID(ServerPlayer *, policy_self, "policy-self");

struct GameMode {
  void *vt;
  ServerPlayer *player;
  char filler[0xA8 - 2 * sizeof(void *)];

  template <typename F, typename... PS> bool queryPolicy(F f, PS... ps) {
    return scm::with_fluids{ policy_self() % player } <<= [&] { return policy_result()[true] <<= [&] { f(ps...); }; };
  }
};

MAKE_HOOK(player_attack, "policy-player-attack", Actor *);
TInstanceHook(bool, _ZN8GameMode6attackER5Actor, GameMode, Actor *target) {
  if (queryPolicy(player_attack, target)) { return original(this, target); }
  return false;
}

MAKE_HOOK(player_destroy, "policy-player-destroy", BlockPos);
TInstanceHook(bool, _ZN8GameMode12destroyBlockERK8BlockPosa, GameMode, BlockPos const &pos, signed char flag) {
  if (queryPolicy(player_destroy, pos)) { return original(this, pos, flag); }
  return false;
}

MAKE_HOOK(player_interact, "policy-player-interact", Actor *, Vec3);
TInstanceHook(bool, _ZN8GameMode8interactER5ActorRK4Vec3, GameMode, Actor *target, Vec3 const &vec) {
  if (queryPolicy(player_interact, target, vec)) { return original(this, target, vec); }
  return false;
}

MAKE_HOOK(player_use, "policy-player-use", ItemInstance *);
TInstanceHook(bool, _ZN8GameMode7useItemER12ItemInstance, GameMode, ItemInstance *instance) {
  if (queryPolicy(player_use, instance)) { return original(this, instance); }
  return false;
}

MAKE_HOOK(player_use_on, "policy-player-use-on", ItemInstance *, BlockPos, Vec3);
TInstanceHook(bool, _ZN8GameMode9useItemOnER12ItemInstanceRK8BlockPosaRK4Vec3P15ItemUseCallback, GameMode, ItemInstance *instance, BlockPos &pos,
              char flag, Vec3 &vec, void *callback) {
  if (queryPolicy(player_use_on, instance, pos, vec)) { return original(this, instance, pos, flag, vec, callback); }
  return false;
}

PRELOAD_MODULE("minecraft policy") {
#ifndef DIAG
#include "main.x"
#endif
}