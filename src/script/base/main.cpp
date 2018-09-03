#include <api.h>

#include <StaticHook.h>
#include <functional>
#include <hook.h>
#include <log.h>
#include <unordered_map>
#include <vector>

#include <base.h>

using namespace chaiscript;

struct TeleportCommand {
  void teleport(Actor &actor, Vec3 pos, Vec3 *center, DimensionId dim) const;
};

void teleport(Actor &actor, Vec3 target, int dim, Vec3 *center) {
  auto real = center != nullptr ? *center : target;
  ((TeleportCommand *)nullptr)->teleport(actor, target, &real, { dim });
}
void teleport1(Actor &actor, Vec3 target, int dim) { teleport(actor, target, dim, nullptr); }
void teleport2(Actor &actor, Vec3 target) { teleport(actor, target, actor.getDimensionId(), nullptr); }

struct Whitelist {};

std::function<bool(mce::UUID &)> whitelistFilter;

TInstanceHook(bool, _ZNK9Whitelist9isAllowedERKN3mce4UUIDERKSs, Whitelist, mce::UUID &uuid, std::string const &msg) {
  if (whitelistFilter) try {
      return whitelistFilter(uuid);
    } catch (std::exception const &e) { Log::error("Player", "%s", e.what()); }
  return true;
}

struct ServerNetworkHandler {
  void addToBlacklist(mce::UUID const &, std::string const &);
  void removeFromBlacklist(mce::UUID const &, std::string const &);
};

struct Minecraft {
  void init(bool);
  void activateWhitelist();
  ServerNetworkHandler *getServerNetworkHandler();
  Level *getLevel() const;
};

Minecraft *mc;

std::function<bool(ServerPlayer &sp, BlockPos const &)> playerDestroy;
std::function<bool(ServerPlayer &sp, ItemInstance &, BlockPos const &)> playerUseItem;
std::function<bool(Actor &e, Vec3 const &, float range)> actorExplode;
std::function<bool(ServerPlayer &sp, Actor &e, Vec3 const &)> playerInteract;
std::function<bool(ServerPlayer &sp, Actor &e)> playerAttack;
std::function<int(ServerPlayer &sp, std::string &ability)> checkAbility;

TInstanceHook(int, _ZN8GameMode12destroyBlockERK8BlockPosa, GameMode, BlockPos const &pos, signed char flag) {
  try {
    if (!playerDestroy || playerDestroy(this->player, pos)) { return original(this, pos, flag); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "Error: %s", e.what());
    return original(this, pos, flag);
  }
}

TInstanceHook(int, _ZN12SurvivalMode12destroyBlockERK8BlockPosa, SurvivalMode, BlockPos const &pos, signed char flag) {
  try {
    if (!playerDestroy || playerDestroy(this->player, pos)) { return original(this, pos, flag); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "Error: %s", e.what());
    return original(this, pos, flag);
  }
}

TInstanceHook(int, _ZN8GameMode9useItemOnER12ItemInstanceRK8BlockPosaRK4Vec3P15ItemUseCallback, GameMode, ItemInstance &item, BlockPos const &pos,
              signed char flag, Vec3 const &ppos, ItemUseCallback *cb) {
  try {
    if (!playerUseItem || playerUseItem(this->player, item, pos)) { return original(this, item, pos, flag, ppos, cb); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "%s", e.what());
    return original(this, item, pos, flag, ppos, cb);
  }
}

TInstanceHook(int, _ZN12SurvivalMode9useItemOnER12ItemInstanceRK8BlockPosaRK4Vec3P15ItemUseCallback, SurvivalMode, ItemInstance &item,
              BlockPos const &pos, signed char flag, Vec3 const &ppos, ItemUseCallback *cb) {
  try {
    if (!playerUseItem || playerUseItem(this->player, item, pos)) { return original(this, item, pos, flag, ppos, cb); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "%s", e.what());
    return original(this, item, pos, flag, ppos, cb);
  }
}

struct BlockSource;

TInstanceHook(void *, _ZN5Level7explodeER11BlockSourceP5ActorRK4Vec3fbbfb, Level, BlockSource &bs, Actor *actor, Vec3 const &pos, float range,
              bool flag1, bool flag2, float value, bool flag3) {
  try {
    if (!actorExplode || actorExplode(*actor, pos, range)) { return original(this, bs, actor, pos, range, flag1, flag2, value, flag3); }
    return nullptr;
  } catch (std::exception const &e) {
    Log::error("EntityExplode", "%s", e.what());
    return original(this, bs, actor, pos, range, flag1, flag2, value, flag3);
  }
}

