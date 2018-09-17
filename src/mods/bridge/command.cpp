#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <StaticHook.h>

enum class MCCATEGORY {
  //
};

struct MCRESULT {
  bool success;
  MCCATEGORY category;
  unsigned short code;
};
struct CommandOrigin {
  virtual ~CommandOrigin();
};

struct AutoCompleteOption {
  std::string s1, s2, s3;
  char filler[120 - 3 * sizeof(std::string)];
};

struct AutoCompleteInformation {
  void *filler;
  std::vector<AutoCompleteOption> list;
};

struct CommandRegistry {
  std::unique_ptr<AutoCompleteInformation> getAutoCompleteOptions(CommandOrigin const &, std::string const &, unsigned int) const;
};

struct MinecraftCommands {
  MCRESULT requestCommandExecution(std::unique_ptr<CommandOrigin> o, std::string const &s, int i, bool b) const;
  CommandRegistry &getRegistry();
};

struct Minecraft {
  MinecraftCommands *getCommands();
};

struct ServerInstance {
  void queueForServerThread(std::function<void()> p1);
};

struct DedicatedServerCommandOrigin : public CommandOrigin {
  char filler[0x40];
  DedicatedServerCommandOrigin(std::string const &s, Minecraft &m);
};

struct CommandOutputMessage {
  char filler[64];
};

struct CommandOutput {
  char filler[16];
  std::vector<CommandOutputMessage> data;
};

struct ServerCommand {
  static Minecraft *mGame;
};

extern "C" void _ZNK20CommandOutputMessage14getUserMessageB5cxx11Ev(std::string &out, CommandOutputMessage const *);

ServerInstance *si __attribute__((visibility("hidden")));

static bool state = false;
static std::stringstream ss;

__attribute__((visibility("hidden"))) std::string execCommand(std::string line) {
  if (line.size() == 0) return "";
  state = true;
  ss.str("");
  std::unique_ptr<DedicatedServerCommandOrigin> commandOrigin(new DedicatedServerCommandOrigin("Server", *ServerCommand::mGame));
  ServerCommand::mGame->getCommands()->requestCommandExecution(std::move(commandOrigin), line, 4, true);
  state = false;
  return ss.str();
}

extern "C" void mcpelauncher_server_thread(std::function<void()> fun) { si->queueForServerThread(fun); }

TClasslessInstanceHook(void, _ZN19CommandOutputSender4sendERK13CommandOriginRK13CommandOutput, const CommandOrigin *orig, const CommandOutput *outp) {
  auto modded = dynamic_cast<DedicatedServerCommandOrigin const *>(orig);
  if (modded && state) {
    for (auto &item : outp->data) {
      std::string data;
      _ZNK20CommandOutputMessage14getUserMessageB5cxx11Ev(data, &item);
      ss << data << std::endl;
    }
  } else {
    original(this, orig, outp);
  }
}

std::vector<std::string> doComplete(std::string input, unsigned pos) {
  std::unique_ptr<DedicatedServerCommandOrigin> commandOrigin(new DedicatedServerCommandOrigin("Server", *ServerCommand::mGame));
  auto info = ServerCommand::mGame->getCommands()->getRegistry().getAutoCompleteOptions(*commandOrigin, input, pos);
  std::vector<std::string> ret;
  for (auto &item : info->list) ret.push_back(item.s1);
  return ret;
}