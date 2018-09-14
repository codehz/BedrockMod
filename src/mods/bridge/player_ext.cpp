#include <StaticHook.h>
#include <base.h>
#include <dlfcn.h>

static auto uuidoffset =
    *(int *)((char *)dlsym(MinecraftHandle(),
                           "_ZN6PlayerC2ER5LevelR12PacketSender8GameTypeRK17NetworkIdentifierhSt10unique_ptrI12SkinInfoDataSt14default_deleteIS9_"
                           "EEN3mce4UUIDERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES8_I11CertificateSA_ISN_EESM_SM_") +
             515);

mce::UUID &Player::getUUID() const { return *(mce::UUID *)((char *)this + uuidoffset); }
std::string Player::getXUID() const { return ExtendedCertificate::getXuid(getCertificate()); }