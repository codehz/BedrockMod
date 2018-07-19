#include <api.h>

#include <StaticHook.h>
#include <log.h>

#include <functional>

#include "base.h"

static std::unordered_multimap<uint16_t, std::function<void(void)>> tickHandles;
static std::vector<std::pair<uint16_t, std::function<void(void)>>> timeoutHandles;
static uint16_t count = 0;

TInstanceHook(void, _ZN5Level4tickEv, Level) {
  for (auto it : tickHandles) {
    if (count % it.first == 0) {
      try {
        it.second();
      } catch (std::exception const &e) { Log::error("BASE", "TICK ERROR: %s", e.what()); }
    }
  }
  auto it = timeoutHandles.begin();
  while (it != timeoutHandles.end()) {
    it->first--;
    if (it->first <= 0) {
      try {
        it->second();
      } catch (std::exception const &e) { Log::error("BASE", "TICK ERROR: %s", e.what()); }
      it = timeoutHandles.erase(it);
    } else
      ++it;
  }
  count++;
  original(this);
}

extern "C" void mod_init() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  m->add(chaiscript::fun([](std::function<void(void)> fn, uint16_t cycle) { tickHandles.emplace(cycle, fn); }), "setInterval");
  m->add(chaiscript::fun([](std::function<void(void)> fn, uint16_t len) { timeoutHandles.push_back(std::make_pair(len, fn)); }), "setTimeout");
  loadModule(m);
}