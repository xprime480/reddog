#include "vm.hpp"

#include <iostream>

#include "VM_executor.hpp"

using namespace std;

VM::VM()
    : program_size(0u), valid_program(true)
{
}

void VM::load(unsigned int reg, unsigned int addr)
{
    maybe_add_op_RA(LOAD, reg, addr);
}

void VM::store(unsigned int reg, unsigned int addr)
{
    maybe_add_op_RA(STORE, reg, addr);
}

VM_exec_status VM::exec(bool verbose)
{
    if (!valid_program)
    {
        return VM_exec_status("Cannot execute invalid program");
    }

    if (verbose)
    {
        cerr << "Starting program execution\n";
//        labels.dump();
        cerr << "program_size = " << program_size << "\n";
    }

    VM_executor executor(program, program_size, heap);
    return executor.exec(verbose);
}

void VM::set_heap(unsigned int addr, int value)
{
    if (addr < MAX_HEAP_SIZE)
    {
        heap[addr] = value;
    }
}

int VM::get_heap(unsigned int addr)
{
    if (addr < MAX_HEAP_SIZE)
    {
        return heap[addr];
    }

    return 0xdeadbeef;
}

bool VM::check_program_size()
{
    if (valid_program)
    {
        if (program_size >= MAX_PROGRAM_SIZE)
        {
            valid_program = false;
        }
    }
    return valid_program;
}

bool VM::check_register(unsigned int reg)
{
    if (valid_program)
    {
        if (reg >= MAX_REGISTERS)
        {
            valid_program = false;
        }
    }
    return valid_program;
}

bool VM::check_address(unsigned int addr)
{
    if (valid_program)
    {
        if (addr >= MAX_HEAP_SIZE)
        {
            valid_program = false;
        }
    }
    return valid_program;
}

void VM::maybe_add_op_RA(OPCODE op, unsigned int reg, unsigned int addr)
{
    if (valid_program)
    {
        if (check_program_size() && check_register(reg) && check_address(addr))
        {
            unsigned int instr = (((unsigned int)op) << 24) | (reg << 16) | addr;
            program[program_size++] = instr;
        }
    }
}
