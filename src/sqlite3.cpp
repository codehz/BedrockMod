#include <api.h>

#include <log.h>

#include <functional>
#include <vector>

#include <sqlite3/sqlite3.h>

using namespace chaiscript;

extern "C" {
const char *mcpelauncher_property_get(const char *name, const char *def);
}

struct Sqlite3Error : std::runtime_error {
  Sqlite3Error(const char *err)
      : std::runtime_error(err) {}
};

enum class Sqlite3StmtStepResult { OK = 0, ROW = 100, DONE = 101 };

struct Sqlite3Stmt {
  sqlite3_stmt *stmt;
  Sqlite3Stmt(sqlite3_stmt *stmt)
      : stmt(stmt) {}
  Sqlite3Stmt(Sqlite3Stmt const &) = delete;
  Sqlite3Stmt &operator=(Sqlite3Stmt const &) = delete;
  Sqlite3StmtStepResult step() {
    auto ret = sqlite3_step(stmt);
    if (ret != 0 && ret != 101 && ret != 100) {
      Log::error("Sqlite3", "step(): %s", sqlite3_errstr(ret));
      throw Sqlite3Error(sqlite3_errstr(ret));
    }
    return (Sqlite3StmtStepResult)ret;
  }
  void reset() {
    int err = 0;
    if (sqlite3_reset(stmt)) {
      Log::error("Sqlite3", "reset(): %s", sqlite3_errstr(err));
      throw Sqlite3Error(sqlite3_errstr(err));
    }
  }
  void clear() { sqlite3_clear_bindings(stmt); }
  void bind(int idx, Boxed_Value const &value) {
    if (value.is_null() || value.is_undef()) {
      sqlite3_bind_null(stmt, idx);
    } else if (value.is_type(chaiscript::user_type<int>())) {
      sqlite3_bind_int(stmt, idx, boxed_cast<int>(value));
    } else if (value.is_type(chaiscript::user_type<int64_t>())) {
      sqlite3_bind_int64(stmt, idx, boxed_cast<int64_t>(value));
    } else if (value.is_type(chaiscript::user_type<float>())) {
      sqlite3_bind_double(stmt, idx, boxed_cast<float>(value));
    } else if (value.is_type(chaiscript::user_type<double>())) {
      sqlite3_bind_double(stmt, idx, boxed_cast<double>(value));
    } else if (value.is_type(chaiscript::user_type<std::string>())) {
      auto str     = boxed_cast<std::string>(value);
      char *buffer = (char *)malloc(str.length() + 1);
      strncpy(buffer, str.c_str(), str.length() + 1);
      sqlite3_bind_text(stmt, idx, buffer, -1, free);
    } else {
      throw Sqlite3Error("Failed to convert the type");
    }
  }
  void bindNamed(std::string name, Boxed_Value const &value) {
    auto idx = sqlite3_bind_parameter_index(stmt, name.c_str());
    if (idx == 0) throw Sqlite3Error("Failed to find parameter");
    return bind(idx, value);
  }
  Boxed_Value get(int col) {
    auto type = sqlite3_column_type(stmt, col);
    switch (type) {
    case SQLITE_INTEGER: return Boxed_Value(sqlite3_column_int64(stmt, col));
    case SQLITE_FLOAT: return Boxed_Value(sqlite3_column_double(stmt, col));
    case SQLITE3_TEXT: return Boxed_Value(std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, col))));
    default: return Boxed_Value(nullptr);
    }
  }
  ~Sqlite3Stmt() { sqlite3_finalize(stmt); }
};

