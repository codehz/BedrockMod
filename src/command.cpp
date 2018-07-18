#include <polyfill.h>

#include <chaiscript/chaiscript.hpp>
#include <functional>
#include <hook.h>
#include <iomanip>
#include <log.h>
#include <map>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <tuple>
#include <utility>

#include <minecraft/command/Command.h>
#include <minecraft/command/CommandMessage.h>
#include <minecraft/command/CommandOutput.h>
#include <minecraft/command/CommandParameterData.h>
#include <minecraft/command/CommandRegistry.h>

#include "player.h"

#define VALID(x) (reinterpret_cast<int const &>(x) != 0)

struct EnumData {
  std::string name;
  std::vector<unsigned int> vec;
};
struct ParamData {
  std::string name;
  unsigned type;
  bool optional;
};
struct OverloadData {
  std::vector<ParamData> params;
};
struct CommandData {
  std::string name;                    // 0x00
  std::string description;             // 0x04
  unsigned char flag, permission;      // 0x08 0x09
  unsigned char padding[2];            // 0x0A 0x0B
  std::vector<OverloadData> overloads; // 0x0C
  unsigned int unk;
};

struct EnumDef {
  std::string name;
  std::vector<std::string> values;
  EnumDef(std::string name)
      : name(name) {}
  void add(std::string value) { values.push_back(value); }
};
struct ParamDef {
  std::string name;
  bool optional;
  ParamDef(std::string name, bool optional)
      : name(name)
      , optional(optional) {}
  virtual ~ParamDef() {}
};
struct BasicParamDef : ParamDef {
  BasicParamDef(std::string name, bool optional)
      : ParamDef(name, optional) {}
  virtual int type() = 0;
};
#define defType(T, N)                                                                                                                                \
  struct T##ParamDef : BasicParamDef {                                                                                                               \
    T##ParamDef(std::string name, bool optional)                                                                                                     \
        : BasicParamDef(name, optional) {}                                                                                                           \
    virtual int type() { return N; }                                                                                                                 \
  }

defType(Int, 0x01);
defType(Float, 0x02);
defType(Value, 0x03);
defType(WildcardInt, 0x04);
defType(Target, 0x05);
defType(WildcardTarget, 0x06);
defType(String, 0x0f);
defType(Position, 0x10);
defType(Message, 0x13);
defType(RawText, 0x15);
defType(Json, 0x18);
defType(Command, 0x1f);

#undef defType
struct EnumParamDef : ParamDef {
  EnumParamDef(std::string name, bool optional, EnumDef def)
      : ParamDef(name, optional)
      , enumdef(def) {}
  EnumDef enumdef;
};
struct OverloadDef {
  std::vector<std::shared_ptr<ParamDef>> params;
  void add(const std::shared_ptr<ParamDef> param) { params.push_back(param); }
};

struct CommandSender;

struct CustomCommand {
  std::string desc;
  unsigned char flag, perm;
  std::vector<OverloadDef> overloads;
  std::function<chaiscript::Boxed_Value(CommandSender orig, std::string)> execute;
  CustomCommand(std::string desc, unsigned char flag, unsigned char perm,
                std::function<chaiscript::Boxed_Value(CommandSender orig, std::string)> execute)
      : desc(desc)
      , flag(flag)
      , perm(perm)
      , execute(execute) {}
  void add(const OverloadDef &data) { overloads.push_back(std::move(data)); }
};

static std::map<std::string, std::shared_ptr<CustomCommand>> CustomCommandMap;

static void *(*$availableCommand)(void *self, std::vector<std::string> &vec1, std::vector<std::string> &vec2, std::vector<EnumData> &enumdatas,
                                  std::vector<CommandData> &commands);

