#include <StaticHook.h>
#include <vector>
#include <memory>

struct EnchantmentInstance {
  int getEnchantType() const;
};

struct Enchant {
  enum Type : int{};
  static std::vector<std::unique_ptr<Enchant>> mEnchants;
  int getMaxLevel() const;
};

TInstanceHook(void, _ZN19EnchantmentInstance15setEnchantLevelEi, EnchantmentInstance, int level) {
  auto &enchant = Enchant::mEnchants[this->getEnchantType()];
  auto max = enchant->getMaxLevel();
  original(this, max < level ? max : level);
}

TInstanceHook(int, _ZNK19EnchantmentInstance15getEnchantLevelEv, EnchantmentInstance) {
  auto level = original(this);
  auto &enchant = Enchant::mEnchants[this->getEnchantType()];
  auto max = enchant->getMaxLevel();
  return max < level ? max : level;
}

struct ItemInstance;

THook(int, _ZN12EnchantUtils15getEnchantLevelEN7Enchant4TypeERK12ItemInstance, Enchant::Type type, ItemInstance const& item) {
  auto level = original(type, item);
  auto &enchant = Enchant::mEnchants[type];
  auto max = enchant->getMaxLevel();
  return max < level ? max : level;
}