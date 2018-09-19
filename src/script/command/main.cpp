// Deps: out/script_command.so: out/script_base.so
#include "../base/main.h"
#include <api.h>

#include <csignal>

#include <minecraft/command/Command.h>
#include <minecraft/command/CommandMessage.h>
#include <minecraft/command/CommandOutput.h>
#include <minecraft/command/CommandParameterData.h>
#include <minecraft/command/CommandRegistry.h>
#include <minecraft/command/CommandVersion.h>

#include <StaticHook.h>

#include <SimpleJit.h>

#include <stack>
#include <string>
#include <unordered_map>

struct TestCommand;
struct MyCommandVTable;
struct ParameterDef;

MAKE_FOREIGN_TYPE(TestCommand *, "command");
MAKE_FOREIGN_TYPE(CommandOrigin *, "command-orig");
MAKE_FOREIGN_TYPE(CommandOutput *, "command-outp");
MAKE_FOREIGN_TYPE(MyCommandVTable *, "command-vtable");
MAKE_FOREIGN_TYPE(ParameterDef *, "parameter-def");

namespace scm {
template <> struct convertible<TestCommand *> : foreign_object_is_convertible<TestCommand *> {};
template <> struct convertible<CommandOrigin *> : foreign_object_is_convertible<CommandOrigin *> {};
template <> struct convertible<CommandOutput *> : foreign_object_is_convertible<CommandOutput *> {};
template <> struct convertible<MyCommandVTable *> : foreign_object_is_convertible<MyCommandVTable *> {};
template <> struct convertible<ParameterDef *> : foreign_object_is_convertible<ParameterDef *> {};
} // namespace scm

struct ParameterDef {
  size_t size;
  std::string name;
  typeid_t<CommandRegistry> type;
  bool (CommandRegistry::*parser)(void *, CommandRegistry::ParseToken const &, CommandOrigin const &, int, std::string &,
                                  std::vector<std::string> &) const;
  void (*init)(void *);
  void (*deinit)(void *);
  SCM (*fetch)(void *, CommandOrigin *);
};

struct MyCommandVTable {
  std::vector<ParameterDef *> defs;
  std::function<void(TestCommand *self, CommandOrigin *, CommandOutput *)> exec;

  template <typename... T>
  MyCommandVTable(std::function<void(TestCommand *self, CommandOrigin *, CommandOutput *)> exec, T... ts)
      : exec(exec)
      , defs(ts...) {}
};

struct TestCommand : Command {
  MyCommandVTable *vt;

  virtual void execute(CommandOrigin const &orig, CommandOutput &outp) { vt->exec(this, const_cast<CommandOrigin *>(&orig), &outp); }

  TestCommand(MyCommandVTable *vt)
      : Command() {
    this->vt      = vt;
    size_t offset = 0;
    for (auto def : vt->defs) {
      if (def->init) def->init((void *)((size_t)this + sizeof(TestCommand) + offset));
      offset += def->size;
    }
  }

  ~TestCommand() {
    size_t offset = 0;
    for (auto def : vt->defs) {
      if (def->deinit) def->deinit((void *)((size_t)this + sizeof(TestCommand) + offset));
      offset += def->size;
    }
  }

  static TestCommand *create(MyCommandVTable *vt) {
    size_t size = 0;
    for (auto def : vt->defs) size += def->size;
    auto ptr = new (malloc(sizeof(TestCommand) + size)) TestCommand(vt);
    return ptr;
  }
};

SCM_DEFINE_PUBLIC(command_fetch, "command-args", 2, 0, 0, (scm::val<TestCommand *> cmd, scm::val<CommandOrigin *> orig), "Get command arguments") {
  std::stack<SCM> st;
  size_t pos = (size_t)cmd.get() + sizeof(TestCommand);
  for (auto &def : cmd->vt->defs) {
    st.push(def->fetch((void *)pos, orig));
    pos += def->size;
  }
  SCM list = SCM_EOL;
  while (!st.empty()) {
    list = scm_cons(st.top(), list);
    st.pop();
  }
  return list;
}