void *availableCommand(void *self, std::vector<std::string> &enumValues, std::vector<std::string> &postfixs, std::vector<EnumData> &enumdatas,
                       std::vector<CommandData> &commands) {
  // int index = 0;
  // for (auto s : enumValues)
  // {
  //   if (reinterpret_cast<int &>(s) != 0)
  //   {
  //     Log::debug("CMD", "value[%04x] %s", index++, s.c_str());
  //   }
  // }
  // index = 0;
  // for (auto enumdata : enumdatas)
  // {
  //   std::stringstream ss;
  //   ss << "enum[" << std::setfill('0') << std::setw(4) << std::hex << index++ << "]" << enumdata.name << "\n\t";
  //   for (auto ui : enumdata.vec)
  //   {
  //     ss << " " << std::setfill('0') << std::setw(4) << std::hex << ui;
  //   }
  //   Log::debug("CMD", " %s", ss.str().c_str());
  // }
  // for (auto commanddata : commands)
  // {
  //   Log::debug("CMD", "command %s", commanddata.name.c_str());
  //   Log::debug("CMD", "\tdesc %s", commanddata.description.c_str());
  //   Log::debug("CMD", "\tflag %d", commanddata.flag);
  //   Log::debug("CMD", "\tperm %d", commanddata.permission);
  //   for (auto overload : commanddata.overloads)
  //   {
  //     Log::debug("CMD", "\toverload");
  //     for (auto param : overload.params)
  //     {
  //       Log::debug("CMD", "\t\tparam: %-20s %08x %d", param.name.c_str(), param.type, param.optional);
  //     }
  //   }
  //   Log::debug("CMD", "\tunk %d", commanddata.unk);
  // }

  // commands.push_back(CommandData{
  //     "ping",
  //     "Ping Server",
  //     0,
  //     0,
  //     {0, 0},
  //     {{}},
  //     255});

  for (auto cmd : CustomCommandMap) {
    auto &def = *cmd.second;
    CommandData commandData;
    commandData.name        = cmd.first;
    commandData.description = def.desc;
    commandData.flag        = def.flag;
    commandData.permission  = def.perm;
    for (auto overload : def.overloads) {
      OverloadData overloadData;
      for (auto param : overload.params) {
        ParamData paramData;
        paramData.name     = param->name;
        paramData.optional = param->optional;
        auto enumparam     = std::dynamic_pointer_cast<EnumParamDef>(param);
        if (enumparam) {
          EnumData enumData;
          enumData.name = enumparam->enumdef.name;
          for (auto value : enumparam->enumdef.values) {
            auto idx = std::find(enumValues.begin(), enumValues.end(), value);
            if (idx != enumValues.end()) {
              auto index = idx - enumValues.begin();
              enumData.vec.push_back(index);
            } else {
              enumValues.push_back(value);
              enumData.vec.push_back(enumValues.size() - 1);
            }
          }
          enumdatas.push_back(enumData);
          paramData.type = 0x00300000 + enumdatas.size() - 1;
        } else {
          auto basicparam = std::dynamic_pointer_cast<BasicParamDef>(param);
          paramData.type  = 0x00100000 + basicparam->type();
        }
        overloadData.params.push_back(paramData);
      }
      commandData.overloads.push_back(overloadData);
    }
    commandData.unk = -1;
    commands.push_back(commandData);
  }
  return $availableCommand(self, enumValues, postfixs, enumdatas, commands);
}

struct CommandContext {
  const char *content;
  CommandOrigin &origin;
  int unk;
};

enum struct CommandOriginType {
  PLAYER            = 0,
  BLOCK             = 1,
  MINECARTBLOCK     = 2,
  DEVCONSOLE        = 3,
  TEST              = 4,
  AUTOMATION_PLAYER = 5,
  CLIENT_AUTOMATION = 6,
  DEDICATED_SERVER  = 7,
  ENTITY            = 8,
  VIRTUAL           = 9,
  GAME_ARGUMENT     = 10,
  ENTITY_SERVER     = 11
};