struct Sqlite3 {
  using callback_t = std::function<int(std::map<std::string, std::string> &smap)>;
  sqlite3 *db;
  Sqlite3(std::string name) {
    auto result = sqlite3_open(name.c_str(), &db);
    if (result != SQLITE_OK) {
      auto msg = sqlite3_errmsg(db);
      Log::error("Sqlite3", "%s: %s", name.c_str(), msg);
      sqlite3_free((void *)msg);
    }
    char *err = 0;
    sqlite3_exec(db, "PRAGMA synchronous = OFF; PRAGMA journal_mode = MEMORY;", nullptr, nullptr, &err);
    if (err != nullptr) {
      Log::error("Sqlite3", "%s: %s", name.c_str(), err);
      sqlite3_free(err);
    }
  }
  Sqlite3(sqlite3 *db)
      : db(db) {}
  Sqlite3(Sqlite3 const &) = delete;
  Sqlite3 &operator=(Sqlite3 const &) = delete;
  ~Sqlite3() { sqlite3_close(db); }
  static int stub(void *cb, int num, char **value, char **key) {
    auto const &callback = *static_cast<callback_t *>(cb);
    std::map<std::string, std::string> data;
    for (int i = 0; i < num; i++) { data.emplace(std::make_pair<std::string, std::string>(key[i], value[i])); }
    return callback(data);
  }
  static int stub0(void *cb, int num, char **value, char **key) { return 0; }
  void exec(std::string sql, callback_t callback) const {
    char *err = 0;
    auto rc   = sqlite3_exec(db, sql.c_str(), Sqlite3::stub, &callback, &err);
    if (err) {
      Log::error("Sqlite3", "%s", err);
      auto exp = Sqlite3Error(err);
      sqlite3_free((void *)err);
      throw exp;
    }
  }
  void exec0(std::string sql) const {
    char *err = 0;
    auto rc   = sqlite3_exec(db, sql.c_str(), Sqlite3::stub0, nullptr, &err);
    if (err) {
      Log::error("Sqlite3", "%s", err);
      auto exp = Sqlite3Error(err);
      sqlite3_free((void *)err);
      throw exp;
    }
  }
  std::shared_ptr<Sqlite3Stmt> prepare(std::string sql) const {
    sqlite3_stmt *stmt;
    auto err = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (err != SQLITE_OK) {
      Log::error("Sqlite3", "prepare(%s) Err: %s", sql.c_str(), sqlite3_errstr(err));
      throw Sqlite3Error(sqlite3_errstr(err));
    }
    return std::make_shared<Sqlite3Stmt>(stmt);
  }
};

extern "C" sqlite3 *getMasterDB();

Sqlite3 global{ getMasterDB() }, world{ "worlds/" + std::string(mcpelauncher_property_get("level-dir", "world")) + "/chai.db" },
    memdb{ ":memory:" };

extern "C" void mod_init() {
  ModulePtr m(new Module());

  utility::add_class<Sqlite3StmtStepResult>(
      *m, "Sqlite3StmtStepResult",
      { { Sqlite3StmtStepResult::OK, "OK" }, { Sqlite3StmtStepResult::ROW, "ROW" }, { Sqlite3StmtStepResult::DONE, "DONE" } });
  utility::add_class<Sqlite3>(*m, "Sqlite3", {},
                              { { fun(&Sqlite3::exec), "exec" }, { fun(&Sqlite3::exec0), "exec" }, { fun(&Sqlite3::prepare), "prepare" } });
  utility::add_class<Sqlite3Stmt>(*m, "Sqlite3Stmt", {},
                                  { { fun(&Sqlite3Stmt::step), "step" },
                                    { fun(&Sqlite3Stmt::reset), "reset" },
                                    { fun(&Sqlite3Stmt::clear), "clear" },
                                    { fun(&Sqlite3Stmt::get), "get" },
                                    { fun(&Sqlite3Stmt::bind), "bind" },
                                    { fun(&Sqlite3Stmt::bindNamed), "bind" } });
  m->add_global_const(const_var(&global), "GlobalDB");
  m->add_global_const(const_var(&world), "WorldDB");
  m->add_global_const(const_var(&memdb), "MemDB");

  loadModule(m);
}