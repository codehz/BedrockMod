#pragma once

#include <polyfill.h>

#include <chaiscript/chaiscript.hpp>
#include <scm/scm.hpp>

extern "C" void loadModule(chaiscript::ModulePtr ptr);
extern "C" chaiscript::ChaiScript &getChai();