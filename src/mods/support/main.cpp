#include <polyfill.h>

#include <StaticHook.h>
#include <log.h>

#include <base.h>

std::unordered_map<NetworkIdentifier, std::vector<ServerPlayer *>> playermap;
std::vector<std::function<void(ServerPlayer &)>> joinedHandles, leftsHandles;

struct ServerNetworkHandler {};
struct ConnectionRequest {};

TInstanceHook(void, _ZN20ServerNetworkHandler24onReady_ClientGenerationER6PlayerRK17NetworkIdentifier, ServerNetworkHandler, ServerPlayer &player,
              NetworkIdentifier const &nid) {
  for (auto joined : joinedHandles) {
    playermap[nid].push_back(&player);
    joined(player);
  }
}

void kickPlayer(ServerPlayer *player) {
  player->disconnect();
  player->getLevel()->getLevelStorage()->save(*player);
  player->remove();

  if (player != nullptr) {
    for (auto left : leftsHandles) left(*player);
  }
}

TInstanceHook(void, _ZN20ServerNetworkHandler13_onPlayerLeftEP12ServerPlayerb, ServerNetworkHandler, ServerPlayer *player, bool flag) {
  kickPlayer(player);
}

TInstanceHook(void, _ZN20ServerNetworkHandler12onDisconnectERK17NetworkIdentifierRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEbSA_,
              ServerNetworkHandler, NetworkIdentifier const &nid, std::string const &str, bool b, std::string const &str2) {
  original(this, nid, str, b, str2);
  playermap.erase(nid);
}

void onPlayerJoined(std::function<void(ServerPlayer &player)> callback) { joinedHandles.push_back(callback); }
void onPlayerLeft(std::function<void(ServerPlayer &player)> callback) { leftsHandles.push_back(callback); }

ServerPlayer *findPlayer(NetworkIdentifier const &nid, unsigned char subIndex) {
  if (playermap.count(nid) < 0) return nullptr;
  auto &players = playermap.at(nid);
  for (auto player : players) {
    if (player->getClientSubId() == subIndex) return player;
  }
  return nullptr;
}

static Minecraft *mc;

TInstanceHook(void, _ZN9Minecraft4initEb, Minecraft, bool v) {
  original(this, v);
  mc = this;
}

extern "C" Minecraft *support_get_minecraft() { return mc; }