static ParameterDef *messageParameter(temp_string const &name) {
  return new ParameterDef{ .size   = sizeof(CommandMessage),
                           .name   = name,
                           .type   = CommandMessage::type_id(),
                           .parser = &CommandRegistry::parse<CommandMessage>,
                           .init   = (void (*)(void *))dlsym(MinecraftHandle(), "_ZN14CommandMessageC2Ev"),
                           .deinit = (void (*)(void *))dlsym(MinecraftHandle(), "_ZN14CommandMessageD2Ev"),
                           .fetch  = [](void *self, CommandOrigin *orig) { return scm::to_scm(((CommandMessage *)self)->getMessage(*orig)); } };
}

SCM_DEFINE_PUBLIC(parameter_message, "parameter-message", 1, 0, 0, (scm::val<char *> name), "Message parameter") {
  return scm::to_scm(messageParameter(name));
}

struct CommandSelectorBase {
  int filler[0x90];
  CommandSelectorBase(bool isPlayer);

  std::shared_ptr<std::vector<Actor *>> newResults(CommandOrigin const &) const;

  static auto fetch(void *self, CommandOrigin *orig) {
    auto selector = ((CommandSelectorBase *)self);
    auto result   = selector->newResults(*orig);
    SCM list      = SCM_EOL;
    for (auto actor : *result) {
      auto player = dynamic_cast<ServerPlayer *>(actor);
      if (player)
        list = scm_cons(scm::to_scm(player), list);
      else
        list = scm_cons(scm::to_scm(actor), list);
    }
    return list;
  }
};

template <typename T> struct CommandSelector {
  CommandSelector();

  static auto type() { return type_id<CommandRegistry, CommandSelector<T>>(); }
};

static ParameterDef *selectorParameter(temp_string const &name, bool playerOnly) {
  auto tid    = playerOnly ? CommandSelector<Player>::type() : CommandSelector<Actor>::type();
  auto parser = playerOnly ? &CommandRegistry::parse<CommandSelector<Player>> : &CommandRegistry::parse<CommandSelector<Actor>>;
  return new ParameterDef{
    .size   = sizeof(CommandSelectorBase),
    .name   = name,
    .type   = tid,
    .parser = parser,
    .init   = (void (*)(void *))dlsym(MinecraftHandle(), playerOnly ? "_ZN15CommandSelectorI6PlayerEC2Ev" : "_ZN15CommandSelectorI5ActorEC2Ev"),
    .deinit = (void (*)(void *))dlsym(MinecraftHandle(), playerOnly ? "_ZN15CommandSelectorI6PlayerED2Ev" : "_ZN15CommandSelectorI5ActorED2Ev"),
    .fetch  = &CommandSelectorBase::fetch
  };
}

SCM_DEFINE_PUBLIC(parameter_selector, "parameter-selector", 1, 1, 0, (scm::val<char *> name, scm::val<bool> playerOnly), "Selector parameter") {
  return scm::to_scm(selectorParameter(name, playerOnly));
}

template <typename T> static void geninit(void *str) { new (str) T(); }

static SCM fetchString(void *self, CommandOrigin *orig) { return scm::to_scm(*(std::string *)self); }

static ParameterDef *stringParameter(temp_string const &name) {
  return new ParameterDef{ .size   = sizeof(std::string),
                           .name   = name,
                           .type   = type_id<CommandRegistry, std::string>(),
                           .parser = &CommandRegistry::parse<std::string>,
                           .init   = (void (*)(void *))geninit<std::string>,
                           .deinit = (void (*)(void *))dlsym(MinecraftHandle(), "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev"),
                           .fetch  = &fetchString };
}

SCM_DEFINE_PUBLIC(parameter_string, "parameter-string", 1, 0, 0, (scm::val<char *> name), "String parameter") {
  return scm::to_scm(stringParameter(name));
}

template <typename T> static SCM simpleFetch(void *v, CommandOrigin *orig) { return scm::to_scm(*(T *)v); }

template <typename T> static ParameterDef *simpleParameter(temp_string const &name) {
  return new ParameterDef{ .size   = sizeof(T),
                           .name   = name,
                           .type   = type_id<CommandRegistry, T>(),
                           .parser = &CommandRegistry::parse<T>,
                           .init   = nullptr,
                           .deinit = nullptr,
                           .fetch  = simpleFetch<T> };
}

