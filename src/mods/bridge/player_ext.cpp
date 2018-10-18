#include <StaticHook.h>
#include <base.h>
#include <dlfcn.h>

static auto uuidoffset = 5704;

mce::UUID &Player::getUUID() const { return *(mce::UUID *)((char *)this + uuidoffset); }
std::string Player::getXUID() const { return ExtendedCertificate::getXuid(getCertificate()); }