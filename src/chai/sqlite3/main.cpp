#include <api.h>

#include <log.h>

#include <functional>
#include <vector>

#include <minecraft/net/UUID.h>

#include <sqlite3/sqlite3.h>

#include <sys/stat.h>

using namespace chaiscript;
using namespace mce;

static void valid_uuid(sqlite3_context *context, int argc, sqlite3_value **argv) {
  auto len   = sqlite3_value_bytes(argv[0]);
  auto puuid = (UUID *)sqlite3_value_blob(argv[0]);
  sqlite3_result_int(context, len == sizeof(UUID) && puuid->least != 0 && puuid->most != 0 ? 1 : 0);
}

static void uuid_to_blob(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::string uuid_str = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
  auto uuid            = UUID::fromString(uuid_str);
  sqlite3_result_blob(context, &uuid, sizeof(UUID), SQLITE_TRANSIENT);
}

static void uuid_to_text(sqlite3_context *context, int argc, sqlite3_value **argv) {
  auto len   = sqlite3_value_bytes(argv[0]);
  auto puuid = (UUID *)sqlite3_value_blob(argv[0]);
  if (len != sizeof(UUID)) { return sqlite3_result_error(context, "UUID must be 16 bytes", -1); }
  auto str = puuid->asString();
  sqlite3_result_text(context, str.c_str(), -1, SQLITE_TRANSIENT);
}

void register_uuid(sqlite3 *db) {
  sqlite3_create_function(db, "valid_uuid", 1, SQLITE_UTF8, 0, valid_uuid, 0, 0);
  sqlite3_create_function(db, "uuid_to_blob", 1, SQLITE_UTF8, 0, uuid_to_blob, 0, 0);
  sqlite3_create_function(db, "uuid_to_text", 1, SQLITE_UTF8, 0, uuid_to_text, 0, 0);
}

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
    } else if (value.is_type(user_type<int>())) {
      sqlite3_bind_int(stmt, idx, boxed_cast<int>(value));
    } else if (value.is_type(user_type<int64_t>())) {
      sqlite3_bind_int64(stmt, idx, boxed_cast<int64_t>(value));
    } else if (value.is_type(user_type<float>())) {
      sqlite3_bind_double(stmt, idx, boxed_cast<float>(value));
    } else if (value.is_type(user_type<double>())) {
      sqlite3_bind_double(stmt, idx, boxed_cast<double>(value));
    } else if (value.is_type(user_type<std::string>())) {
      auto str = boxed_cast<std::string>(value);
      sqlite3_bind_text(stmt, idx, str.c_str(), str.length() + 1, SQLITE_TRANSIENT);
    } else if (value.is_type(user_type<UUID>())) {
      auto &uuid = boxed_cast<UUID const &>(value);
      sqlite3_bind_blob(stmt, idx, &uuid, sizeof(UUID), SQLITE_TRANSIENT);
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
    case SQLITE_BLOB: {
      auto size = sqlite3_column_bytes(stmt, col);
      if (size != sizeof(UUID)) return Boxed_Value(nullptr);
      return Boxed_Value(*(UUID *)sqlite3_column_blob(stmt, col));
    }
    default: return Boxed_Value(nullptr);
    }
  }
  ~Sqlite3Stmt() { sqlite3_finalize(stmt); }
};

int createPath(mode_t mode, std::string &path) {
  struct stat st;

  for (std::string::iterator iter = path.begin(); iter != path.end();) {
    std::string::iterator newIter = std::find(iter, path.end(), '/');
    std::string newPath           = std::string(path.begin(), newIter);

    if (stat(newPath.c_str(), &st) != 0) {
      if (mkdir(newPath.c_str(), mode) != 0 && errno != EEXIST) {
        Log::error("Sqlite3", "Cannot create folder [%s]: %s", newPath.c_str(), strerror(errno));
        return -1;
      }
    } else if (!S_ISDIR(st.st_mode)) {
      errno = ENOTDIR;
      Log::error("Sqlite3", "Path [%s] is not a dir", newPath.c_str());
      return -1;
    }

    iter = newIter;
    if (newIter != path.end()) ++iter;
  }
  return 0;
}

struct Sqlite3 {
  using callback_t = std::function<int(std::map<std::string, std::string> &smap)>;
  sqlite3 *db;
  Sqlite3(std::string name) {
    if (name != ":memory:") {
      auto dir = name.substr(0, name.find_last_of("/"));
      createPath(0755, dir);
    }
    auto result = sqlite3_open(name.c_str(), &db);
    if (result != SQLITE_OK) {
      auto msg = sqlite3_errmsg(db);
      Log::error("Sqlite3", "%s: %s", name.c_str(), msg);
      sqlite3_free((void *)msg);
    }
    char *err = 0;
    sqlite3_exec(db, "PRAGMA synchronous = OFF; PRAGMA journal_mode = MEMORY; PRAGMA foreign_keys = ON;", nullptr, nullptr, &err);
    if (err != nullptr) {
      Log::error("Sqlite3", "%s: %s", name.c_str(), err);
      sqlite3_free(err);
    }
    register_uuid(db);
  }
  Sqlite3(sqlite3 *db)
      : db(db) {
    register_uuid(db);
  }
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
      Log::error("Sqlite3", "prepare(%s) Err: %s", sql.c_str(), sqlite3_errmsg(db));
      throw Sqlite3Error(sqlite3_errmsg(db));
    }
    return std::make_shared<Sqlite3Stmt>(stmt);
  }
};

extern "C" sqlite3 *getMasterDB();

Sqlite3 profile{ getMasterDB() }, world{ "worlds/" + std::string(mcpelauncher_property_get("level-dir", "world")) + "/chai.db" }, memdb{ ":memory:" };

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
  m->add_global_const(const_var(&profile), "ProfileDB");
  m->add_global_const(const_var(&world), "WorldDB");
  m->add_global_const(const_var(&memdb), "MemDB");

  loadModule(m);
}