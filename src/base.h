#pragma once

#include <exception>

#include <polyfill.h>

#include <chaiscript/chaiscript.hpp>

#include <minecraft/net/NetworkIdentifier.h>

struct EntityUniqueID {
  long long high, low;
};

struct EntityRuntimeID {
  long long eid = 0;
};

struct Vec3;

struct BlockPos {
  int x, y, z;
  BlockPos(int x, int y, int z)
      : x(x)
      , y(y)
      , z(z) {}
  BlockPos(Vec3 const &);
};

struct Vec3 {
  float x, y, z;
  Vec3(float x, float y, float z)
      : x(x)
      , y(y)
      , z(z) {}
  Vec3(BlockPos const &);
  Vec3()             = default;
  Vec3(Vec3 const &) = default;
};

struct Vec2 {
  float x, y;
};

struct DimensionId {
  int value;
  DimensionId(int value)
      : value(value) {}
};

struct BinaryStream;
struct NetEventCallback;

struct Packet {
  int unk_4 = 2, unk_8 = 1;
  unsigned char playerSubIndex = 0;

  Packet(unsigned char playerSubIndex)
      : playerSubIndex(playerSubIndex) {}

  virtual ~Packet();
  virtual void *getId() const                                               = 0;
  virtual void *getName() const                                             = 0;
  virtual void *write(BinaryStream &) const                                 = 0;
  virtual void *read(BinaryStream &)                                        = 0;
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const = 0;
  virtual bool disallowBatching() const;
};
struct Level;

struct Entity {
  const std::string &getNameTag() const;
  EntityRuntimeID getRuntimeID() const;
  Vec2 const &getRotation() const;
  Vec3 const &getPos() const;
  Level *getLevel() const;
  int getDimensionId() const;
};

struct Mob : Entity {
  float getYHeadRot() const;
};

struct Player : Mob {
  void remove();
  NetworkIdentifier const &getClientId() const;
  unsigned char getClientSubId() const;
};

struct ServerPlayer : Player {
  void disconnect();
  void sendNetworkPacket(Packet &packet) const;
};

struct PacketSender {
  virtual void *sendToClient(NetworkIdentifier const &, Packet const &, unsigned char) = 0;
};

struct Level {
  ServerPlayer *getPlayer(const std::string &name) const;
  ServerPlayer *getPlayer(EntityUniqueID uuid) const;
  PacketSender &getPacketSender() const;
  void forEachPlayer(std::function<bool(Player &)>);
};

enum struct InputMode { UNK };

struct ItemInstance;
struct ItemUseCallback;

struct GameMode {
  GameMode(Player &);
  ServerPlayer &player;
  virtual ~GameMode();
  virtual int startDestroyBlock(BlockPos const &, signed char, bool &);
  virtual int destroyBlock(BlockPos const &, signed char);
  virtual int continueDestroyBlock(BlockPos const &, signed char, bool &);
  virtual int stopDestroyBlock(BlockPos const &);
  virtual int startBuildBlock(BlockPos const &, signed char);
  virtual int buildBlock(BlockPos const &, signed char);
  virtual int continueBuildBlock(BlockPos const &, signed char);
  virtual int stopBuildBlock(void);
  virtual void tick(void);
  virtual long double getPickRange(InputMode const &, bool);
  virtual int useItem(ItemInstance &);
  virtual int useItemOn(ItemInstance &, BlockPos const &, signed char, Vec3 const &, ItemUseCallback *);
  virtual int interact(Entity &, Vec3 const &);
  virtual int attack(Entity &);
  virtual int releaseUsingItem(void);
  virtual int setTrialMode(bool);
  virtual int isInTrialMode(void);
  virtual int registerUpsellScreenCallback(std::function<void(bool)>);
};
struct SurvivalMode : GameMode {
  SurvivalMode(Player &);
  virtual ~SurvivalMode();
  virtual int startDestroyBlock(BlockPos const &, signed char, bool &);
  virtual int destroyBlock(BlockPos const &, signed char);
  virtual void tick(void);
  virtual int useItem(ItemInstance &);
  virtual int useItemOn(ItemInstance &, BlockPos const &, signed char, Vec3 const &, ItemUseCallback *);
  virtual int setTrialMode(bool);
  virtual int isInTrialMode(void);
  virtual int registerUpsellScreenCallback(std::function<void(bool)>);
};

extern void onPlayerAdded(std::function<void(ServerPlayer &player)> callback);
extern void onPlayerJoined(std::function<void(ServerPlayer &player)> callback);
extern void onPlayerLeft(std::function<void(ServerPlayer &player)> callback);

extern ServerPlayer *findPlayer(NetworkIdentifier const &nid, unsigned char subIndex);
