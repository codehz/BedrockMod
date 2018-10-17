#include <StaticHook.h>
#include <string>
class NetworkIdentifier;
class ClientToServerHandshakePacket;
struct ServerNetworkHandler {
  void handleClientToServerHandshake(NetworkIdentifier const &, ClientToServerHandshakePacket const &);
};
TInstanceHook(void, _ZN20ServerNetworkHandler11handleLoginERK17NetworkIdentifierRK11LoginPacket, ServerNetworkHandler, NetworkIdentifier const &netId,
              void *packet) {
  original(this, netId, packet);
  ClientToServerHandshakePacket *whateverhonestly = nullptr;
  this->handleClientToServerHandshake(netId, *whateverhonestly);
}

struct NetworkIdentifier {};
struct Packet {
  size_t vt;
};

static auto vt_ServerToClientHandshakePacket = (size_t)dlsym(MinecraftHandle(), "_ZTV29ServerToClientHandshakePacket") + 0x10;

TClasslessInstanceHook(void, _ZN12PacketSender19sendToPrimaryClientERK17NetworkIdentifierRK6Packet, NetworkIdentifier const &id, Packet const &pkt) {
  if (pkt.vt != vt_ServerToClientHandshakePacket) original(this, id, pkt);
}

TClasslessInstanceHook(void, _ZN20EncryptedNetworkPeer16enableEncryptionERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE) {}