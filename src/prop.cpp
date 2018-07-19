#include <api.h>

#include <log.h>

extern "C" {
const char *mcpelauncher_property_get(const char *name, const char *def);
const char *mcpelauncher_property_get_group(const char *group, const char *name, const char *def);
void mcpelauncher_property_set(const char *name, const char *value);
void mcpelauncher_property_set_group(const char *group, const char *name, const char *value);
void mod_init() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  m->add(chaiscript::fun([](std::string name, std::string def) -> std::string { return mcpelauncher_property_get(name.c_str(), def.c_str()); }),
         "getProperty");
  m->add(chaiscript::fun([](std::string name, std::string value) { mcpelauncher_property_set(name.c_str(), value.c_str()); }), "setProperty");
  m->add(chaiscript::fun([](std::string group, std::string name, std::string def) -> std::string {
           return mcpelauncher_property_get_group(group.c_str(), name.c_str(), def.c_str());
         }),
         "getProperty");
  m->add(chaiscript::fun([](std::string group, std::string name, std::string value) {
           mcpelauncher_property_set_group(group.c_str(), name.c_str(), value.c_str());
         }),
         "setProperty");
  loadModule(m);
}
}