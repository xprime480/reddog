
#include <functional>
#include <iostream>
#include <string>

#include "vm.hpp"
#include "Runner.hpp"

using namespace std;

bool empty_program()
{
    VM vm;
    return EXPECT_RUN_OK(vm, "Empty program", [](bool) -> bool { return true; });
}

bool load_bad_register()
{
    VM vm;
    vm.load(32, 0);
    return EXPECT_ERROR(vm, "Load Bad Register");
}

bool load_bad_address()
{
    VM vm;
    vm.load(31, 8192);
    return EXPECT_ERROR(vm, "Load Bad Address");
}

bool store_bad_register()
{
    VM vm;
    vm.store(32, 0);
    return EXPECT_ERROR(vm, "Store Bad Register");
}

bool store_bad_address()
{
    VM vm;
    vm.store(31, 8192);
    return EXPECT_ERROR(vm, "Store Bad Address");
}

bool load_store_ok()
{
    VM vm;
    vm.load(31, 0);
    vm.store(31, 1);

    vm.set_heap(0, 42);
    return EXPECT_RUN_OK(vm, "Load Store OK", [&vm](bool verbose) -> bool { 
        int act = vm.get_heap(1);
        if ( verbose )
        {
            if (42 != act)
            {
                cerr << "Expected 42, got " << act << "\n";
            }
        }
        return 42 == act; 
    });
}

void load_suite(Runner &runner)
{
    runner(load_bad_register);
    runner(load_bad_address);
    runner(store_bad_register);
    runner(store_bad_address);
    runner(load_store_ok);
}

template<typename F>
void binop_test(Runner & runner,
                int lhs,
                int rhs,
                void (VM::*op)(unsigned int, unsigned int, unsigned int), 
                F fn, 
                const char * name)
{
    runner([&]()->bool {
        VM vm;
        vm.load(0, 0);
        vm.load(1, 1);
        (vm.*op)(0, 1, 2);
        vm.store(2, 2);
        char buf[128];
        sprintf(buf, "%s Correctly", name);
        vm.set_heap(0, lhs);
        vm.set_heap(1, rhs);
        return EXPECT_RUN_OK(vm, buf, [&vm, lhs, rhs, op, fn, name](bool verbose) -> bool {
            if ( verbose )
            {
                vm.dump_heap(0, 2);
            }
            int act = vm.get_heap(2);
            int exp = fn(lhs, rhs);
            if ( verbose )
            {
                cerr << "Expected: " << exp << "; actual: " << act << "\n";
            }
            return act == exp;
        });
    });
}

template <typename F>
void math_tests(Runner & runner, void (VM::*op)(unsigned int, unsigned int, unsigned int), F fn, const char * name)
{
    runner([op, name]()->bool {
        VM vm;
        (vm.*op)(32, 1, 2);
        char buf[128];
        sprintf(buf, "%s Bad Register 1", name);
        EXPECT_ERROR(vm, buf);
    });
    runner([op, name]()->bool {
        VM vm;
        (vm.*op)(0, 32, 2);
        char buf[128];
        sprintf(buf, "%s Bad Register 2", name);
        EXPECT_ERROR(vm, buf);
    });
    runner([op, name]()->bool {
        VM vm;
        (vm.*op)(0, 1, 32);
        char buf[128];
        sprintf(buf, "%s Bad Register 3", name);
        EXPECT_ERROR(vm, buf);
    });

    binop_test(runner, 12, 4, op, fn, name);}

bool divide_by_zero()
{
    VM vm;
    vm.load(0, 0);
    vm.load(1, 1);
    vm.div(0, 1, 2);
    vm.store(2, 2);
    vm.set_heap(0, 1);
    vm.set_heap(1, 0);
    EXPECT_ERROR(vm, "Divide By Zero");
}

void math_suite(Runner & runner)
{
    math_tests(runner, &VM::add, plus<int>(), "Add");
    math_tests(runner, &VM::sub, minus<int>(), "Sub");
    math_tests(runner, &VM::mul, multiplies<int>(), "Mul");
    math_tests(runner, &VM::div, divides<int>(), "Div");
    runner(divide_by_zero);
}

template <typename T>
struct compares
{
    T operator()(const T & l, const T & r) const
    {
        return (l<r) ? -1 : ((l==r) ? 0 : 1);
    }
};

void cmp_suite(Runner & runner)
{
    auto op = &VM::cmp;
    auto fn = compares<int>();

    math_tests(runner, op, fn, "Cmp");
    binop_test(runner, -20, 20, op, fn, "CMP LT");
    binop_test(runner, 42, 42, op, fn, "CMP EQ");
    binop_test(runner, 42, 17, op, fn, "CMP GT");
}

bool program_too_long()
{
    VM vm;
    for (int i = 0; i < 2048; ++i)
    {
        vm.add(0, 0, 0);
    }

    return EXPECT_ERROR(vm, "Program Too Long");
}

