#include <api.h>

#include <StaticHook.h>
#include <fix/string.h>
#include <functional>
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

#include <base.h>

using namespace json;
using namespace chaiscript;

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
  std::function<Boxed_Value(CommandSender orig, std::string)> execute;
  CustomCommand(std::string desc, unsigned char flag, unsigned char perm, std::function<Boxed_Value(CommandSender orig, std::string)> execute)
      : desc(desc)
      , flag(flag)
      , perm(perm)
      , execute(execute) {}
  void add(const OverloadDef &data) { overloads.push_back(std::move(data)); }
};

struct CustomCommandWithPlayerSelector {
  std::string desc;
  unsigned char flag, perm;
  std::vector<OverloadDef> overloads;
  std::function<Boxed_Value(CommandSender orig, std::vector<ServerPlayer *>, std::string)> execute;
  CustomCommandWithPlayerSelector(std::string desc, unsigned char flag, unsigned char perm,
                                  std::function<Boxed_Value(CommandSender orig, std::vector<ServerPlayer *>, std::string)> execute)
      : desc(desc)
      , flag(flag)
      , perm(perm)
      , execute(execute) {}
  void add(const OverloadDef &data) { overloads.push_back(std::move(data)); }
};

std::map<std::string, std::shared_ptr<CustomCommand>> CustomCommandMap;
std::map<std::string, std::shared_ptr<CustomCommandWithPlayerSelector>> CustomCommandWithPlayerSelectorMap;

template <typename T>
void regCommand(T const &cmd, std::vector<std::string> &enumValues, std::vector<std::string> &postfixs, std::vector<EnumData> &enumdatas,
                std::vector<CommandData> &commands) {
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

TClasslessInstanceHook(void *, _ZN23AvailableCommandsPacketC2ERKSt6vectorISsSaISsEES4_OS0_INS_8EnumDataESaIS5_EEOS0_INS_11CommandDataESaIS9_EEOS0_INS_12SoftEnumDataESaISD_EE,
                       std::vector<std::string> &enumValues, std::vector<std::string> &postfixs, std::vector<EnumData> &enumdatas,
                       std::vector<CommandData> &commands, std::vector<EnumData> &softenumdatas) {
  for (auto cmd : CustomCommandMap) { regCommand(cmd, enumValues, postfixs, enumdatas, commands); }
  for (auto cmd : CustomCommandWithPlayerSelectorMap) { regCommand(cmd, enumValues, postfixs, enumdatas, commands); }
  return original(this, enumValues, postfixs, enumdatas, commands, softenumdatas);
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
  virtual void getRequestId()             = 0;
  virtual void getName()                  = 0;
  virtual void getBlockPosition()         = 0;
  virtual void getWorldPosition()         = 0;
  virtual Level *getLevel() const         = 0;
  virtual void *getDimension()            = 0;
  virtual Actor *getEntity() const       = 0;
  virtual int getPermissionsLevel() const = 0;
  virtual void *clone() const             = 0;
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
      return static_cast<ServerPlayer *>(orig->getEntity());
    } else
      throw std::runtime_error("CommandOrigin is not player!");
  }
  int getPermissionsLevel() { return orig->getPermissionsLevel(); }
};

