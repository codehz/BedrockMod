#include <api.h>

#include <StaticHook.h>
#include <fix/string.h>
#include <log.h>

#include <functional>

#include "player.h"

struct TextPacket : Packet {
  unsigned char type; // 13
  int primaryName;
  int thirdPartyName;
  int platform;
  std::string message; // 28
  unsigned char filler[30];
  static TextPacket createSystemMessage(std::string const &);

  TextPacket(unsigned char playerSubIndex)
      : Packet(playerSubIndex) {}

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

static std::function<void(ServerPlayer *, std::string)> chatHook;

TClasslessInstanceHook(void, _ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK10TextPacket, NetworkIdentifier const &nid,
                       TextPacket const &packet) {
  if (chatHook) {
    auto player = findPlayer(nid, packet.playerSubIndex);
    try {
      chatHook(player, packet.message);
      return;
    } catch (std::exception const &e) { Log::error("CHATHOOK", "Error: %s", e.what()); }
  }
  original(this, nid, packet);
}

extern "C" void mod_init() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  m->add(chaiscript::fun([](ServerPlayer *player, std::string message, unsigned type) {
           auto packet = TextPacket::createSystemMessage(message);
           packet.type = type;
           player->sendNetworkPacket(packet);
         }),
         "sendCustomMessage");
  m->add(chaiscript::fun([](ServerPlayer *player, std::string message) {
           auto packet = TextPacket::createSystemMessage(message);
           player->sendNetworkPacket(packet);
         }),
         "sendMessage");
  m->add(chaiscript::fun([](std::function<void(ServerPlayer *, std::string)> fn) { chatHook = fn; }), "setChatHook");
  loadModule(m);
}