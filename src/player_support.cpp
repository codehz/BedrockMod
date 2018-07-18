#include <polyfill.h>

#include <StaticHook.h>
#include <log.h>

#include "player.h"

static std::unordered_map<NetworkIdentifier, Player *> playermap;
static std::vector<std::function<void(ServerPlayer &)>> joinedHandles, leftsHandles;

struct ServerNetworkHandler {};
struct ConnectionRequest {};

TInstanceHook(ServerPlayer *, _ZN20ServerNetworkHandler16_createNewPlayerERK17NetworkIdentifierRK17ConnectionRequest, ServerNetworkHandler,
              NetworkIdentifier const &nid, ConnectionRequest const &req) {
  ServerPlayer *ret = original(this, nid, req);
  playermap[nid]    = ret;
  for (auto joined : joinedHandles) try {
      joined(*ret);
    } catch (const std::exception &e) { Log::error("ChaiExtra", e.what()); }
  return ret;
}

TInstanceHook(void, _ZN20ServerNetworkHandler24onReady_ClientGenerationER6PlayerRK17NetworkIdentifier, ServerNetworkHandler, Player &player,
              NetworkIdentifier const &nid) {}

TInstanceHook(void, _ZN20ServerNetworkHandler13_onPlayerLeftEP12ServerPlayer, ServerNetworkHandler, ServerPlayer *player) {
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

void onPlayerJoined(std::function<void(ServerPlayer &player)> callback) { joinedHandles.push_back(callback); }
void onPlayerLeft(std::function<void(ServerPlayer &player)> callback) { leftsHandles.push_back(callback); }
