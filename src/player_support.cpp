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
              NetworkIdentifier const &nid) {
  // original(this, player, nid); - do not call original so there are no join messages
  // instance.onPlayerJoined(player);
}

TInstanceHook(void, _ZN20ServerNetworkHandler13_onPlayerLeftEP12ServerPlayer, ServerNetworkHandler, ServerPlayer *player) {
  // original(this, player); - do not call original so there are no quit messages
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

Minecraft *minecraft = 0;

TInstanceHook(void *, _ZN9Minecraft4initEb, Minecraft, bool b) {
  minecraft = this;
  return original(this, b);
}

// struct Minecraft {
//   Level *getLevel() const;
//   IMPL(Minecraft, void *, init, , bool b) {
//     auto ret = $init()(this, b);
//     level    = getLevel();
//     level->getPacketSender();
//     Log::info("Player", "Level set");
//     return ret;
//   }
// };

void Player::sendPacket(Packet &packet) {
  Log::trace("Player", "mc: %08x, level: %08x", minecraft, minecraft->getLevel());
  this->getLevel()->getPacketSender().sendToClient(this->getClientId(), packet, this->getClientSubId());
}

void onPlayerJoined(std::function<void(ServerPlayer &player)> callback) { joinedHandles.push_back(callback); }
void onPlayerLeft(std::function<void(ServerPlayer &player)> callback) { leftsHandles.push_back(callback); }
