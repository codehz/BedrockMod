// Deps: out/script_chat.so: out/script_base.so
#include "../base/main.h"
#include <api.h>

#include <StaticHook.h>

struct TextPacket : Packet {
  unsigned char type; // 17
  unsigned char filler[96 - 18];
  std::string message; // 96
  unsigned char filler2[150];
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

MAKE_HOOK(player_chat, "player-chat", ServerPlayer *, std::string);
MAKE_FLUID(bool, cancel_chat, "cancel-chat");

TInstanceHook(void, _ZN20ServerNetworkHandler10handleTextERK17NetworkIdentifierRK10TextPacket, ServerNetworkHandler, NetworkIdentifier const &nid,
              TextPacket const &packet) {
  auto canceled = cancel_chat()[false] <<= [=] {
    auto player = _getServerPlayer(nid, packet.playerSubIndex);
    player_chat((ServerPlayer *)player, packet.message);
  };
  if (!canceled) original(this, nid, packet);
}

SCM_DEFINE_PUBLIC(c_send_message, "send-message", 2, 1, 0, (scm::val<ServerPlayer *> player, scm::val<std::string> message, scm::val<int> type),
                  "Send message to player") {
  auto packet = TextPacket::createSystemMessage(message);
  if (scm_is_integer(type.scm)) packet.type = type;
  player->sendNetworkPacket(packet);
  return SCM_UNSPECIFIED;
}

PRELOAD_MODULE("minecraft chat") {
#ifndef DIAG
#include "main.x"
#endif
}