TInstanceHook(int, _ZN8GameMode8interactER5ActorRK4Vec3, GameMode, Actor &ent, Vec3 const &pos) {
  try {
    if (!playerInteract || playerInteract(this->player, ent, pos)) { return original(this, ent, pos); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "%s", e.what());
    return original(this, ent, pos);
  }
}

TInstanceHook(int, _ZN8GameMode6attackER5Actor, GameMode, Actor &ent) {
  try {
    if (!playerAttack || playerAttack(this->player, ent)) { return original(this, ent); }
    return 0;
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "%s", e.what());
    return original(this, ent);
  }
}

TInstanceHook(bool, _ZN6Player13canUseAbilityERKSs, Player, std::string &abi) {
  try {
    if (checkAbility) {
      switch (checkAbility(*reinterpret_cast<ServerPlayer *>(this), abi)) {
      case 1: return true;
      case 0: return false;
      }
    }
    return original(this, abi);
  } catch (std::exception const &e) {
    Log::error("PlayerAction", "%s", e.what());
    return original(this, abi);
  }
}

struct ServerInstance;

extern "C" void mod_set_server(ServerInstance *si) {
  mc->activateWhitelist();
}

TInstanceHook(void, _ZN9Minecraft4initEb, Minecraft, bool v) {
  original(this, v);
  mc = this;
}

constexpr auto movsdoffset   = 38;
const static auto uuidoffset = *(short *)((char *)dlsym(MinecraftHandle(), "_ZN20MinecraftScreenModel18getLocalPlayerUUIDEv") + movsdoffset);

struct PlayerMap {
  std::unordered_map<mce::UUID, Boxed_Value> datas;
  Boxed_Value &operator[](ServerPlayer &player) { return datas[*(mce::UUID *)((char *)&player + uuidoffset)]; }
  bool contains(ServerPlayer &player) const { return datas.count(*(mce::UUID *)((char *)&player + uuidoffset)) != 0; }
  void erase(ServerPlayer &player) { datas.erase(*(mce::UUID *)((char *)&player + uuidoffset)); }
  void clear() { datas.clear(); }
  PlayerMap() {}
};

