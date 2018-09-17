// Deps: out/script_command.so: out/script_base.so
#include "../base/main.h"
#include <api.h>

#include <minecraft/command/Command.h>
#include <minecraft/command/CommandMessage.h>
#include <minecraft/command/CommandOutput.h>
#include <minecraft/command/CommandParameterData.h>
#include <minecraft/command/CommandRegistry.h>
#include <minecraft/command/CommandVersion.h>

#include <StaticHook.h>

#include <SimpleJit.h>

#include <string>
#include <unordered_map>

struct TestCommand;

MAKE_FOREIGN_TYPE(TestCommand *, "command");
MAKE_FOREIGN_TYPE(CommandOrigin *, "command-orig");
MAKE_FOREIGN_TYPE(CommandOutput *, "command-outp");

namespace scm {
template <> struct convertible<TestCommand *> : foreign_object_is_convertible<TestCommand *> {};
template <> struct convertible<CommandOrigin *> : foreign_object_is_convertible<CommandOrigin *> {};
template <> struct convertible<CommandOutput *> : foreign_object_is_convertible<CommandOutput *> {};
} // namespace scm

struct ParameterDef {
  std::string name, symbol;
  size_t size;
  void (*init)(void *);
  void (*deinit)(void *);
  SCM (*fetch)(TestCommand *);
};

struct MyCommandVTable {
  std::vector<ParameterDef> defs;
  std::function<void(TestCommand *self, CommandOrigin *, CommandOutput *)> exec;

  template <typename... T>
  MyCommandVTable(std::function<void(TestCommand *self, CommandOrigin *, CommandOutput *)> exec, T... ts)
      : exec(exec)
      , defs(ts...) {}
};

struct TestCommand : Command {
  MyCommandVTable *vt;
  void *stub[0];

  ~TestCommand() {
    size_t offset = 0;
    for (auto def : vt->defs) {
      def.deinit(stub[offset]);
      offset += def.size;
    }
  }

  void execute(CommandOrigin const &orig, CommandOutput &outp) { vt->exec(this, const_cast<CommandOrigin *>(&orig), &outp); }

  TestCommand(MyCommandVTable *vt)
      : Command() {
    this->vt      = vt;
    size_t offset = 0;
    for (auto def : vt->defs) {
      def.init(stub[offset]);
      offset += def.size;
    }
  }

  static std::unique_ptr<TestCommand> create(MyCommandVTable *vt) {
    size_t size = 0;
    for (auto def : vt->defs) size += def.size;
    return std::unique_ptr<TestCommand>(new (malloc(sizeof(TestCommand) + size)) TestCommand(vt));
  }
};

struct PingCommand : Command {
  ~PingCommand() {}
  void execute(CommandOrigin const &orig, CommandOutput &outp) {
    outp.addMessage("pong");
    outp.success();
  }
  static void setup(CommandRegistry &reg) {
    reg.registerCommand("ping", "ping", (CommandPermissionLevel)0, (CommandFlag)0, (CommandFlag)0);
    reg.registerCustomOverload("ping", CommandVersion(0, INT32_MAX), CommandRegistry::allocateCommand<PingCommand>);
  }
};

struct MinecraftCommands {
  CommandRegistry &getRegistry();
};

struct CommandRegistryApply {
  std::string name, description;
  int level;
  std::vector<MyCommandVTable *> vts;
};

static CommandRegistry *registry;

THook(void, _ZN9XPCommand5setupER15CommandRegistry, CommandRegistry &reg) {
  original(reg);
  registry = &reg;
}

static void handleCommandApply(CommandRegistryApply &apply) {
  registry->registerCommand(apply.name, apply.description.c_str(), (CommandPermissionLevel)apply.level, (CommandFlag)0, (CommandFlag)0);
  for (auto vt : apply.vts) {
    registry->registerCustomOverload(apply.name.c_str(), CommandVersion(0, INT32_MAX),
                                     gen_function([=]() -> std::unique_ptr<Command> { return TestCommand::create(vt); }));
  }
}

SCM_DEFINE_PUBLIC(register_simple_command, "reg-simple-command", 4, 0, 0,
                  (scm::val<char *> name, scm::val<char *> description, scm::val<int> level,
                   scm::callback<void, TestCommand *, CommandOrigin *, CommandOutput *> cb),
                  "Register simple command") {
  CommandRegistryApply apply{ .name = name.get(), .description = description.get(), .level = level.get() };
  apply.vts.emplace_back(new MyCommandVTable(cb));
  handleCommandApply(apply);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(outp_add, "outp-add", 2, 0, 0, (scm::val<CommandOutput *> outp, scm::val<char *> msg), "Add message to command output") {
  outp->addMessage(msg.get());
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(outp_success, "outp-success", 1, 1, 0, (scm::val<CommandOutput *> outp, scm::val<char *> msg), "Set command output to success") {
  if (scm_is_string(msg.scm)) outp->addMessage(msg.get());
  outp->success();
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(outp_error, "outp-error", 2, 0, 0, (scm::val<CommandOutput *> outp, scm::val<char *> msg), "Set command output to success") {
  outp->error(msg.get());
  return SCM_UNSPECIFIED;
}

struct CommandOrigin {
  virtual ~CommandOrigin();
  virtual std::string getRequestId();
  virtual std::string getName();
  virtual BlockPos getBlockPosition();
  virtual Vec3 getWorldPosition();
  virtual Level &getLevel();
  virtual void *getDimension();
  virtual Actor &getEntity();
  virtual int getPermissionsLevel();
  virtual CommandOrigin *clone();
  virtual bool canCallHiddenCommands();
  virtual bool hasChatPerms();
  virtual bool hasTellPerms();
  virtual bool canUseAbility(std::string);
  virtual void *getSourceId();
  virtual void *getSourceSubId();
  virtual void *getOutputReceiver();
  virtual int getOriginType();
  virtual void *toCommandOriginData();
  virtual mce::UUID const &getUUID();
  virtual bool mayOverrideName();
};

SCM_DEFINE_PUBLIC(orig_type, "orig-type", 1, 0, 0, (scm::val<CommandOrigin *> orig), "Get CommandOrigin type") {
  return scm::to_scm(orig->getOriginType());
}

SCM_DEFINE_PUBLIC(orig_player, "orig-player", 1, 0, 0, (scm::val<CommandOrigin *> orig), "Get CommandOrigin type") {
  if (orig->getOriginType() == 0) { return scm::to_scm((ServerPlayer *)&orig->getEntity()); }
  return SCM_BOOL_F;
}

PRELOAD_MODULE("minecraft command") {
#ifndef DIAG
#include "main.x"
#endif
}