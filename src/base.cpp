#include <api.h>

#include <StaticHook.h>
#include <functional>
#include <hook.h>
#include <log.h>
#include <unordered_map>
#include <vector>

#include "base.h"

struct TeleportCommand {
  void teleport(Entity &entity, Vec3 pos, Vec3 *center, DimensionId dim) const;
};

void teleport(Entity &entity, Vec3 target, int dim, Vec3 *center) {
  auto real = center != nullptr ? *center : target;
  ((TeleportCommand *)nullptr)->teleport(entity, target, &real, { dim });
}
void teleport1(Entity &entity, Vec3 target, int dim) { teleport(entity, target, dim, nullptr); }
void teleport2(Entity &entity, Vec3 target) { teleport(entity, target, entity.getDimensionId(), nullptr); }

struct Whitelist {};

namespace mce {
struct UUID {
  uint64_t most, least;
  const std::string asString() const;
  void fromString(std::string const &);
};
} // namespace mce

std::function<bool(std::string)> whitelistFilter;

TInstanceHook(bool, _ZNK9Whitelist9isAllowedERKN3mce4UUIDERKSs, Whitelist, mce::UUID &uuid, std::string const &msg) {
  if (whitelistFilter) try {
      return whitelistFilter(uuid.asString());
    } catch (std::exception const &e) { Log::error("Player", "Error: %s", e.what()); }
  return true;
}

struct Minecraft {
  void activateWhitelist();
  Level *getLevel() const;
};

Minecraft *mc;

struct ServerInstance {
  char filler[0x10];
  Minecraft *minecraft;
};

enum struct PlayerActionType { DESTROY, BUILD };

std::function<bool(ServerPlayer &sp, PlayerActionType, BlockPos const &)> playerAction;

TInstanceHook(int, _ZN8GameMode12destroyBlockERK8BlockPosa, GameMode, BlockPos const &pos, signed char flag) {
  try {
    if (!playerAction || playerAction(this->player, PlayerActionType::DESTROY, pos)) { return original(this, pos, flag); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "Error: %s", e.what());
    return original(this, pos, flag);
  }
}

TInstanceHook(int, _ZN12SurvivalMode12destroyBlockERK8BlockPosa, SurvivalMode, BlockPos const &pos, signed char flag) {
  try {
    if (!playerAction || playerAction(this->player, PlayerActionType::DESTROY, pos)) { return original(this, pos, flag); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "Error: %s", e.what());
    return original(this, pos, flag);
  }
}

TInstanceHook(int, _ZN8GameMode9useItemOnER12ItemInstanceRK8BlockPosaRK4Vec3P15ItemUseCallback, GameMode, ItemInstance &item, BlockPos const &pos,
              signed char flag, Vec3 const &ppos, ItemUseCallback *cb) {
  try {
    if (!playerAction || playerAction(this->player, PlayerActionType::BUILD, pos)) { return original(this, item, pos, flag, ppos, cb); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "Error: %s", e.what());
    return original(this, item, pos, flag, ppos, cb);
  }
}

TInstanceHook(int, _ZN12SurvivalMode9useItemOnER12ItemInstanceRK8BlockPosaRK4Vec3P15ItemUseCallback, SurvivalMode, ItemInstance &item,
              BlockPos const &pos, signed char flag, Vec3 const &ppos, ItemUseCallback *cb) {
  try {
    if (!playerAction || playerAction(this->player, PlayerActionType::BUILD, pos)) { return original(this, item, pos, flag, ppos, cb); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "Error: %s", e.what());
    return original(this, item, pos, flag, ppos, cb);
  }
}

extern "C" void mod_set_server(ServerInstance *si) {
  si->minecraft->activateWhitelist();
  mc = si->minecraft;
}

extern "C" void mod_init() {
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
  m->add(chaiscript::user_type<BlockPos>(), "BlockPos");
  m->add(chaiscript::user_type<Vec3>(), "Vec3");
  m->add(chaiscript::fun(&BlockPos::x), "x");
  m->add(chaiscript::fun(&BlockPos::y), "y");
  m->add(chaiscript::fun(&BlockPos::z), "z");
  m->add(chaiscript::constructor<BlockPos(int, int, int)>(), "BlockPos");
  m->add(chaiscript::constructor<BlockPos(Vec3 const &)>(), "BlockPos");
  m->add(chaiscript::fun(&Vec3::x), "x");
  m->add(chaiscript::fun(&Vec3::y), "y");
  m->add(chaiscript::fun(&Vec3::z), "z");
  m->add(chaiscript::constructor<Vec3(float, float, float)>(), "Vec3");
  m->add(chaiscript::constructor<Vec3(BlockPos const &)>(), "Vec3");
  m->add(chaiscript::fun([](std::function<bool(std::string)> const &fn) { whitelistFilter = fn; }), "setWhitelistFilter");
  m->add(chaiscript::fun(&ServerPlayer::sendNetworkPacket), "sendPacket");
  m->add(chaiscript::fun(&Entity::getPos), "getPos");
  m->add(chaiscript::fun(&Entity::getDimensionId), "getDim");
  m->add(chaiscript::fun(teleport), "teleport");
  m->add(chaiscript::fun(teleport1), "teleport");
  m->add(chaiscript::fun(teleport2), "teleport");
  m->add(chaiscript::fun([](ServerPlayer &player) -> std::string { return player.getNameTag(); }), "getNameTag");
  m->add(chaiscript::fun(onPlayerAdded), "onPlayerAdded");
  m->add(chaiscript::fun(onPlayerJoined), "onPlayerJoined");
  m->add(chaiscript::fun(onPlayerLeft), "onPlayerLeft");
  m->add(chaiscript::fun([](std::function<void(ServerPlayer & sp)> fn) {
           mc->getLevel()->forEachPlayer([&fn](Player &p) -> bool {
             try {
               fn(static_cast<ServerPlayer &>(p));
             } catch (std::exception const &e) {
               Log::error("BASE", "forEachPlayers: %s", e.what());
               return false;
             }
             return true;
           });
         }),
         "forEachPlayer");
  chaiscript::utility::add_class<PlayerActionType>(*m, "PlayerActionType",
                                                   { { PlayerActionType::DESTROY, "DESTROY" }, { PlayerActionType::BUILD, "BUILD" } });
  m->add(chaiscript::fun([](std::function<bool(ServerPlayer & sp, PlayerActionType, BlockPos const &)> fn) { playerAction = fn; }), "onPlayerAction");
  loadModule(m);
}