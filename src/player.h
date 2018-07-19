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

struct Vec3 {
  float x, y, z;
  Vec3(float x, float y, float z)
      : x(x)
      , y(y)
      , z(z) {}
  Vec3() = default;
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
};

extern void onPlayerAdded(std::function<void(ServerPlayer &player)> callback);
extern void onPlayerJoined(std::function<void(ServerPlayer &player)> callback);
extern void onPlayerLeft(std::function<void(ServerPlayer &player)> callback);

void registerPlayer(chaiscript::ModulePtr m);
