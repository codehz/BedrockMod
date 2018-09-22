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

int limitLevel(int input, int max) {
  if (input < 0) return 0;
  else if (input > max) return 0;
  return input;
}

TInstanceHook(void, _ZN19EnchantmentInstance15setEnchantLevelEi, EnchantmentInstance, int level) {
  auto &enchant = Enchant::mEnchants[this->getEnchantType()];
  auto max = enchant->getMaxLevel();
  original(this, limitLevel(level, max));
}

TInstanceHook(int, _ZNK19EnchantmentInstance15getEnchantLevelEv, EnchantmentInstance) {
  auto level = original(this);
  auto &enchant = Enchant::mEnchants[this->getEnchantType()];
  auto max = enchant->getMaxLevel();
  return limitLevel(level, max);
}

struct ItemInstance;

THook(int, _ZN12EnchantUtils15getEnchantLevelEN7Enchant4TypeERK12ItemInstance, Enchant::Type type, ItemInstance const& item) {
  auto level = original(type, item);
  auto &enchant = Enchant::mEnchants[type];
  auto max = enchant->getMaxLevel();
  return limitLevel(level, max);
}