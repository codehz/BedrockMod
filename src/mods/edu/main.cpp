#include <StaticHook.h>
#include <string>

#define hookedu(name)                                                                                                                                \
  THook(bool, name, void *) { return true; }

hookedu(_ZNK15DedicatedServer9isEduModeEv);
hookedu(_ZNK11AppPlatform9isEduModeEv);
hookedu(_ZNK17ConnectionRequest9isEduModeEv);
hookedu(_ZNK5Level5isEduEv);
hookedu(_ZNK9LevelData23isEducationEditionLevelEv);
hookedu(_ZNK13LevelSettings23isEducationEditionWorldEv);
hookedu(_ZNK12LevelSummary5isEduEv);
hookedu(_ZNK9LevelData24educationFeaturesEnabledEv);
hookedu(_ZNK13LevelSettings24educationFeaturesEnabledEv);