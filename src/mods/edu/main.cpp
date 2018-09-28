#include <StaticHook.h>
#include <string>

#define hookedu(name)                                                                                                                                \
  THook(bool, name, void *) { return true; }

#define hooknotedu(name)                                                                                                                             \
  THook(bool, name, void *) { return false; }

hookedu(_ZNK15DedicatedServer9isEduModeEv);
hookedu(_ZNK11AppPlatform9isEduModeEv);
hookedu(_ZNK5Level5isEduEv);
hooknotedu(_ZNK9LevelData23isEducationEditionLevelEv);
hooknotedu(_ZNK13LevelSettings23isEducationEditionWorldEv);
hookedu(_ZNK12LevelSummary5isEduEv);
hookedu(_ZNK9LevelData24educationFeaturesEnabledEv);
hookedu(_ZNK13LevelSettings24educationFeaturesEnabledEv);
hookedu(_ZNK6Option7getBoolEv);

TClasslessInstanceHook(
    void *, _ZN15CommandRegistry15registerCommandERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPKc22CommandPermissionLevel11CommandFlagSB_,
    std::string const &name, const char *prop, char level, char flag1, char flag2) {
  if (flag1 & 2) original(this, name, prop, 1 > level ? 1 : level, flag1 & ~2, flag2);
  return original(this, name, prop, level, flag1, flag2);
}

// TClasslessInstanceHook(void, _ZN12ActorMappingC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES7_, std::string const &name,
//                        std::string const &value) {
//   printf("%s - %s\n", name.c_str(), value.c_str());
//   original(this, name, value.length() == 0 ? name : value);
// }