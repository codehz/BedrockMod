// Deps: out/script_world.so: out/script_base.so
#include "../base/main.h"

#include <api.h>

namespace scm {
template <> struct convertible<Block *> : foreign_object_is_convertible<Block *> {};
} // namespace scm

MAKE_FOREIGN_TYPE(Block *, "block");

struct CommandArea {
  BlockSource &getRegion();

  ~CommandArea();
};

struct CommandAreaFactory {
  Dimension *dim;
  CommandAreaFactory(Dimension &);

  std::unique_ptr<CommandArea> findArea(BlockPos const &, bool) const;
};

struct BlockTypeRegistry {
  static BlockLegacy *lookupByName(std::string const &);
};

enum ActorType : size_t {};

ActorType EntityTypeFromString(std::string const &);

struct ActorUniqueID {
  char filler[0xC];
  operator long() const;
};

struct CommandUtils {
  static bool spawnEntityAt(BlockSource &, Vec3 const &, ActorType, ActorUniqueID &, Actor *);
};

SCM_DEFINE_PUBLIC(get_block_at, "get-block@", 2, 0, 0, (scm::val<BlockPos> pos, scm::val<int> did), "Get Block") {
  auto dim = ServerCommand::mGame->getLevel().getDimension(DimensionId(did));
  CommandAreaFactory factory{ *dim };
  auto area = factory.findArea(pos, false);
  if (!area) return SCM_BOOL_F;
  auto &source = area->getRegion();
  return scm::to_scm(source.getBlock(pos));
}

SCM_DEFINE_PUBLIC(get_block_player_at, "player-get-block@", 2, 0, 0, (scm::val<BlockPos> pos, scm::val<ServerPlayer *> player), "Get Block") {
  auto &source = player->getRegion();
  return scm::to_scm(source.getBlock(pos));
}

SCM_DEFINE_PUBLIC(set_block_at, "set-block@", 3, 0, 0, (scm::val<BlockPos> pos, scm::val<int> did, scm::val<std::string> name), "Set block") {
  auto dim = ServerCommand::mGame->getLevel().getDimension(DimensionId(did));
  CommandAreaFactory factory{ *dim };
  auto area = factory.findArea(pos, false);
  if (!area) return SCM_BOOL_F;
  auto &source = area->getRegion();
  auto bl      = BlockTypeRegistry::lookupByName(name);
  if (!bl) return SCM_BOOL_F;
  source.setBlock(pos, *bl->getBlockStateFromLegacyData(0), 3, nullptr);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(set_block_player_at, "player-set-block@", 3, 0, 0,
                  (scm::val<BlockPos> pos, scm::val<ServerPlayer *> player, scm::val<std::string> name), "Set block") {
  auto &source = player->getRegion();
  auto bl      = BlockTypeRegistry::lookupByName(name);
  if (!bl) return SCM_BOOL_F;
  source.setBlock(pos, *bl->getBlockStateFromLegacyData(0), 3, nullptr);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(block_name, "block-name", 1, 0, 0, (scm::val<Block *> block), "Get Block Name") {
  return scm::to_scm(block->getLegacyBlock()->getFullName());
}

SCM_DEFINE_PUBLIC(spawn_actor, "spawn-actor", 3, 0, 0, (scm::val<Vec3> pos, scm::val<int> did, scm::val<std::string> name), "Spawn actor") {
  auto dim = ServerCommand::mGame->getLevel().getDimension(DimensionId(did));
  CommandAreaFactory factory{ *dim };
  auto area = factory.findArea(BlockPos(pos), false);
  if (!area) return SCM_BOOL_F;
  auto &source = area->getRegion();
  auto atype   = EntityTypeFromString(name);
  if (!atype) return SCM_BOOL_F;
  ActorUniqueID id;
  auto ret = CommandUtils::spawnEntityAt(source, pos, atype, id, nullptr);
  if (!ret) return SCM_BOOL_F;
  return scm::to_scm((long)id);
}

SCM_DEFINE_PUBLIC(player_spawn_actor, "player-spawn-actor", 3, 0, 0, (scm::val<Vec3> pos, scm::val<ServerPlayer *> player, scm::val<std::string> name), "Spawn actor") {
  auto &source = player->getRegion();
  auto atype   = EntityTypeFromString(name);
  if (!atype) return SCM_BOOL_F;
  ActorUniqueID id;
  auto ret = CommandUtils::spawnEntityAt(source, pos, atype, id, nullptr);
  if (!ret) return SCM_BOOL_F;
  return scm::to_scm((long)id);
}

PRELOAD_MODULE("minecraft world") {
#ifndef DIAG
#include "main.x"
#endif
}