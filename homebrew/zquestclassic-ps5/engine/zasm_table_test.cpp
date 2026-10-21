// SPDX-License-Identifier: GPL-3.0-or-later
// This test links the ORIGINAL upstream components/zasm/table.cpp.
// It does not substitute the command table or claim that the VM executes.
#include "components/zasm/table.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

int main() {
    auto waitframe = get_script_command(WAITFRAME);
    auto goto_cmd = get_script_command(GOTO);
    auto setv = get_script_command(std::string("SETV"));
    assert(waitframe && strcmp(waitframe->name, "WAITFRAME") == 0);
    assert(waitframe->args == 0);
    assert(goto_cmd && goto_cmd->args == 1);
    assert(setv && setv->command == SETV && setv->args == 2);
    assert(get_script_command(std::string("missing-opcode")) == nullptr);
    const auto& dependencies = get_register_dependencies(0);
    assert(dependencies.size() < 200);
    puts("PASS: original ZASM instruction table provides real WAITFRAME/GOTO/SETV opcodes");
}
