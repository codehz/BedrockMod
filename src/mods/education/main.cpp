#include <StaticHook.h>
#include <string>

bool asBool(const char *str) {
  if (str == "0" || str == "false") return false;
  return true;
}

extern "C" {
const char *mcpelauncher_property_get(const char *name, const char *def);
const char *mcpelauncher_property_get_group(const char *group, const char *name, const char *def);
}

static bool experimental = asBool(mcpelauncher_property_get("experimental", "false"));
static bool education    = asBool(mcpelauncher_property_get("education", "false"));

TClasslessInstanceHook(bool, _ZNK9LevelData23isEducationEditionLevelEv) { return education; }

TClasslessInstanceHook(bool, _ZNK9LevelData24educationFeaturesEnabledEv) { return education; }

TClasslessInstanceHook(bool, _ZNK17ConnectionRequest9isEduModeEv) { return education; }

TClasslessInstanceHook(bool, _ZNK13LevelSettings23isEducationEditionWorldEv) { return education; }

TClasslessInstanceHook(bool, _ZNK13MinecraftGame9isEduModeEv) { return education; }

TClasslessInstanceHook(bool, _ZThn16_NK13MinecraftGame9isEduModeEv) { return education; }

TClasslessInstanceHook(bool, _ZNK9LevelData30hasExperimentalGameplayEnabledEv) { return experimental; }

TClasslessInstanceHook(bool, _ZNK13LevelSettings31shouldForceExperimentalGameplayEv) { return experimental; }

TClasslessInstanceHook(bool, _ZNK5Level30hasExperimentalGameplayEnabledEv) { return experimental; }

TClasslessInstanceHook(bool, _ZNK12ItemInstance14isExperimentalEv) { return !experimental; }

TClasslessInstanceHook(bool, _ZNK5Actor14isExperimentalEv) { return !experimental; }

TClasslessInstanceHook(bool, _ZNK9LevelData32achievementsWillBeDisabledOnLoadEv) { return false; }

TClasslessInstanceHook(bool, _ZNK9LevelData23hasAchievementsDisabledEv) { return false; }

TClasslessInstanceHook(void, _ZN9LevelData19disableAchievementsEv) {}

struct Item {
  static bool mAllowExperimental;
};

struct Enchant {
  static bool mAllowExperimental;
};

extern "C" void mod_init() {
  Item::mAllowExperimental    = experimental;
  Enchant::mAllowExperimental = experimental;
}