
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
    runner([op, name, fn]()->bool {
        VM vm;
        vm.load(0, 0);
        vm.load(1, 1);
        (vm.*op)(0, 1, 2);
        vm.store(2, 2);
        char buf[128];
        sprintf(buf, "%s Correctly", name);
        vm.set_heap(0, 12);
        vm.set_heap(1, 4);
        return EXPECT_RUN_OK(vm, buf, [&vm, fn](bool verbose) -> bool {
            if ( verbose )
            {
                vm.dump_heap(0, 2);
            }
            int act = vm.get_heap(2);
            int exp = fn(12, 4);
            if ( verbose )
            {
                cerr << "Expected: " << exp << "; actual: " << act << "\n";
            }
            return act == exp;
        });
    });
}

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

int main(void)
{
    Runner runner;

    runner(empty_program);

    load_suite(runner);
    math_suite(runner);

    return runner.report();
}
