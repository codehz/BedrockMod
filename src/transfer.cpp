#include <api.h>
#include "base.h"

using namespace chaiscript;

struct TransferPacket : Packet {
  std::string ip;
  unsigned short port;

  TransferPacket(std::string ip, unsigned short port)
      : Packet(0)
      , ip(ip)
      , port(port) {}

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

extern "C" void mod_init() {
  ModulePtr m(new Module());

  m->add(fun([](ServerPlayer *player, std::string ip, unsigned short port) {
           TransferPacket packet{ ip, port };
           player->sendNetworkPacket(packet);
         }),
         "transferServer");
  loadModule(m);
}