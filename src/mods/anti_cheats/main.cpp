#include <StaticHook.h>
#include <vector>
#include <memory>

struct EnchantmentInstance {
  int getEnchantType() const;
};

struct Enchant {
  static std::vector<std::unique_ptr<Enchant>> mEnchants;
  int getMaxLevel() const;
};

TInstanceHook(void, _ZN19EnchantmentInstance15setEnchantLevelEi, EnchantmentInstance, int level) {
  auto &enchant = Enchant::mEnchants[this->getEnchantType()];
  auto max = enchant->getMaxLevel();
  original(this, max < level ? max : level);
}