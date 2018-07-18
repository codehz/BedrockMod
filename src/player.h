#pragma once

#include <exception>

#include <polyfill.h>

#include <chaiscript/chaiscript.hpp>

#include <minecraft/net/NetworkIdentifier.h>

struct EntityUniqueID {
  long long high, low;
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
  Level *getLevel() const;
};

struct Mob : Entity {};

struct Player : Mob {
  void remove();
  NetworkIdentifier const &getClientId() const;
  unsigned char getClientSubId() const;

  void sendPacket(Packet &packet);
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

struct Minecraft {
  Level *getLevel() const;
};

extern Minecraft *minecraft;

extern void onPlayerJoined(std::function<void(ServerPlayer &player)> callback);
extern void onPlayerLeft(std::function<void(ServerPlayer &player)> callback);

void registerPlayer(chaiscript::ModulePtr m);
