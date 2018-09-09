#include <StaticHook.h>
#include <string>

#include <api.h>

TClasslessInstanceHook(bool, _ZN17ConnectionRequest6verifyERKSt6vectorISsSaISsEEx, std::vector<std::string> const &list, long long number) {
  for (auto &item : list) { Log::info("Verify:MAIN", "%s", item.c_str()); }
  original(this, list, number);
  return true;
}

TClasslessInstanceHook(bool, _ZN26SubClientConnectionRequest6verifyERKSt6vectorISsSaISsEEx, std::vector<std::string> const &list, long long number) {
  for (auto &item : list) { Log::info("Verify:SUB", "%s", item.c_str()); }
  original(this, list, number);
  return true;
}