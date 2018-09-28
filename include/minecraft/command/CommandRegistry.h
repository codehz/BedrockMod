#pragma once

#include "CommandVersion.h"
#include <memory>
#include <vector>

class Command;
class CommandParameterData;
class CommandOrigin;
enum CommandPermissionLevel : int {};
enum CommandFlag : int {};

class CommandRegistry {

public:
  class ParseToken;
  struct Overload {
    CommandVersion version;
    std::unique_ptr<Command> (*allocator)();
    std::vector<CommandParameterData> params;
    int filler; // I don't think this is an actual member, looks like padding to me

    Overload(CommandVersion version, std::unique_ptr<Command> (*allocator)())
        : version(version)
        , allocator(allocator) {}
  };
  struct Signature {
    char filler[64];
    std::vector<CommandRegistry::Overload> overloads;
  };

  Signature *findCommand(std::string const &);

  void buildOverload(Overload &);

  void registerOverloadInternal(Signature &, Overload &);

  void registerCommand(std::string const &, char const *, CommandPermissionLevel, CommandFlag, CommandFlag);

  template <typename T> bool parse(void *, ParseToken const &, CommandOrigin const &, int, std::string &, std::vector<std::string> &) const;

  template <typename T> static std::unique_ptr<Command> allocateCommand() { return std::unique_ptr<Command>(new T()); }

  template <typename T, typename... Args> void registerOverload(const char *name, CommandVersion version, Args &&... args) {
    Signature *signature = findCommand(name);
    signature->overloads.emplace_back(version, allocateCommand<T>);
    Overload &overload = *signature->overloads.rbegin();
    buildOverload(overload);
    overload.params = { args... };
    registerOverloadInternal(*signature, overload);
  }

  template <typename F> void registerCustomOverload(const char *name, CommandVersion version, std::unique_ptr<Command> (*allocator)(), F f) {
    Signature *signature = findCommand(name);
    signature->overloads.emplace_back(version, allocator);
    Overload &overload = *signature->overloads.rbegin();
    buildOverload(overload);
    f(overload);
    registerOverloadInternal(*signature, overload);
  }

  void addSoftEnum(std::string const &, std::vector<std::string>);
};