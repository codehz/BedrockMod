#include <polyfill.h>

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

CHAISCRIPT_MODULE_EXPORT chaiscript::ModulePtr create_chaiscript_module_chat() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  m->add(chaiscript::fun([](ServerPlayer &player, std::string message) {
           auto packet = TextPacket::createSystemMessage(message);
           player.sendNetworkPacket(packet);
         }),
         "sendMessage");
  return m;
}