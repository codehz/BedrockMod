#include <polyfill.h>

#include <StaticHook.h>
#include <fix/string.h>
#include <log.h>

#include "base.h"

std::unordered_map<NetworkIdentifier, std::vector<ServerPlayer *>> playermap;
std::vector<std::function<void(ServerPlayer &)>> addedHandles, joinedHandles, leftsHandles;

struct ServerNetworkHandler {};
struct ConnectionRequest {};

TInstanceHook(ServerPlayer *, _ZN20ServerNetworkHandler16_createNewPlayerERK17NetworkIdentifierRK17ConnectionRequest, ServerNetworkHandler,
              NetworkIdentifier const &nid, ConnectionRequest const &req) {
  ServerPlayer *ret = original(this, nid, req);

  playermap[nid].push_back(ret);
  for (auto added : addedHandles) try {
      added(static_cast<ServerPlayer &>(*ret));
    } catch (const std::exception &e) { Log::error("ChaiExtra", e.what()); }
  return ret;
}

TInstanceHook(void, _ZN20ServerNetworkHandler24onReady_ClientGenerationER6PlayerRK17NetworkIdentifier, ServerNetworkHandler, Player &player,
              NetworkIdentifier const &nid) {
  for (auto joined : joinedHandles) try {
      joined(static_cast<ServerPlayer &>(player));
    } catch (const std::exception &e) { Log::error("ChaiExtra", e.what()); }
}

TInstanceHook(void, _ZN20ServerNetworkHandler13_onPlayerLeftEP12ServerPlayerb, ServerNetworkHandler, ServerPlayer *player, bool flag) {
  player->disconnect();
  player->remove();

  if (player != nullptr) {
    for (auto left : leftsHandles) try {
        left(*player);
      } catch (const std::exception &e) { Log::error("ChaiExtra", e.what()); }
  }
}

TInstanceHook(void, _ZN20ServerNetworkHandler12onDisconnectERK17NetworkIdentifierRKSsbS4_, ServerNetworkHandler, NetworkIdentifier const &nid,
              std::string const &str, bool b, std::string const &str2) {
  original(this, nid, str, b, str2);
  playermap.erase(nid);
}

void onPlayerAdded(std::function<void(ServerPlayer &player)> callback) { addedHandles.push_back(callback); }
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

extern "C" void __init() { mcpe::string::empty = static_cast<mcpe::string *>(dlsym(MinecraftHandle(), "_ZN4Util12EMPTY_STRINGE")); }