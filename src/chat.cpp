#include <api.h>

#include <log.h>

#include "player.h"
struct TextPacket : Packet {
  char filler[0x30];
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

extern "C" void mod_init() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  m->add(chaiscript::fun([](ServerPlayer &player, std::string message, unsigned type) {
           auto packet      = TextPacket::createSystemMessage(message);
           packet.filler[0] = type;
           player.sendNetworkPacket(packet);
         }),
         "sendCustomMessage");
  m->add(chaiscript::fun([](ServerPlayer &player, std::string message) {
           auto packet = TextPacket::createSystemMessage(message);
           player.sendNetworkPacket(packet);
         }),
         "sendMessage");
  loadModule(m);
}