bool jmp_always(int test_value)
{
    VM vm;
    vm.load(0, 0);   // 0
    vm.jmp(4);       // 1 jump if condition
    vm.load(1, 1);   // 2
    vm.jmp(5);       // 3 
    vm.load(1, 2);   // 4
    vm.store(1, 3);  // 5

    vm.set_heap(0, test_value);
    vm.set_heap(1, 0);
    vm.set_heap(2, 1);

    int exp = 1;

    return EXPECT_RUN_OK(vm, "JMP", [&vm, exp](bool verbose) -> bool {
        if ( verbose )
        {
            vm.dump_heap(0, 4);
        }
        int act = vm.get_heap(3);
        if ( verbose )
        {
            cerr << "Expected: " << exp << "; actual: " << act << "\n";
        }
        return act == exp;
    });
}

void jmp_suite(Runner &runner)
{
    const string base("JMP");

    runner([&]() -> bool {
        VM vm;
        vm.jmp(9999);
        return EXPECT_ERROR(vm, base + " Bad Address");
    });
    runner([&]() -> bool {
        VM vm;
        vm.jmp(22); // only detectable at run time
        return EXPECT_ERROR(vm, base + " Bad Address @ Runtime");
    });
    runner([&]() -> bool {
        return jmp_always(-1);
    });
    runner([&]() -> bool {
        return jmp_always(0);
    });
    runner([&]() -> bool {
        return jmp_always(+1);
    });
}

bool jxx_bad_register(void (VM::*op)(unsigned int reg, unsigned int loc), string const &label)
{
    VM vm;
    (vm.*op)(99, 0);
    return EXPECT_ERROR(vm, label);
}

bool jxx_bad_location(void (VM::*op)(unsigned int reg, unsigned int loc), string const &label)
{
    VM vm;
    (vm.*op)(0, 9999);
    return EXPECT_ERROR(vm, label);
}

bool jxx_when_test(void (VM::*op)(unsigned int, unsigned int), string const &label, int exp, int test_value)
{
    VM vm;
    vm.load(0, 0);   // 0
    (vm.*op)(0, 4);  // 1 jump if condition
    vm.load(1, 1);   // 2
    vm.jmp(5);       // 3 
    vm.load(1, 2);   // 4
    vm.store(1, 3);  // 5

    vm.set_heap(0, test_value);
    vm.set_heap(1, 0);
    vm.set_heap(2, 1);

    return EXPECT_RUN_OK(vm, label, [&vm, exp](bool verbose) -> bool {
        if ( verbose )
        {
            vm.dump_heap(0, 4);
        }
        int act = vm.get_heap(3);
        if ( verbose )
        {
            cerr << "Expected: " << exp << "; actual: " << act << "\n";
        }
        return act == exp;
    });
}

bool jxx_when_lt(void (VM::*op)(unsigned int, unsigned int), string const &label, int exp)
{
    return jxx_when_test(op, label, exp, -1);
}

bool jxx_when_eq(void (VM::*op)(unsigned int, unsigned int), string const &label, int exp)
{
    return jxx_when_test(op, label, exp, 0);
}

bool jxx_when_gt(void (VM::*op)(unsigned int, unsigned int), string const &label, int exp)
{
    return jxx_when_test(op, label, exp, 1);
}

void conditional_jmp_suite(Runner &runner, void (VM::*op)(unsigned int, unsigned int), const char *name, bool lt, bool eq, bool gt)
{
    const string base(name);

    runner([&]() -> bool {
        return jxx_bad_register(op, base + " Bad Register");
    });
    runner([&]() -> bool {
        return jxx_bad_location(op, base + " Bad Location");
    });
    runner([&]() -> bool {
        return jxx_when_lt(op, base + " when LT", lt ? 1 : 0);
    });
    runner([&]() -> bool {
        return jxx_when_eq(op, base + " when EQ", eq ? 1 : 0);
    });
    runner([&]() -> bool {
        return jxx_when_gt(op, base + " when GT", gt ? 1 : 0);
    });
}

int main(void)
{
    Runner runner;

    runner(empty_program);

    load_suite(runner);
    math_suite(runner);
    cmp_suite(runner);

    runner(program_too_long);

    jmp_suite(runner);

    conditional_jmp_suite(runner, &VM::jeq, "JEQ", false, true, false);
    conditional_jmp_suite(runner, &VM::jne, "JNE", true, false, true);
    conditional_jmp_suite(runner, &VM::jlt, "JLT", true, false, false);
    conditional_jmp_suite(runner, &VM::jle, "JLE", true, true, false);
    conditional_jmp_suite(runner, &VM::jgt, "JGT", false, false, true);
    conditional_jmp_suite(runner, &VM::jge, "JGE", false, true, true);

    return runner.report();
}
