#include <string>
#include <memory>
#include <functional>
#include <sstream>

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

struct MinecraftCommands {
  MCRESULT requestCommandExecution(std::unique_ptr<CommandOrigin> o, std::string const &s, int i, bool b) const;
};

struct Minecraft {
  MinecraftCommands *getCommands();
};

struct ServerInstance {
  void queueForServerThread(std::function<void()> p1);
  Minecraft *getMinecraft();
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

extern "C" void _ZNK20CommandOutputMessage14getUserMessageB5cxx11Ev(std::string &out, CommandOutputMessage const *);

ServerInstance *si __attribute__ ((visibility ("hidden")));

static bool state = false;
static std::stringstream ss;

__attribute__ ((visibility ("hidden"))) std::string execCommand(std::string line) {
  if (line.size() == 0) return "";
  state = true;
  ss.str("");
  std::unique_ptr<DedicatedServerCommandOrigin> commandOrigin(new DedicatedServerCommandOrigin("Server", *si->getMinecraft()));
  si->getMinecraft()->getCommands()->requestCommandExecution(std::move(commandOrigin), line, 4, true);
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
      ss << data;
    }
  } else {
    original(this, orig, outp);
  }
}