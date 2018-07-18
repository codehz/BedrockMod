#include <polyfill.h>

#include <StaticHook.h>
#include <chaiscript/chaiscript.hpp>
#include <functional>
#include <hook.h>
#include <log.h>
#include <unordered_map>
#include <vector>

#include "player.h"

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
  m->add(chaiscript::fun(&Player::sendPacket), "sendPacket");
  m->add(chaiscript::fun([](ServerPlayer &player) -> std::string { return player.getNameTag(); }), "getNameTag");
  m->add(chaiscript::fun([](std::function<chaiscript::Boxed_Value(ServerPlayer &)> f) { onPlayerJoined(f); }), "onPlayerJoined");
  m->add(chaiscript::fun([](std::function<chaiscript::Boxed_Value(ServerPlayer &)> f) { onPlayerLeft(f); }), "onPlayerLeft");
  return m;
}