// Deps: out/libanti_cheats.so: out/libsupport.so out/libbridge.so

#include <StaticHook.h>
#include <base.h>
#include <log.h>

#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>

struct EnchantmentInstance {
  int getEnchantType() const;
  void setEnchantLevel(int);
};

struct Enchant {
  enum Type : int {};
  static std::vector<std::unique_ptr<Enchant>> mEnchants;
  virtual ~Enchant();
  virtual bool isCompatibleWith(Enchant::Type type);
  virtual int getMinCost(int);
  virtual int getMaxCost(int);
  virtual int getMinLevel();
  virtual int getMaxLevel();
};

int limitLevel(int input, int max) {
  if (input < 0)
    return 0;
  else if (input > max)
    return 0;
  return input;
}

TInstanceHook(int, _ZNK19EnchantmentInstance15getEnchantLevelEv, EnchantmentInstance) {
  auto level    = original(this);
  auto &enchant = Enchant::mEnchants[this->getEnchantType()];
  auto max      = enchant->getMaxLevel();
  auto result   = limitLevel(level, max);
  if (result != level) setEnchantLevel(level);
  return result;
}

struct InventorySource {
  int getType() const;
  int getContainerId() const;

  std::string toString() const;
};

struct InventoryAction {
  char filler[480];
  int getSlot() const;
  ItemInstance *getFromItem() const;
  ItemInstance *getToItem() const;
  InventorySource *getSource() const;
};

struct InventoryTransactionManager {
  ServerPlayer *player;
};

TInstanceHook(void *, _ZN27InventoryTransactionManager9addActionERK15InventoryAction, InventoryTransactionManager, InventoryAction &action) {
  // Log::debug("AC", "(%s:%d) From %s to %s", action.getSource()->toString().c_str(), action.getSlot(), action.getFromItem()->toString().c_str(),
  //            action.getToItem()->toString().c_str());
  if (action.getSource()->getType() == 0) {
    auto c = action.getSource()->getContainerId();
    if (c == 119) {
      switch (action.getToItem()->getId()) {
      case 0: break;
      default:
        if (action.getToItem()->isOffhandItem()) { break; }
        Log::debug("AC", "Kicked %s due to offhand hack for %s", player->getUUID().asString().c_str(), action.getToItem()->toString().c_str());
        ServerCommand::mGame->getNetEventCallback().disconnectClient(player->getClientId(), "Cheating detected", false);
        return nullptr;
      }
    }
  }
  return original(this, action);
}

struct ConnectionRequest {
  std::string getDeviceId() const;
  std::string getTenantId() const;
  std::string getDeviceModel() const;
  std::string getSelfSignedId() const;
  std::string getSkinGeometry() const;
  std::string getServerAddress() const;
  std::string getClientPlatformId() const;
  std::string getSkinGeometryName() const;
  std::string getGameVersionString() const;
  std::string getClientThirdPartyName() const;
  std::string getClientPlatformOnlineId() const;
  std::string getClientPlatformOfflineId() const;
  std::string getSkinId() const;

  int getDeviceOS() const;
  int getGuiScale() const;
  int getUIProfile() const;
  int getCurrentInputMode() const;
  int getDefaultInputMode() const;
  int getADRole() const;
  std::string toString();
};

TInstanceHook(bool, _ZNK17ConnectionRequest9isEduModeEv, ConnectionRequest) {
  Log::debug("login", "Version: %s", getGameVersionString().c_str());
  Log::debug("login", "3rdName: %s", getClientThirdPartyName().c_str());
  Log::debug("login", "DeviceID: %s", getDeviceId().c_str());
  Log::debug("login", "DeviceModel: %s", getDeviceModel().c_str());
  Log::debug("login", "ServerAddress: %s", getServerAddress().c_str());
  // Log::debug("login", "TenantID: %s", getTenantId().c_str()); // Always empty
  Log::debug("login", "SkinId: %s", getSkinId().c_str());
  // Log::debug("login", "SkinGeometry: %s", getSkinGeometry().c_str()); // too long!
  Log::debug("login", "SkinGeometryName: %s", getSkinGeometryName().c_str());
  Log::debug("login", "SelfSignedId: %s", getSelfSignedId().c_str());
  // Log::debug("login", "ClientPlatformOnlineId: %s", getClientPlatformOnlineId().c_str()); // Always empty
  // Log::debug("login", "ClientPlatformOfflineId: %s", getClientPlatformOfflineId().c_str()); // Always empty
  Log::debug("login", "OS: %d GuiScale: %d UIProfile: %d CurrentIME: %d DefaultIME: %d ADRole: %d", getDeviceOS(), getGuiScale(), getUIProfile(),
             getCurrentInputMode(), getDefaultInputMode(), getADRole());
  return getGameVersionString() == "1.6.0";
}