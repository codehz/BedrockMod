// Deps: out/script_chat.so: out/script_base.so
#include <api.h>

#include <StaticHook.h>

#include <list>
#include <memory>

struct FixedFunction {
  int16_t chip;
  scm::callback<> fun;
  FixedFunction(int16_t chip, scm::callback<> fun)
      : chip(chip)
      , fun(fun) {
    scm_gc_protect_object(fun);
  }
  FixedFunction(FixedFunction &&rhs)
      : chip(rhs.chip)
      , fun(rhs.fun) {
    rhs.fun.setInvalid();
  }
  operator int16_t() { return chip; }
  void operator()() { fun(); }
  ~FixedFunction() {
    if (scm_is_true(fun)) scm_gc_unprotect_object(fun);
  }
};

std::unordered_multimap<int16_t, FixedFunction> tickHandlers;
std::list<FixedFunction> timeoutHandlers;
int16_t count = 0;

TInstanceHook(void, _ZN5Level4tickEv, Level) {
  for (auto &it : tickHandlers)
    if (count % it.first == it.second) it.second();
  if (!timeoutHandlers.empty()) {
    auto &to = timeoutHandlers.front();
    if (to.chip-- <= 0) {
      to();
      timeoutHandlers.pop_front();
    }
    while (!timeoutHandlers.empty() && timeoutHandlers.front().chip <= 0) {
      timeoutHandlers.front()();
      timeoutHandlers.pop_front();
    }
  }
  count++;
  original(this);
}

SCM_DEFINE_PUBLIC(c_set_interval, "interval-run", 2, 0, 0, (scm::val<uint> cycle, scm::callback<> fn), "setInterval") {
  auto it = tickHandlers.emplace(std::make_pair(cycle, FixedFunction{ (int16_t)(count % cycle), fn }));
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(c_set_timeout, "delay-run", 2, 0, 0, (scm::val<uint> len, scm::callback<> fn), "setTimeout") {
  int16_t sum = 0;
  for (auto it = timeoutHandlers.begin(); it != timeoutHandlers.end(); ++it) {
    sum += it->chip;
    if (sum > len) {
      timeoutHandlers.insert(it, FixedFunction(it->chip - sum + len, fn));
      it->chip -= sum - len;
      return SCM_UNSPECIFIED;
    }
  }
  timeoutHandlers.push_back(FixedFunction(len - sum, fn));
  return SCM_UNSPECIFIED;
}

LOADFILE(preload, "src/script/tick/preload.scm");

PRELOAD_MODULE("minecraft tick") {
#ifndef DIAG
#include "main.x"
#include "preload.scm.z"
#endif

  scm_c_eval_string(&file_preload_start);
}