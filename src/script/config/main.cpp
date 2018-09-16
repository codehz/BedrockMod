#include <api.h>

#include "main.h"

SCM_DEFINE_PUBLIC(c_open_config, "open-config", 1, 0, 0, (scm::val<std::string> name), "Open config file") {
  auto ret = new INIReader(name);
  if (ret->ParseError() < 0) {
    delete ret;
    scm_misc_error("open-config", "Cannot open config file: ~A", scm::list(name.scm));
    return SCM_BOOL_F;
  }
  return scm::to_scm(ret);
}

SCM_DEFINE_PUBLIC(c_config_get_int, "config-get-integer", 3, 1, 0,
                  (scm::val<INIReader *> reader, scm::val<std::string> section, scm::val<std::string> name, scm::val<long> def), "Get integer") {
  auto d = scm_is_number(def.scm) ? def.get() : -1;
  return scm::to_scm(reader->GetInteger(section, name, d));
}

SCM_DEFINE_PUBLIC(c_config_get_real, "config-get-double", 3, 1, 0,
                  (scm::val<INIReader *> reader, scm::val<std::string> section, scm::val<std::string> name, scm::val<double> def), "Get real") {
  auto d = scm_is_number(def.scm) ? def.get() : -1.0;
  return scm::to_scm(reader->GetReal(section, name, d));
}

SCM_DEFINE_PUBLIC(c_config_get_bool, "config-get-bool", 3, 1, 0,
                  (scm::val<INIReader *> reader, scm::val<std::string> section, scm::val<std::string> name, scm::val<bool> def), "Get boolean") {
  auto d = scm_is_number(def.scm) ? def.get() : false;
  return scm::to_scm(reader->GetBoolean(section, name, d));
}

SCM_DEFINE_PUBLIC(c_config_get, "config-get", 3, 1, 0,
                  (scm::val<INIReader *> reader, scm::val<std::string> section, scm::val<std::string> name, scm::val<std::string> def),
                  "Get string") {
  auto d = scm_is_number(def.scm) ? def.get() : "";
  return scm::to_scm(reader->Get(section, name, d));
}

PRELOAD_MODULE("minecraft config") {
  scm::sym_list data_slots = { "ptr" };
  scm::foreign_type<INIReader *>("config", data_slots, [](SCM s) { delete (INIReader *)scm_foreign_object_ref(s, 0); });
#ifndef DIAG
#include "main.x"
#endif
}