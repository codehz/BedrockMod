#include <api.h>

#include <StaticHook.h>
#include <functional>
#include <hook.h>
#include <log.h>
#include <unordered_map>
#include <vector>

#include "base.h"

using namespace chaiscript;

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
  ModulePtr m(new Module());
  m->add(fun([]() -> BlockPos const & {
           if (!mc) throw std::runtime_error("Minecraft is not loaded");
           return mc->getLevel()->getDefaultSpawn();
         }),
         "getDefaultSpawn");
  m->add(fun([](BlockPos const &pos) {
           if (!mc) throw std::runtime_error("Minecraft is not loaded");
           mc->getLevel()->setDefaultSpawn(pos);
         }),
         "getDefaultSpawn");
  utility::add_class<Level>(*m, "Level", {},
                            { { fun(&Level::getDefaultSpawn), "getSpawnPoint" }, { fun(&Level::setDefaultSpawn), "setSpawnPoint" } });
  utility::add_class<Vec3>(*m, "Vec3", { constructor<Vec3(float, float, float)>(), constructor<Vec3(BlockPos const &)>() },
                           { { fun(&BlockPos::x), "x" }, { fun(&BlockPos::y), "y" }, { fun(&BlockPos::z), "z" } });
  utility::add_class<BlockPos>(*m, "BlockPos", { constructor<BlockPos(int, int, int)>(), constructor<BlockPos(Vec3 const &)>() },
                               { { fun(&BlockPos::x), "x" }, { fun(&BlockPos::y), "y" }, { fun(&BlockPos::z), "z" } });
  m->add(user_type<Entity>(), "Entity");
  m->add(user_type<Mob>(), "Mob");
  m->add(base_class<Entity, Mob>());
  m->add(user_type<Player>(), "Player");
  m->add(base_class<Entity, Player>());
  m->add(base_class<Mob, Player>());
  m->add(user_type<ServerPlayer>(), "ServerPlayer");
  m->add(base_class<Entity, ServerPlayer>());
  m->add(base_class<Mob, ServerPlayer>());
  m->add(base_class<Player, ServerPlayer>());
  m->add(fun([](std::function<bool(std::string)> const &fn) { whitelistFilter = fn; }), "setWhitelistFilter");
  m->add(fun(&ServerPlayer::sendNetworkPacket), "sendPacket");
  m->add(fun(&Entity::getPos), "getPos");
  m->add(fun(&Entity::getDimensionId), "getDim");
  m->add(fun(teleport), "teleport");
  m->add(fun(teleport1), "teleport");
  m->add(fun(teleport2), "teleport");
  m->add(fun([](ServerPlayer &player) -> std::string { return player.getNameTag(); }), "getNameTag");
  m->add(fun(onPlayerAdded), "onPlayerAdded");
  m->add(fun(onPlayerJoined), "onPlayerJoined");
  m->add(fun(onPlayerLeft), "onPlayerLeft");
  m->add(fun([](std::function<void(ServerPlayer & sp)> fn) {
           if (mc)
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
  utility::add_class<PlayerActionType>(*m, "PlayerActionType", { { PlayerActionType::DESTROY, "DESTROY" }, { PlayerActionType::BUILD, "BUILD" } });
  m->add(fun([](std::function<bool(ServerPlayer & sp, PlayerActionType, BlockPos const &)> fn) { playerAction = fn; }), "onPlayerAction");
  loadModule(m);
}