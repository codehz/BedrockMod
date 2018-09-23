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
  return limitLevel(level, max);
}

struct ItemInstance;

THook(int, _ZN12EnchantUtils15getEnchantLevelEN7Enchant4TypeERK12ItemInstance, Enchant::Type type, ItemInstance const &item) {
  auto level    = original(type, item);
  auto &enchant = Enchant::mEnchants[type];
  auto max      = enchant->getMaxLevel();
  return limitLevel(level, max);
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
