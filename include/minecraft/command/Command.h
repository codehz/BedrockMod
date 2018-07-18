#pragma once

#include "../util/typeid.h"
#include "CommandRegistry.h"

class CommandOutput;

class Command {

private:
    char filler[0x10];

public:
    Command();
    virtual ~Command();
    virtual void execute(CommandOrigin const&, CommandOutput&) = 0;

};
