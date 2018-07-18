#include <polyfill.h>

#include <StaticHook.h>
#include <chaiscript/chaiscript.hpp>
#include <functional>
#include <hook.h>
#include <log.h>
#include <unordered_map>
#include <vector>

#include "player.h"

struct TeleportCommand {
  void teleport(Entity &entity, Vec3 pos, Vec3 *center, DimensionId dim) const;
};

void teleport(Entity &entity, Vec3 target, int dim, Vec3 *center) {
  auto real = center != nullptr ? *center : target;
  ((TeleportCommand *)nullptr)->teleport(entity, target, &real, { dim });
}
void teleport1(Entity &entity, Vec3 target, int dim) { teleport(entity, target, dim, nullptr); }
void teleport2(Entity &entity, Vec3 target) { teleport(entity, target, entity.getDimensionId(), nullptr); }

CHAISCRIPT_MODULE_EXPORT chaiscript::ModulePtr create_chaiscript_module_player() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  m->add(chaiscript::user_type<Entity>(), "Entity");
  m->add(chaiscript::user_type<Mob>(), "Mob");
  m->add(chaiscript::base_class<Entity, Mob>());
  m->add(chaiscript::user_type<Player>(), "Player");
  m->add(chaiscript::base_class<Entity, Player>());
  m->add(chaiscript::base_class<Mob, Player>());
  m->add(chaiscript::user_type<ServerPlayer>(), "ServerPlayer");
  m->add(chaiscript::base_class<Entity, ServerPlayer>());
  m->add(chaiscript::base_class<Mob, ServerPlayer>());
  m->add(chaiscript::base_class<Player, ServerPlayer>());
  m->add(chaiscript::user_type<Vec3>(), "Vec3");
  m->add(chaiscript::constructor<Vec3(float, float, float)>(), "Vec3");
  m->add(chaiscript::fun(&ServerPlayer::sendNetworkPacket), "sendPacket");
  m->add(chaiscript::fun(&Entity::getPos), "getPos");
  m->add(chaiscript::fun(&Entity::getDimensionId), "getDim");
  m->add(chaiscript::fun(teleport), "teleport");
  m->add(chaiscript::fun(teleport1), "teleport");
  m->add(chaiscript::fun(teleport2), "teleport");
  m->add(chaiscript::fun([](ServerPlayer &player) -> std::string { return player.getNameTag(); }), "getNameTag");
  m->add(chaiscript::fun([](std::function<void(ServerPlayer &)> f) { onPlayerJoined(f); }), "onPlayerJoined");
  m->add(chaiscript::fun([](std::function<void(ServerPlayer &)> f) { onPlayerLeft(f); }), "onPlayerLeft");
  return m;
}