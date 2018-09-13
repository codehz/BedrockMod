#include <StaticHook.h>
#include <string>

TClasslessInstanceHook(bool, _ZN17ConnectionRequest6verifyERKSt6vectorINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESaIS6_EEl, std::vector<std::string> const &list, long long number) {
  for (auto &item : list) { Log::info("Verify:MAIN", "%s", item.c_str()); }
  original(this, list, number);
  return true;
}

TClasslessInstanceHook(bool, _ZN26SubClientConnectionRequest6verifyERKSt6vectorINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESaIS6_EEl, std::vector<std::string> const &list, long long number) {
  for (auto &item : list) { Log::info("Verify:SUB", "%s", item.c_str()); }
  original(this, list, number);
  return true;
}