void replaceAll(std::string &str, const std::string &from, const std::string &to) {
  if (from.empty()) return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

template <typename InputIt> std::string join(InputIt begin, InputIt end, const std::string &separator = ", ", const std::string &concluder = "") {
  std::ostringstream ss;

  if (begin != end) {
    ss << *begin++; // see 3.
  }

  while (begin != end) // see 3.
  {
    ss << separator;
    ss << *begin++;
  }

  ss << concluder;
  return ss.str();
}

TInstanceHook(void, _ZN14CommandContextC2ERKSsSt10unique_ptrI13CommandOriginSt14default_deleteIS3_EEi, CommandContext, std::string &content,
              CommandOrigin &orig, int unk) {
  if (VALID(content)) {
    std::istringstream buf(content.substr(1));
    std::istream_iterator<std::string> beg(buf), eos;
    if ((*beg).compare("custom") == 0) {
      content = "/echo " + JSON(content).dump();
    } else {
      auto it = CustomCommandMap.find(*beg);
      if (it != CustomCommandMap.end()) {
        replaceAll(content, "@", "◎");
        content = "/custom " + content;
      } else {
        auto it2 = CustomCommandWithPlayerSelectorMap.find(*beg);
        if (it2 != CustomCommandWithPlayerSelectorMap.end()) {
          auto cmd = *beg;
          beg++;
          if (beg != eos) {
            auto selector = *beg;
            ++beg;
            content = "/custom2 " + selector + " /" + cmd + " " + join(beg, eos, " ");
          }
        }
      }
    }
  }
  original(this, content, orig, unk);
}

template <typename T> struct CommandSelectorResults {
  std::shared_ptr<std::vector<T *>> content;
  bool empty() const;
};

struct CommandSelectorBase {
  CommandSelectorBase(bool);
  virtual ~CommandSelectorBase();
};

template <typename T> struct CommandSelector : CommandSelectorBase {
  char filler[0x74];
  CommandSelector();

  const CommandSelectorResults<T> results(CommandOrigin const &) const;
};

struct CommandSelectorPlayer : CommandSelector<Player> {
  CommandSelectorPlayer()
      : CommandSelector() {}
  ~CommandSelectorPlayer() {}

  static typeid_t<CommandRegistry> type_id() {
    static typeid_t<CommandRegistry> ret =
        type_id_minecraft_symbol<CommandRegistry>("_ZZ7type_idI15CommandRegistry15CommandSelectorI6PlayerEE8typeid_tIT_EvE2id");
    return ret;
  }
};

struct StubCommand : Command {
  CommandSelectorPlayer selector;
  CommandMessage msg;
  ~StubCommand() {}
  void execute(CommandOrigin const &orig, CommandOutput &outp) {
    if (VALID(msg)) {
      auto content = msg.getMessage(orig);
      std::istringstream buf(content.substr(1));
      std::istream_iterator<std::string> beg(buf), end(buf);
      auto it = CustomCommandMap.find(*beg);
      if (it != CustomCommandMap.end()) {
        if (it->second->perm < orig.getPermissionsLevel()) {
          std::stringstream ss;
          ss << "Permission Denied, Requested Command " << *beg << "(" << it->second->perm << ") But your permissions level is " << orig.getPermissionsLevel() << std::endl;
          outp.error(ss.str());
          return;
        }
        try {
          auto boxed = it->second->execute({ orig }, content.substr(std::min(content.length(), 2 + beg->length())));
          if (boxed.is_type(user_type<std::string>())) {
            std::string result = boxed_cast<std::string>(boxed);
            if (!result.empty()) outp.addMessage(result);
          }
          outp.success();
          return;
        } catch (const std::exception &e) {
          Log::error("CMD", "%s", e.what());
          outp.error(e.what());
          return;
        }
      }
      auto it2 = CustomCommandWithPlayerSelectorMap.find(*beg);
      if (it2 != CustomCommandWithPlayerSelectorMap.end()) {
        if (it->second->perm < orig.getPermissionsLevel()) {
          outp.error("Permission Denied");
          return;
        }
        try {
          auto ret = selector.results(orig);
          if (ret.empty()) {
            auto boxed = it2->second->execute({ orig }, {}, content.substr(std::min(content.length(), 2 + beg->length())));
            if (boxed.is_type(user_type<std::string>())) {
              std::string result = boxed_cast<std::string>(boxed);
              if (!result.empty()) outp.addMessage(result);
            }
          } else {
            for (auto item : *ret.content) { Log::trace("CMD", "item: %s", item->getNameTag().c_str()); }
            auto boxed = it2->second->execute({ orig }, reinterpret_cast<std::vector<ServerPlayer *> &>(*ret.content),
                                              content.substr(std::min(content.length(), 2 + beg->length())));
            if (boxed.is_type(user_type<std::string>())) {
              std::string result = boxed_cast<std::string>(boxed);
              if (!result.empty()) outp.addMessage(result);
            }
          }
          outp.success();
          return;
        } catch (const std::exception &e) {
          Log::error("CMD", "%s", e.what());
          outp.error(e.what());
          return;
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

TClasslessInstanceHook(void, _ZN10SayCommand5setupER15CommandRegistry, CommandRegistry &reg) {
  Log::info("CMD", "Setup Command");
  reg.registerCommand("custom", "STUB Command.", (CommandPermissionLevel)0, (CommandFlag)0, (CommandFlag)0);
  reg.registerOverload<StubCommand>("custom", CommandVersion(0, INT32_MAX),
                                    CommandParameterData(CommandMessage::type_id(), &CommandRegistry::parse<CommandMessage>, "stub",
                                                         (CommandParameterDataType)0, nullptr, offsetof(StubCommand, msg), true, -1));
  reg.registerCommand("custom2", "STUB Command.", (CommandPermissionLevel)0, (CommandFlag)0, (CommandFlag)0);
  reg.registerOverload<StubCommand>("custom2", CommandVersion(0, INT32_MAX),
                                    CommandParameterData(CommandSelectorPlayer::type_id(), &CommandRegistry::parse<CommandSelector<Player>>, "target",
                                                         (CommandParameterDataType)0, nullptr, offsetof(StubCommand, selector), true, -1),
                                    CommandParameterData(CommandMessage::type_id(), &CommandRegistry::parse<CommandMessage>, "stub",
                                                         (CommandParameterDataType)0, nullptr, offsetof(StubCommand, msg), true, -1));
  reg.registerCommand("echo", "ECHO Command.", (CommandPermissionLevel)0, (CommandFlag)0, (CommandFlag)0);
  reg.registerOverload<EchoCommand>("echo", CommandVersion(0, INT32_MAX),
                                    CommandParameterData(CommandMessage::type_id(), &CommandRegistry::parse<CommandMessage>, "content",
                                                         (CommandParameterDataType)0, nullptr, offsetof(EchoCommand, msg), true, -1));
  original(this, reg);
}

template <typename T> void regParam(const ModulePtr &m, std::string name) {
  m->add(user_type<T>(), name);
  m->add(base_class<ParamDef, T>());
  m->add(constructor<T(std::string, bool)>(), name);
}

std::shared_ptr<CustomCommand> defcmd(std::string name, std::string desc, unsigned char flag, unsigned char perm,
                                      std::function<Boxed_Value(CommandSender orig, std::string)> execute) {
  CustomCommandMap.emplace(std::make_pair(name, std::make_shared<CustomCommand>(desc, flag, perm, execute)));
  return CustomCommandMap.find(name)->second;
}

std::shared_ptr<CustomCommandWithPlayerSelector>
defcmd2(std::string name, std::string desc, unsigned char flag, unsigned char perm,
        std::function<Boxed_Value(CommandSender orig, std::vector<ServerPlayer *>, std::string)> execute) {
  CustomCommandWithPlayerSelectorMap.emplace(std::make_pair(name, std::make_shared<CustomCommandWithPlayerSelector>(desc, flag, perm, execute)));
  return CustomCommandWithPlayerSelectorMap.find(name)->second;
}

extern "C" void mod_init() {
  ModulePtr m(new Module());
  utility::add_class<CommandSender>(*m, "CommandOrigin", {},
                                    {
                                        { fun(&CommandSender::isPlayer), "isPlayer" },
                                        { fun(&CommandSender::getPlayer), "getPlayer" },
                                        { fun(&CommandSender::getPermissionsLevel), "getPermissionsLevel" },
                                    });
  m->add(user_type<CommandSender>(), "CommandOrigin");
  m->add(fun(&CommandSender::isPlayer), "isPlayer");
  m->add(fun(&CommandSender::getPlayer), "getPlayer");
  m->add(user_type<EnumDef>(), "EnumDef");
  m->add(constructor<EnumDef(std::string)>(), "EnumDef");
  m->add(fun(&EnumDef::add), "add");
  m->add(user_type<ParamDef>(), "Param");
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
  m->add(user_type<EnumParamDef>(), "Param_enum");
  m->add(constructor<EnumParamDef(std::string, bool, EnumDef)>(), "Param_enum");
  m->add(base_class<ParamDef, EnumParamDef>());
  m->add(user_type<OverloadDef>(), "CommandOverload");
  m->add(constructor<OverloadDef()>(), "CommandOverload");
  m->add(fun(&OverloadDef::add), "add");
  m->add(user_type<CustomCommand>(), "CustomCommand");
  m->add(user_type<CustomCommandWithPlayerSelector>(), "CustomCommand2");
  m->add(fun(&defcmd), "defcmd");
  m->add(fun(&defcmd2), "defcmd2");
  m->add(fun(&CustomCommand::add), "add");
  m->add(fun(&CustomCommandWithPlayerSelector::add), "add");
  loadModule(m);
}