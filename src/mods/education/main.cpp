#include <StaticHook.h>

TClasslessInstanceHook(bool, _ZNK9LevelData23isEducationEditionLevelEv) { return true; }

TClasslessInstanceHook(bool, _ZNK9LevelData24educationFeaturesEnabledEv) { return true; }

TClasslessInstanceHook(bool, _ZNK17ConnectionRequest9isEduModeEv) { return true; }

TClasslessInstanceHook(bool, _ZNK9LevelData32achievementsWillBeDisabledOnLoadEv) { return false; }

TClasslessInstanceHook(bool, _ZNK9LevelData23hasAchievementsDisabledEv) { return false; }

TClasslessInstanceHook(void, _ZN9LevelData19disableAchievementsEv) {}

TClasslessInstanceHook(bool, _ZNK9LevelData30hasExperimentalGameplayEnabledEv) { return true; }

TClasslessInstanceHook(bool, _ZNK13LevelSettings23isEducationEditionWorldEv) { return true; }

TClasslessInstanceHook(bool, _ZNK13MinecraftGame9isEduModeEv) { return true; }

TClasslessInstanceHook(bool, _ZThn16_NK13MinecraftGame9isEduModeEv) { return true; }

TClasslessInstanceHook(bool, _ZNK13LevelSettings31shouldForceExperimentalGameplayEv) { return true; }

TClasslessInstanceHook(bool, _ZNK5Level30hasExperimentalGameplayEnabledEv) { return true; }

TClasslessInstanceHook(bool, _ZNK12ItemInstance14isExperimentalEv) { return false; }

TClasslessInstanceHook(bool, _ZNK5Actor14isExperimentalEv) { return false; }

struct Item {
  static bool mAllowExperimental;
};

struct Enchant {
  static bool mAllowExperimental;
};

extern "C" void mod_init() {
  Item::mAllowExperimental = true;
  Enchant::mAllowExperimental = true;
}