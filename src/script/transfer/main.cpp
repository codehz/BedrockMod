// Deps: out/script_transfer.so: out/script_base.so
#include <api.h>
#include <base.h>

#include "../base/main.h"

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

SCM_DEFINE(c_transfer_world, "player-transfer", 2, 1, 0, (scm::val<ServerPlayer *> player, scm::val<std::string> address, scm::val<unsigned short> port), "Transfer player to another server") {
  unsigned short rport = scm_is_integer(port.scm) ? port.get() : 19132;
  TransferPacket packet{ address, rport };
  player->sendNetworkPacket(packet);
  return SCM_UNSPECIFIED;
}

static void init_guile() {
#ifndef DIAG
#include "main.x"
#endif
}

extern "C" void mod_init() { script_preload(init_guile); }