SCM_DEFINE_PUBLIC(parameter_int, "parameter-int", 1, 0, 0, (scm::val<char *> name), "Integer parameter") {
  return scm::to_scm(simpleParameter<int>(name));
}

SCM_DEFINE_PUBLIC(parameter_float, "parameter-float", 1, 0, 0, (scm::val<char *> name), "Float parameter") {
  return scm::to_scm(simpleParameter<float>(name));
}

struct CommandRawText {
  std::string value;
};

static ParameterDef *textParameter(temp_string const &name) {
  return new ParameterDef{ .size   = sizeof(std::string),
                           .name   = name,
                           .type   = type_id<CommandRegistry, CommandRawText>(),
                           .parser = &CommandRegistry::parse<CommandRawText>,
                           .init   = (void (*)(void *))geninit<std::string>,
                           .deinit = (void (*)(void *))dlsym(MinecraftHandle(), "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev"),
                           .fetch  = &fetchString };
}

SCM_DEFINE_PUBLIC(parameter_text, "parameter-text", 1, 0, 0, (scm::val<char *> name), "String parameter") { return scm::to_scm(textParameter(name)); }

struct CommandPosition {
  char filler[16];
  Vec3 getPosition(CommandOrigin const &) const;
  static SCM fetch(void *self, CommandOrigin *orig) { return scm::to_scm(((CommandPosition *)self)->getPosition(*orig)); }
};

static auto positionParameter(temp_string const &name) {
  return new ParameterDef{
    .size   = sizeof(CommandPosition),
    .name   = name,
    .type   = type_id<CommandRegistry, CommandPosition>(),
    .parser = &CommandRegistry::parse<CommandPosition>,
    .init   = geninit<CommandPosition>,
    .deinit = nullptr,
    .fetch  = &CommandPosition::fetch,
  };
}

SCM_DEFINE_PUBLIC(parameter_position, "parameter-position", 1, 0, 0, (scm::val<char *> name), "Position parameter") {
  return scm::to_scm(positionParameter(name));
}

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
                                     gen_function([=]() -> std::unique_ptr<Command> { return std::unique_ptr<Command>(TestCommand::create(vt)); }),
                                     [&](CommandRegistry::Overload &overload) {
                                       size_t offset = sizeof(TestCommand);
                                       for (auto p : vt->defs) {
                                         overload.params.emplace_back(CommandParameterData(p->type, p->parser, p->name.c_str(),
                                                                                           (CommandParameterDataType)0, nullptr, offset, false, -1));
                                         offset += p->size;
                                       }
                                     });
  }
}

SCM_DEFINE_PUBLIC(register_simple_command, "reg-simple-command", 4, 0, 0,
                  (scm::val<char *> name, scm::val<char *> description, scm::val<int> level,
                   scm::callback<void, TestCommand *, CommandOrigin *, CommandOutput *> cb),
                  "Register simple command") {
  CommandRegistryApply apply{ .name = name.get(), .description = description.get(), .level = level.get() };
  apply.vts.emplace_back(new MyCommandVTable(cb));
  scm_gc_protect_object(cb.scm);
  handleCommandApply(apply);
  return SCM_BOOL_T;
}

SCM_DEFINE_PUBLIC(make_vtable, "command-vtable", 2, 0, 0,
                  (scm::slist<scm::val<ParameterDef *>> params, scm::callback<void, TestCommand *, CommandOrigin *, CommandOutput *> cb),
                  "Make vtable for custom command") {
  scm_gc_protect_object(cb.scm);
  auto ret = new MyCommandVTable(cb);
  for (auto def : params) { ret->defs.emplace_back(def); }
  return scm::to_scm(ret);
}

SCM_DEFINE_PUBLIC(register_command, "reg-command", 4, 0, 0,
                  (scm::val<char *> name, scm::val<char *> description, scm::val<int> level, scm::slist<MyCommandVTable *> vts),
                  "Register simple command") {
  CommandRegistryApply apply{ .name = name.get(), .description = description.get(), .level = level.get() };
  for (auto vt : vts) { apply.vts.emplace_back(vt); }
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