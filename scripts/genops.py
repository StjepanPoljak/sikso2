#!/usr/bin/env python3

import re
from collections import OrderedDict

class OpcodeList():

    def __init__(self, filename):

        self.instr_list = OrderedDict()

        with open(filename, mode='r', errors='ignore') as file:
            for line in file.readlines():
                sregex_match = Subinstr.regex.match(line)
                if sregex_match:
                    name = sregex_match.group(2)
                    if name not in self.instr_list:
                        self.instr_list[name] = Instr(sregex_match.group(2), [])

                    cycles = sregex_match.group(5)
                    mode = 'MODE_{}'.format(sregex_match.group(1))
                    if cycles[-1] == '+':
                        cycles = cycles[:-1]
                        mode = mode + ' | MODE_EXTRA_CYCLE'
                    self.instr_list[name].list.append(Subinstr(
                        int(sregex_match.group(3), 16),
                        cycles,
                        sregex_match.group(4),
                        mode
                    ))

    def print_c(self):
        res = '''#include <stdlib.h>

#include "cpu.h"
#include "instr.h"

'''
        count = 0
        for name, instr in self.instr_list.items():
            res = res + 'subinstr_t ' + instr.get_list_name()
            res = res + '[] = {\n'
            count = 0
            for subinstr in instr.list:
                res = res + str(subinstr) \
                    + (',\n' if count <= len(instr.list) - 2 else '')
                count = count + 1
            res = res + '\n};\n\n'
        print(res[:-1]) # cheap way to account for the extra newline

        res = 'instr_t instr_list[] = {\n'
        count = 0
        for name, instr in self.instr_list.items():
            res = res + str(instr)
            res = res + (',\n' if count <= len(self.instr_list) - 2 else '')
            count = count + 1
        res = res + '\n};'
        res = res + '''

instr_t* get_instr_list(void) {
	return instr_list;
}

size_t get_instr_list_size(void) {
	return sizeof(instr_list) / sizeof(*instr_list);
}'''
        print(res)

class Subinstr():

    regex = re.compile('^([A-Z_]+)\s+([A-Z]..)\s+\$([0-9A-F].)\s+([0-9])\s+([0-9]\+?)$')

    def __init__(self, opcode, cycles, length, mode, extra=False, supported='CPU_6502_CORE'):
        self.opcode = opcode
        self.cycles = cycles
        self.extra = extra
        self.length = length
        self.mode = mode
        self.supported = supported

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        res = '\t(subinstr_t) {'
        res = res + '\n\t\t.opcode = {},'.format(hex(self.opcode))
        res = res + '\n\t\t.cycles = {},'.format(self.cycles)
        res = res + '\n\t\t.length = {},'.format(self.length)
        res = res + '\n\t\t.mode = {}{},'.format(self.mode, 'MODE_EXTRA_CYCLE' if self.extra else '')
        res = res + '\n\t\t.supported = {}'.format(self.supported)
        res = res + '\n\t}'

        return res

class Instr():

    def __init__(self, name, list):
        self.name = name
        self.list = list

        return

    def get_list_name(self):
        return '{}_{}'.format(self.name.lower(), 'list' if len(self.list) > 1 else 'single')

    def get_action_name(self):
        return '{}_action'.format(self.name.lower())

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        res = '\t(instr_t) {'
        res = res + '\n\t\t.name = "{}",'.format(self.name)
        res = res + '\n\t\t.list = {},'.format(self.get_list_name())
        res = res + '\n\t\t.size = {},'.format(len(self.list))
        res = res + '\n\t\t.action = NULL'
        res = res + '\n\t}'

        return res

if __name__ == "__main__":

    OpcodeList("scripts/6502ops.txt").print_c()

