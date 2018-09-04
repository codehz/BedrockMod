// Deps: out/script_chat.so: out/script_base.so
#include <api.h>

#include <StaticHook.h>

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

TClasslessInstanceHook(void, _ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK10TextPacket, NetworkIdentifier const &nid,
                       TextPacket const &packet) {
  if (scm::sym(R"(%player-chat)")) {
    auto player = findPlayer(nid, packet.playerSubIndex);
    scm::call(R"(%player-chat)", (ServerPlayer *)&player, packet.message);
    return;
  }
  original(this, nid, packet);
}

SCM_DEFINE(c_send_message, "send-message", 2, 1, 0, (scm::val<ServerPlayer *> player, scm::val<std::string> message, scm::val<int> type),
           "Send message to player") {
  auto packet = TextPacket::createSystemMessage(message);
  if (scm_is_integer(type.scm)) packet.type = type;
  player->sendNetworkPacket(packet);
  return SCM_UNSPECIFIED;
}

static void init_guile() {
#ifndef DIAG
#include "main.x"
#endif
}

extern "C" void mod_init() { script_preload(init_guile); }