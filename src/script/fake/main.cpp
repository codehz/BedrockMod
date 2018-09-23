#include <api.h>

#include <StaticHook.h>
#include <unordered_set>

struct ExplodePacket : Packet {
  char filler[0x40 - sizeof(Packet)];
  ExplodePacket(Vec3 const &pos, float size, std::unordered_set<BlockPos> const &set) : Packet(0) {
    static auto fn = (void (*)(ExplodePacket *, Vec3 const &, float, std::unordered_set<BlockPos> const &))dlsym(
        MinecraftHandle(), "_ZN13ExplodePacketC2ERK4Vec3fRKSt13unordered_setI8BlockPosSt4hashIS4_ESt8equal_toIS4_ESaIS4_EE");
    fn(this, pos, size, set);
  }

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

struct Dimension {
  void sendPacketForPosition(BlockPos const &, Packet const &, Player const *);
};

namespace std {
template <> struct hash<BlockPos> { std::size_t operator()(const BlockPos &) const noexcept; };
}; // namespace std

SCM_DEFINE_PUBLIC(fake_explode, "fake-explode", 3, 0, 0, (scm::val<float> size, scm::val<Vec3> pos, scm::val<int> did), "Create a fake explode") {
  auto &level = ServerCommand::mGame->getLevel();
  auto dim    = level.getDimension(DimensionId(did));
  ExplodePacket pkt{ pos.get(), size.get(), {} };
  dim->sendPacketForPosition(BlockPos(pos), pkt, nullptr);
  return SCM_UNSPECIFIED;
}

PRELOAD_MODULE("minecraft fake") {
#ifndef DIAG
#include "main.x"
#endif
}