struct CommandOrigin {
  virtual ~CommandOrigin();
  virtual void getRequestId()               = 0;
  virtual void getName()                    = 0;
  virtual void getBlockPosition()           = 0;
  virtual void getWorldPosition()           = 0;
  virtual Level *getLevel() const           = 0;
  virtual void *getDimension()              = 0;
  virtual Entity *getEntity() const         = 0;
  virtual void *getPermissionsLevel() const = 0;
  virtual void *clone() const               = 0;
  virtual bool canCallHiddenCommands() const;
  virtual bool hasChatPerms() const;
  virtual bool hasTellPerms() const;
  virtual bool canUseAbility(std::string const &) const;
  virtual void *getSourceId() const;
  virtual void *getSourceSubId() const;
  virtual void *getOutputReceiver() const;
  virtual CommandOriginType getOriginType() const = 0;
  virtual void *toCommandOriginData() const;
  virtual void *getUUID() const;
  // virtual void *_setUUID(mce::UUID const &);
};

struct CommandSender {
  CommandOrigin const *orig;
  CommandSender(CommandOrigin const &orig)
      : orig(&orig) {}
  bool isPlayer() { return orig->getOriginType() == CommandOriginType::PLAYER; }
  ServerPlayer *getPlayer() {
    if (isPlayer()) {
      Log::debug("CMD", "GetEntity: %08x", orig->getEntity());
      return static_cast<ServerPlayer *>(orig->getEntity());
    } else
      throw new std::runtime_error("CommandOrigin is not player!");
  }
};

static void (*$createCommandContext)(CommandContext *ctx, std::string const &content, CommandOrigin &orig, int unk);

void createCommandContext(CommandContext *ctx, std::string &content, CommandOrigin &orig, int unk) {
  if (VALID(content)) {
    std::istringstream buf(content.substr(1));
    std::istream_iterator<std::string> beg(buf);
    auto it = CustomCommandMap.find(*beg);
    if (it != CustomCommandMap.end()) { content = "/custom " + content; }
  }
  $createCommandContext(ctx, content, orig, unk);
}

struct StubCommand : Command {
  CommandMessage msg;
  ~StubCommand() {}
  void execute(CommandOrigin const &orig, CommandOutput &outp) {
    if (VALID(msg)) {
      auto content = msg.getMessage(orig);
      std::istringstream buf(content.substr(1));
      std::istream_iterator<std::string> beg(buf);
      auto it = CustomCommandMap.find(*beg);
      if (it != CustomCommandMap.end()) {
        try {
          auto boxed = it->second->execute({ orig }, content);
          if (boxed.is_type(chaiscript::user_type<std::string>()))
            content = "/echo " + chaiscript::boxed_cast<std::string>(boxed);
          else
            content = "/echo";
        } catch (const std::exception &e) {
          Log::error("CMD", "Error in callback: %s", e.what());
          content = "/echo Error.";
        }
      }
    }
    outp.success();
  }
};

struct EchoCommand : Command {
  CommandMessage msg;
  ~EchoCommand() {}
  void execute(CommandOrigin const &orig, CommandOutput &outp) {
    if (VALID(msg)) { outp.addMessage(msg.getMessage(orig)); }
    outp.success();
  }
};

struct SayCommand {
  IMPL_STATIC(void, setup, CommandRegistry &reg) {
    Log::info("CMD", "Setup Command");
    reg.registerCommand("custom", "STUB Command.", (CommandPermissionLevel)0, (CommandFlag)0, (CommandFlag)0);
    reg.registerOverload<StubCommand>("custom", CommandVersion(0, INT32_MAX),
                                      CommandParameterData(CommandMessage::type_id(), &CommandRegistry::parse<CommandMessage>, "stub",
                                                           (CommandParameterDataType)0, nullptr, offsetof(StubCommand, msg), true, -1));
    reg.registerCommand("echo", "ECHO Command.", (CommandPermissionLevel)0, (CommandFlag)0, (CommandFlag)0);
    reg.registerOverload<EchoCommand>("echo", CommandVersion(0, INT32_MAX),
                                      CommandParameterData(CommandMessage::type_id(), &CommandRegistry::parse<CommandMessage>, "content",
                                                           (CommandParameterDataType)0, nullptr, offsetof(StubCommand, msg), true, -1));
    $setup()(reg);
  }
};

