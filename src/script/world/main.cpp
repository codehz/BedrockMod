// Deps: out/script_world.so: out/script_base.so out/script_nbt.so
#include "../base/main.h"
#include "../nbt/main.h"

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

struct BedrockBlocks {
  static Block *mAir;
};

SCM_DEFINE_PUBLIC(get_block_at, "get-block", 2, 0, 0, (scm::val<BlockPos> pos, scm::val<int> did), "Get Block") {
  auto dim = ServerCommand::mGame->getLevel().getDimension(DimensionId(did));
  if (!dim) return SCM_BOOL_F;
  CommandAreaFactory factory{ *dim };
  auto area = factory.findArea(pos, false);
  if (!area) return SCM_BOOL_F;
  auto &source = area->getRegion();
  return scm::to_scm(source.getBlock(pos));
}

SCM_DEFINE_PUBLIC(get_block_player_at, "get-block/player", 2, 0, 0, (scm::val<BlockPos> pos, scm::val<ServerPlayer *> player), "Get Block") {
  auto &source = player->getRegion();
  return scm::to_scm(source.getBlock(pos));
}

SCM_DEFINE_PUBLIC(set_block_at, "set-block", 3, 0, 0, (scm::val<BlockPos> pos, scm::val<int> did, scm::val<std::string> name), "Set block") {
  auto dim = ServerCommand::mGame->getLevel().getDimension(DimensionId(did));
  if (!dim) return SCM_BOOL_F;
  CommandAreaFactory factory{ *dim };
  auto area = factory.findArea(pos, false);
  if (!area) return SCM_BOOL_F;
  auto &source = area->getRegion();
  auto bl      = BlockTypeRegistry::lookupByName(name);
  if (!bl) return SCM_BOOL_F;
  source.setBlock(pos, *bl->getBlockStateFromLegacyData(0), 3, nullptr);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(set_block_player_at, "set-block/player", 3, 0, 0,
                  (scm::val<BlockPos> pos, scm::val<ServerPlayer *> player, scm::val<std::string> name), "Set block") {
  auto &source = player->getRegion();
  auto bl      = BlockTypeRegistry::lookupByName(name);
  if (!bl) return SCM_BOOL_F;
  source.setBlock(pos, *bl->getBlockStateFromLegacyData(0), 3, nullptr);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(clear_block_at, "clear-block", 2, 0, 0, (scm::val<BlockPos> pos, scm::val<int> did), "Clear block") {
  auto dim = ServerCommand::mGame->getLevel().getDimension(DimensionId(did));
  if (!dim) return SCM_BOOL_F;
  CommandAreaFactory factory{ *dim };
  auto area = factory.findArea(pos, false);
  if (!area) return SCM_BOOL_F;
  auto &source = area->getRegion();
  source.setBlock(pos, *BedrockBlocks::mAir, 3, nullptr);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(clear_block_player_at, "clear-block/player", 2, 0, 0, (scm::val<BlockPos> pos, scm::val<ServerPlayer *> player), "Clear block") {
  auto &source = player->getRegion();
  source.setBlock(pos, *BedrockBlocks::mAir, 3, nullptr);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(block_name, "block-name", 1, 0, 0, (scm::val<Block *> block), "Get Block Name") {
  return scm::to_scm(block->getLegacyBlock()->getFullName());
}

SCM_DEFINE_PUBLIC(spawn_actor, "spawn-actor", 3, 0, 0, (scm::val<Vec3> pos, scm::val<int> did, scm::val<std::string> name), "Spawn actor") {
  auto dim = ServerCommand::mGame->getLevel().getDimension(DimensionId(did));
  if (!dim) return SCM_BOOL_F;
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

SCM_DEFINE_PUBLIC(player_spawn_actor, "spawn-actor/player", 3, 0, 0,
                  (scm::val<Vec3> pos, scm::val<ServerPlayer *> player, scm::val<std::string> name), "Spawn actor") {
  auto &source = player->getRegion();
  auto atype   = EntityTypeFromString(name);
  if (!atype) return SCM_BOOL_F;
  ActorUniqueID id;
  auto ret = CommandUtils::spawnEntityAt(source, pos, atype, id, nullptr);
  if (!ret) return SCM_BOOL_F;
  return scm::to_scm((long)id);
}

SCM_DEFINE_PUBLIC(create_item_instance, "create-item-instance", 2, 2, 0,
                  (scm::val<std::string> name, scm::val<int> number, scm::val<int> aux, scm::val<CompoundTag *> tag), "Create ItemInstance") {
  auto item = ItemRegistry::lookupByName(name, true);
  if (!item) return SCM_BOOL_F;
  return scm::to_scm(new ItemInstance(*item, number, aux[0], tag[nullptr]));
}

SCM_DEFINE_PUBLIC(with_item_instance, "with-item-instance", 2, 0, 0, (scm::val<ItemInstance *> ins, scm::callback<SCM, ItemInstance *> cb),
                  "Managed ItemInstance") {
  if (scm_is_false(ins.scm)) return SCM_BOOL_F;
  scm::dynwind dyn;
  auto instance = dyn(ins.get());
  auto ret      = cb(ins);
  return ret;
}

SCM_DEFINE_PUBLIC(spawn_item, "spawn-item", 3, 0, 0, (scm::val<Vec3> pos, scm::val<int> did, scm::val<ItemInstance *> item), "Spawn actor") {
  auto &level   = ServerCommand::mGame->getLevel();
  auto &spawner = level.getSpawner();
  auto dim      = level.getDimension(DimensionId(did));
  if (!dim) return SCM_BOOL_F;
  CommandAreaFactory factory{ *dim };
  auto area = factory.findArea(BlockPos(pos), false);
  if (!area) return SCM_BOOL_F;
  auto &source = area->getRegion();
  auto ret     = spawner.spawnItem(source, *item.get(), nullptr, pos, 0);
  if (!ret) return SCM_BOOL_F;
  return scm::to_scm(ret);
}

SCM_DEFINE_PUBLIC(player_spawn_item, "spawn-item/player", 3, 0, 0,
                  (scm::val<Vec3> pos, scm::val<ServerPlayer *> player, scm::val<ItemInstance *> item), "Spawn actor") {
  auto &level   = player->getLevel();
  auto &spawner = level.getSpawner();
  auto &source  = player->getRegion();
  auto ret      = spawner.spawnItem(source, *item.get(), nullptr, pos, 0);
  if (!ret) return SCM_BOOL_F;
  return scm::to_scm(ret);
}

PRELOAD_MODULE("minecraft world") {
#ifndef DIAG
#include "main.x"
#endif
}