extern "C" void mod_init() {
  ModulePtr m(new Module());
  utility::add_class<PlayerMap>(*m, "PlayerMap", { constructor<PlayerMap()>() },
                                { { fun(&PlayerMap::operator[]), "[]" },
                                  { fun(&PlayerMap::contains), "contains" },
                                  { fun(&PlayerMap::erase), "erase" },
                                  { fun(&PlayerMap::clear), "clear" } });
  utility::add_class<Item>(
      *m, "Item", {},
      { { fun(&ItemRegistry::getItem), "getItem" }, { fun(&ItemRegistry::findItem), "findItem" }, { fun(&Item::getId), "getId" }, { fun(&Item::operator==), "==" } });
  m->add(fun([]() -> BlockPos const & {
           if (!mc) throw std::runtime_error("Minecraft is not loaded");
           return mc->getLevel()->getDefaultSpawn();
         }),
         "getDefaultSpawn");
  m->add(fun([](BlockPos const &pos) {
           if (!mc) throw std::runtime_error("Minecraft is not loaded");
           mc->getLevel()->setDefaultSpawn(pos);
         }),
         "setDefaultSpawn");
  utility::add_class<mce::UUID>(
      *m, "UUID", { constructor<mce::UUID(const mce::UUID &)>() },
      { { fun(&mce::UUID::asString), "to_string" }, { fun(&mce::UUID::operator==), "==" }, { fun(&mce::UUID::operator!=), "!=" } });
  utility::add_class<Vec3>(*m, "Vec3",
                           { constructor<Vec3(float, float, float)>(), constructor<Vec3(BlockPos const &)>(), constructor<Vec3(Vec3 const &)>() },
                           { { fun(&Vec3::x), "x" }, { fun(&Vec3::y), "y" }, { fun(&Vec3::z), "z" } });
  utility::add_class<BlockPos>(
      *m, "BlockPos", { constructor<BlockPos(int, int, int)>(), constructor<BlockPos(Vec3 const &)>(), constructor<BlockPos(BlockPos const &)>() },
      { { fun(&BlockPos::x), "x" }, { fun(&BlockPos::y), "y" }, { fun(&BlockPos::z), "z" } });
  m->add(user_type<Actor>(), "Actor");
  m->add(user_type<Mob>(), "Mob");
  m->add(base_class<Actor, Mob>());
  m->add(user_type<Player>(), "Player");
  m->add(base_class<Actor, Player>());
  m->add(base_class<Mob, Player>());
  m->add(user_type<ServerPlayer>(), "ServerPlayer");
  m->add(base_class<Actor, ServerPlayer>());
  m->add(base_class<Mob, ServerPlayer>());
  m->add(base_class<Player, ServerPlayer>());
  m->add(fun(&mce::UUID::fromString), "UUID");
  m->add(fun([](std::function<bool(mce::UUID &)> const &fn) { whitelistFilter = fn; }), "setWhitelistFilter");
  m->add(fun(&ServerPlayer::sendNetworkPacket), "sendPacket");
  m->add(fun(&Actor::getPos), "getPos");
  m->add(fun(&Actor::getDimensionId), "getDim");
  m->add(fun(&Player::getSpawnPosition), "getSpawnPos");
  m->add(fun(teleport), "teleport");
  m->add(fun(teleport1), "teleport");
  m->add(fun(teleport2), "teleport");
  m->add(fun([](Actor &actor) -> std::vector<std::string> {
           std::vector<std::string> temp;
           actor.getDebugText(temp);
           //  for (auto item : temp) { Log::trace("Actor", "::%s", item.c_str()); }
           return temp;
         }),
         "getDebugText");
  m->add(fun([](Actor &actor) -> std::string { return actor.getNameTag(); }), "getNameTag");
  m->add(fun(&Player::getCommandPermissionLevel), "getPermissionLevel");
  m->add(fun(onPlayerAdded), "onPlayerAdded");
  m->add(fun(onPlayerJoined), "onPlayerJoined");
  m->add(fun(onPlayerLeft), "onPlayerLeft");
  m->add(fun([](mce::UUID uuid) {
           Log::trace("getPlayer", "mc: %08x", mc);
           if (!mc) throw std::runtime_error("Minecraft is not loaded");
           Log::trace("getPlayer", "Level: %08x", mc->getLevel());
           return mc->getLevel()->getPlayer(uuid);
         }),
         "getPlayer");
  m->add(fun([](std::string const &name) {
           if (!mc) throw std::runtime_error("Minecraft is not loaded");
           return mc->getLevel()->getPlayer(name);
         }),
         "getPlayer");
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
  utility::add_class<ItemInstance>(*m, "ItemInstance", {},
                                   { { fun(&ItemInstance::getName), "getName" },
                                     { fun(&ItemInstance::getCustomName), "getCustomName" },
                                     { fun(&ItemInstance::isNull), "isNull" },
                                     { fun(&ItemInstance::getId), "getId" } });
  m->add(fun([](decltype(playerDestroy) fn) { playerDestroy = fn; }), "onPlayerDestroy");
  m->add(fun([](decltype(playerUseItem) fn) { playerUseItem = fn; }), "onPlayerUseItem");
  m->add(fun([](decltype(actorExplode) fn) { actorExplode = fn; }), "onActorExplode");
  m->add(fun([](decltype(playerInteract) fn) { playerInteract = fn; }), "onPlayerInteract");
  m->add(fun([](decltype(playerAttack) fn) { playerAttack = fn; }), "onPlayerAttack");
  m->add(fun([](decltype(checkAbility) fn) { checkAbility = fn; }), "onCheckAbility");
  m->add(fun([](Player &player) -> mce::UUID { return *(mce::UUID *)((char *)&player + uuidoffset); }), "getUUID");
  m->add(fun(&kickPlayer), "kick");
  m->add(fun([](mce::UUID const &uuid, std::string const &reason) { mc->getServerNetworkHandler()->addToBlacklist(uuid, reason); }),
         "addToBlacklist");
  m->add(fun([](mce::UUID const &uuid, std::string const &reason) { mc->getServerNetworkHandler()->removeFromBlacklist(uuid, reason); }),
         "removeFromBlacklist");

  getChai().add(bootstrap::standard_library::vector_type<std::vector<ServerPlayer *>>("PlayerList"));
  getChai().add(bootstrap::standard_library::vector_type<std::vector<std::string>>("StringList"));

  loadModule(m);
}