template <typename T> void regParam(const chaiscript::ModulePtr &m, std::string name) {
  m->add(chaiscript::user_type<T>(), name);
  m->add(chaiscript::base_class<ParamDef, T>());
  m->add(chaiscript::constructor<T(std::string, bool)>(), name);
}

std::shared_ptr<CustomCommand> defcmd(std::string name, std::string desc, unsigned char flag, unsigned char perm,
                                      std::function<chaiscript::Boxed_Value(CommandSender orig, std::string)> execute) {
  CustomCommandMap.emplace(std::make_pair(name, std::make_shared<CustomCommand>(desc, flag, perm, execute)));
  return CustomCommandMap.find(name)->second;
}

struct PlayerCommandSender : CommandSender {};

CHAISCRIPT_MODULE_EXPORT chaiscript::ModulePtr create_chaiscript_module_command() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  HOOK(SayCommand, setup);
  void *handle = dlopen("libminecraftpe.so", RTLD_LAZY);
  mcpelauncher_hook(dlsym(handle, "_ZN23AvailableCommandsPacketC2ERKSt6vectorISsSaISsEES4_OS0_INS_8EnumDataESaIS5_EEOS0_INS_11CommandDataESaIS9_EE"),
                    reinterpret_cast<void *>(availableCommand), (void **)&$availableCommand);
  mcpelauncher_hook(dlsym(handle, "_ZN14CommandContextC2ERKSsSt10unique_ptrI13CommandOriginSt14default_deleteIS3_EEi"),
                    reinterpret_cast<void *>(createCommandContext), (void **)&$createCommandContext);
  m->add(chaiscript::user_type<CommandSender>(), "CommandOrigin");
  m->add(chaiscript::fun(&CommandSender::isPlayer), "isPlayer");
  m->add(chaiscript::fun(&CommandSender::getPlayer), "getPlayer");
  m->add(chaiscript::user_type<EnumDef>(), "EnumDef");
  m->add(chaiscript::constructor<EnumDef(std::string)>(), "EnumDef");
  m->add(chaiscript::fun(&EnumDef::add), "add");
  m->add(chaiscript::user_type<ParamDef>(), "Param");
  regParam<IntParamDef>(m, "Param_int");
  regParam<FloatParamDef>(m, "Param_float");
  regParam<ValueParamDef>(m, "Param_value");
  regParam<WildcardIntParamDef>(m, "Param_int_wildcard");
  regParam<TargetParamDef>(m, "Param_target");
  regParam<WildcardTargetParamDef>(m, "Param_target_wildcard");
  regParam<StringParamDef>(m, "Param_string");
  regParam<PositionParamDef>(m, "Param_position");
  regParam<MessageParamDef>(m, "Param_message");
  regParam<RawTextParamDef>(m, "Param_raw");
  regParam<JsonParamDef>(m, "Param_json");
  regParam<CommandParamDef>(m, "Param_command");
  m->add(chaiscript::user_type<EnumParamDef>(), "Param_enum");
  m->add(chaiscript::constructor<EnumParamDef(std::string, bool, EnumDef)>(), "Param_enum");
  m->add(chaiscript::base_class<ParamDef, EnumParamDef>());
  m->add(chaiscript::user_type<OverloadDef>(), "CommandOverload");
  m->add(chaiscript::constructor<OverloadDef()>(), "CommandOverload");
  m->add(chaiscript::fun(&OverloadDef::add), "add");
  m->add(chaiscript::user_type<CustomCommand>(), "CustomCommand");
  m->add(chaiscript::fun(&defcmd), "defcmd");
  m->add(chaiscript::fun(&CustomCommand::add), "add");
  return m;
}