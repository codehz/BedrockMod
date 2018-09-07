#include <api.h>

#include <log.h>

extern "C" {
const char *mcpelauncher_property_get(const char *name, const char *def);
const char *mcpelauncher_property_get_group(const char *group, const char *name, const char *def);
void mcpelauncher_property_set(const char *name, const char *value);
void mcpelauncher_property_set_group(const char *group, const char *name, const char *value);
}

SCM_DEFINE_PUBLIC(c_prop_get, "prop-get", 1, 1, 0, (scm::val<const char *> name, scm::val<std::string> def), "Get property") {
  auto rdef = scm_is_string(def.scm) ? (std::string)def : "";
  return scm::to_scm(mcpelauncher_property_get((temp_string)name, rdef.c_str()));
}

SCM_DEFINE_PUBLIC(c_prop_group_get, "prop-group-get", 2, 1, 0, (scm::val<const char *> group, scm::val<const char *> name, scm::val<std::string> def),
                  "Get group property") {
  auto rdef = scm_is_string(def.scm) ? (std::string)def : "";
  return scm::to_scm(mcpelauncher_property_get_group((temp_string)group, (temp_string)name, rdef.c_str()));
}

SCM_DEFINE_PUBLIC(c_prop_set, "prop-set", 2, 0, 0, (scm::val<const char *> name, scm::val<const char *> val), "Set property") {
  mcpelauncher_property_set((temp_string)name, (temp_string)val);
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(c_prop_group_set, "prop-group-set", 3, 0, 0,
                  (scm::val<const char *> group, scm::val<const char *> name, scm::val<const char *> val), "Set property") {
  mcpelauncher_property_set_group((temp_string)group, (temp_string)name, (temp_string)val);
  return SCM_UNSPECIFIED;
}

PRELOAD_MODULE("minecraft conf") {
#ifndef DIAG
#include "main.x"
#endif
}