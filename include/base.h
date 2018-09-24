#pragma once

#include <exception>

#include <polyfill.h>

#include <minecraft/net/NetworkIdentifier.h>
#include <minecraft/net/UUID.h>

#include <log.h>
#include <memory>
#include <unordered_map>

struct Item;

struct ItemRegistry {
  static std::unordered_map<std::string, std::unique_ptr<Item>> mItemLookupMap;
  static Item *getItem(short id);
  static Item *lookupByName(std::string const &str, bool);
};

struct Item {
  unsigned short filler[0x1000];
  Item(Item const &) = delete;
  Item &operator=(Item const &) = delete;
  unsigned short getId() const;
  bool operator==(Item const &rhs) const { return this == &rhs; }
};

struct EntityUniqueID {
  long long high, low;
};

struct EntityRuntimeID {
  long long eid = 0;
};

struct Vec3;

struct BlockPos {
  int x, y, z;
  BlockPos();
  BlockPos(int x, int y, int z)
      : x(x)
      , y(y)
      , z(z) {}
  BlockPos(Vec3 const &);
  BlockPos(BlockPos const &p)
      : x(p.x)
      , y(p.y)
      , z(p.z) {}
  BlockPos const &operator=(BlockPos const&);
  bool operator==(BlockPos const&);
  bool operator!=(BlockPos const&);
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
  static Vec2 ZERO;
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

struct BlockEntity {
  void setData(int val);
  int getData() const;
  void setChanged();
};

struct Biome;
struct Block;

struct BlockLegacy {
  Block *getBlockStateFromLegacyData(unsigned char) const;
  std::string getFullName() const;
};

struct Block {
  BlockLegacy *getLegacyBlock() const;
};

struct ActorBlockSyncMessage;

struct BlockSource {
  BlockEntity *getBlockEntity(BlockPos const &);
  Biome *getBiome(BlockPos const &);
  Block *getBlock(BlockPos const &) const;
  Block *getExtraBlock(BlockPos const &) const;

  void setBlock(BlockPos const&,Block const&,int,ActorBlockSyncMessage const*);
};

struct ItemInstance;
struct CompoundTag;

struct Actor {
  const std::string &getNameTag() const;
  EntityRuntimeID getRuntimeID() const;
  Vec2 const &getRotation() const;
  Vec3 const &getPos() const;
  Level *getLevel() const;
  int getDimensionId() const;
  void getDebugText(std::vector<std::string> &);
  BlockSource &getRegion() const;
  void setOffhandSlot(ItemInstance const &);
  bool save(CompoundTag &);

  virtual ~Actor();
};

struct Mob : Actor {
  float getYHeadRot() const;
};

struct Certificate {};

struct ExtendedCertificate {
  static std::string getXuid(Certificate const &);
};

struct Player : Mob {
  void remove();
  Certificate &getCertificate() const;
  mce::UUID &getUUID() const;  // requires bridge
  std::string getXUID() const; // requires bridge
  NetworkIdentifier const &getClientId() const;
  unsigned char getClientSubId() const;
  BlockPos getSpawnPosition();
  bool setRespawnPosition(BlockPos const&,bool);
  bool setBedRespawnPosition(BlockPos const&);
  int getCommandPermissionLevel() const;
  bool isSurvival() const;
  bool isAdventure() const;
  bool isCreative() const;
  bool isWorldBuilder();

  void setOffhandSlot(ItemInstance const &);
};

struct ServerPlayer : Player {
  void disconnect();
  void sendNetworkPacket(Packet &packet) const;

  void openInventory();
};

struct PacketSender {
  virtual void *sendToClient(NetworkIdentifier const &, Packet const &, unsigned char) = 0;
};

struct LevelStorage {
  void save(Actor &);
};

struct Dimension;

struct Level {
  LevelStorage *getLevelStorage();
  ServerPlayer *getPlayer(const std::string &name) const;
  ServerPlayer *getPlayer(mce::UUID const &uuid) const;
  ServerPlayer *getPlayer(EntityUniqueID uuid) const;

  PacketSender &getPacketSender() const;
  void forEachPlayer(std::function<bool(Player &)>);
  BlockPos const &getDefaultSpawn() const;
  void setDefaultSpawn(BlockPos const &);
  Dimension *getDimension(DimensionId) const;
  void addEntity(BlockSource &, std::unique_ptr<Actor>);
  void addAutonomousEntity(BlockSource &, std::unique_ptr<Actor>);
};

struct DedicatedServer {
  void stop();
};

struct NetworkStats {
  int32_t filler, ping, avgping, maxbps;
  float packetloss, avgpacketloss;
};

struct NetworkPeer {
  virtual ~NetworkPeer();
  virtual void sendPacket();
  virtual void receivePacket();
  virtual NetworkStats getNetworkStatus(void);
};

struct NetworkHandler {
  NetworkPeer &getPeerForUser(NetworkIdentifier const&);
};

struct ServerNetworkHandler : NetworkHandler {
  void disconnectClient(NetworkIdentifier const&,std::string const&,bool);
};

struct MinecraftCommands;

struct Minecraft {
  void init(bool);
  void activateWhitelist();
  Level &getLevel() const;
  ServerNetworkHandler &getNetworkHandler();
  ServerNetworkHandler &getNetEventCallback();
  MinecraftCommands &getCommands();
};

struct ServerCommand {
  static Minecraft *mGame;
};

struct ServerInstance {
  void *vt, *filler;
  DedicatedServer *server;
  void queueForServerThread(std::function<void()> p1);
};

enum struct InputMode { UNK };

struct ItemInstance {
  bool isNull() const;
  unsigned short getId() const;
  std::string getName() const;
  std::string getCustomName() const;
  std::string toString() const;

  bool isOffhandItem() const;

  static ItemInstance EMPTY_ITEM;
};
struct ItemUseCallback;

extern void onPlayerJoined(std::function<void(ServerPlayer &player)> callback);
extern void onPlayerLeft(std::function<void(ServerPlayer &player)> callback);

extern ServerPlayer *findPlayer(NetworkIdentifier const &nid, unsigned char subIndex);
extern void kickPlayer(ServerPlayer *player);