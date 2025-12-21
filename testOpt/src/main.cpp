
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include "interpreter.hpp"
#include "opcode.hpp"

void test_simple_add_print()
{
    Interpreter vm;

    // --- Function ---
    Function *func = vm.addFunction("test");

    func->chunk.addConstant(Value::makeInt(1)); // idx 0
    func->chunk.addConstant(Value::makeInt(2)); // idx 1

    func->chunk.write(OP_CONSTANT, 1);
    func->chunk.write(0, 1);

    func->chunk.write(OP_CONSTANT, 1);
    func->chunk.write(1, 1);

    func->chunk.write(OP_ADD, 1);
    func->chunk.write(OP_PRINT, 1);
    func->chunk.write(OP_RETURN, 1);

    // --- Fiber ---
    Process *proc = vm.addProcess("main", func);
    Fiber *fiber = &proc->fibers[0];

    // --- Run ---
    vm.run_fiber(fiber, 100);

    assert(fiber->state == FiberState::DEAD);
    printf("✓ test_simple_add_print passed\n");
}

void test_return_value()
{
    Interpreter vm;
    Function *func = vm.addFunction("ret");

    func->chunk.addConstant(Value::makeInt(40));
    func->chunk.addConstant(Value::makeInt(2));

    func->chunk.write(OP_CONSTANT, 1);
    func->chunk.write(0, 1);
    func->chunk.write(OP_CONSTANT, 1);
    func->chunk.write(1, 1);
    func->chunk.write(OP_ADD, 1);
    func->chunk.write(OP_RETURN, 1);

    Process *proc = vm.addProcess("p", func);
    Fiber *fiber = &proc->fibers[0];

    vm.run_fiber(fiber, 100);

    assert(fiber->stackTop > fiber->stack);
    Value result = fiber->stackTop[-1];
    assert(result.isInt() && result.asInt() == 42);

    printf("✓ test_return_value passed\n");
}

void test_function_call()
{
    Interpreter vm;

    // --- callee ---
    Function *add = vm.addFunction("add", 2);
    add->chunk.write(OP_GET_LOCAL, 1);
    add->chunk.write(0, 1);
    add->chunk.write(OP_GET_LOCAL, 1);
    add->chunk.write(1, 1);
    add->chunk.write(OP_ADD, 1);
    add->chunk.write(OP_RETURN, 1);

    // --- caller ---
    Function *main = vm.addFunction("main", 0);
    main->chunk.addConstant(Value::makeInt(20));
    main->chunk.addConstant(Value::makeInt(22));
    main->chunk.addConstant(Value::makeFunction(0));

    main->chunk.write(OP_CONSTANT, 1); main->chunk.write(2, 1);  // push função PRIMEIRO
    main->chunk.write(OP_CONSTANT, 1); main->chunk.write(0, 1);  // push 20
    main->chunk.write(OP_CONSTANT, 1); main->chunk.write(1, 1);  // push 22
    main->chunk.write(OP_CALL, 1);     main->chunk.write(2, 1);  // call(2 args)
    main->chunk.write(OP_RETURN, 1);

    Process *proc = vm.addProcess("process", main);
    Fiber *fiber = &proc->fibers[0];

    vm.run_fiber(fiber, 200);

    Value result = fiber->stackTop[-1];
    assert(result.isInt() && result.asInt() == 42);

    printf("✓ test_function_call passed\n");
}

Value native_print(Interpreter* vm, int argCount, Value* args)
{
    for (int i = 0; i < argCount; i++)
    {
        vm->print(args[i]);
        if (i < argCount - 1) printf(" ");
    }
    printf("\n");
    return Value::makeNil();
}

Value native_sqrt(Interpreter* vm, int argCount, Value* args)
{
    if (!args[0].isDouble() && !args[0].isInt())
    {
        vm->runtimeError("sqrt expects a number");
        return Value::makeNil();
    }
    
    double value = args[0].isInt() ? 
        static_cast<double>(args[0].asInt()) : args[0].asDouble();
    
    return Value::makeDouble(std::sqrt(value));
}

Value native_clock(Interpreter* vm, int argCount, Value* args)
{
    return Value::makeDouble(static_cast<double>(clock()) / CLOCKS_PER_SEC);
}

void test_native_call()
{
    Interpreter vm;

    vm.registerNative("print", native_print, -1);
    vm.registerNative("sqrt", native_sqrt, 1);
    vm.registerNative("clock", native_clock, 0);
    
    // main: print(sqrt(16))
    Function* main = vm.addFunction("main", 0);
    
    
    // Get sqrt native
    Value native = Value::makeNative(1);
    main->chunk.addConstant(native);
    main->chunk.write(OP_CONSTANT, 1);    main->chunk.write(0, 1);
    
    // Push 16
    main->chunk.addConstant(Value::makeInt(16));
    main->chunk.write(OP_CONSTANT, 1);     main->chunk.write(1, 1);
 
    // Call sqrt(1 arg)
    main->chunk.write(OP_CALL, 1);    main->chunk.write(1, 1);
    
    // Print result
    main->chunk.write(OP_PRINT, 1);
    main->chunk.write(OP_RETURN, 1);
    
    Process* proc = vm.addProcess("test", main);
    vm.run_fiber(&proc->fibers[0], 200);
    
    printf("✓ test_native_call passed\n");
}

void test_stack_api()
{
    Interpreter vm;
 
    
    Function* main = vm.addFunction("main", 0);
    Process* proc = vm.addProcess("test", main);
   
    
    // === BASIC PUSH/POP ===
    assert(vm.getTop() == 0);
    
    vm.pushInt(42);
    vm.pushDouble(3.14);
    vm.pushString("hello");
    vm.pushBool(true);
    vm.pushNil();
    
    assert(vm.getTop() == 5);
    
    // === TYPE CHECKING (positivo) ===
    assert(vm.isInt(0));
    assert(vm.isDouble(1));
    assert(vm.isString(2));
    assert(vm.isBool(3));
    assert(vm.isNil(4));
    
    // === TYPE CHECKING (negativo - topo) ===
    assert(vm.isNil(-1));
    assert(vm.isBool(-2));
    assert(vm.isString(-3));
    assert(vm.isDouble(-4));
    assert(vm.isInt(-5));
    
    // === CONVERSIONS ===
    assert(vm.toInt(0) == 42);
    assert(vm.toDouble(1) == 3.14);
    assert(strcmp(vm.toString(2), "hello") == 0);
    assert(vm.toBool(3) == true);
    
    // toDouble aceita int
    assert(vm.toDouble(0) == 42.0);
    
    // === PEEK ===
    const Value& v = vm.peek(-1);
    assert(v.isNil());
    
    const Value& v2 = vm.peek(0);
    assert(v2.isInt() && v2.asInt() == 42);
    
    // === POP ===
    Value popped = vm.pop();
    assert(popped.isNil());
    assert(vm.getTop() == 4);
    
    // === SETTOP ===
    vm.setTop(2);
    assert(vm.getTop() == 2);
    assert(vm.isInt(0));
    assert(vm.isDouble(1));
    
    vm.setTop(0);
    assert(vm.getTop() == 0);
    
    printf("✓ test_stack_api passed\n");
}


void test_fiber_yield()
{
    Interpreter vm;
    
    // Fiber que faz: yield(100); print(42); return;
    Function* main = vm.addFunction("main", 0);
    
    // yield(100)
    main->chunk.addConstant(Value::makeInt(100));
    main->chunk.write(OP_CONSTANT, 1); 
    main->chunk.write(0, 1);
    main->chunk.write(OP_YIELD, 1);
    
    // print(42)
    main->chunk.addConstant(Value::makeInt(42));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(1, 1);
    main->chunk.write(OP_PRINT, 1);
    
    main->chunk.write(OP_RETURN, 1);
    
    Process* proc = vm.addProcess("test", main);
    vm.spawnProcess(proc);
    
    printf("=== Test Fiber Yield ===\n");
    
    // Frame 0: executa até yield
    vm.update(0.016f);  // 16ms
    Fiber* fiber = &proc->fibers[0];
    assert(fiber->state == FiberState::SUSPENDED);
    printf("Frame 0: fiber suspended (resumeTime=%.3f)\n", fiber->resumeTime);


    
    // Frame 1-5: ainda suspenso (100ms não passaram)
    for (int i = 1; i <= 5; i++)
    {
        vm.update(0.016f);
        assert(fiber->state == FiberState::SUSPENDED);
        printf("Frame %d: still suspended\n", i);
    }
    
    // Frame 6-7: ~112ms passaram, deve resumir e printar 42
    vm.update(0.016f);
    printf("Frame 6: should resume and print 42:\n");
    assert(fiber->state == FiberState::DEAD);  // Terminou
    
    printf("✓ test_fiber_yield passed\n\n");
}

void test_process_frame()
{
    Interpreter vm;
 
    
    // Loop: x++; print(x); frame(100);
    Function* main = vm.addFunction("main", 0);
    
    // x = 0 (global)
    
    String* name= vm.addGlobalEx("x", Value::makeInt(0));
    if(!name)
    {
        printf("Failed to add global 'x'\n");
        return;
    }
    main->chunk.addConstant(Value::makeString(name));
    
    // === LOOP START ===
    int loopStart = main->chunk.count;
    
    // x = x + 1
    main->chunk.write(OP_GET_GLOBAL, 1); 
    main->chunk.write(0, 1);
    main->chunk.addConstant(Value::makeInt(1));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(1, 1);
    main->chunk.write(OP_ADD, 1);
    main->chunk.write(OP_SET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_POP, 1);
    
    // print(x)
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_PRINT, 1);
    
    // frame(100)
    main->chunk.addConstant(Value::makeInt(100));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(2, 1);
    main->chunk.write(OP_FRAME, 1);
    
    // === LOOP BACK ===
    // Offset = distância do IP atual até loopStart
    // OP_LOOP: ip -= offset, então:
    int loopEnd = main->chunk.count;
    int offset = loopEnd - loopStart + 2;  // +2 pelo OP_LOOP + short
    
    main->chunk.write(OP_LOOP, 1);
    main->chunk.writeShort(offset, 1);
    
    Process* proc = vm.addProcess("counter", main);
    
    printf("=== Test Process Frame ===\n");
    printf("Loop: loopStart=%d, loopEnd=%d, offset=%d\n", 
           loopStart, loopEnd, offset);
    
    for (int frame = 0; frame < 10; frame++)
    {
        printf("Frame %d: ", frame);
        vm.update(0.016f);
        
        if (proc->state == ProcessState::SUSPENDED)
        {
            printf("(suspended until t=%.3f)", proc->resumeTime);
        }
        printf("\n");
    }
    
    printf("✓ test_process_frame passed\n\n");
}

void test_multiple_fibers()
{
    Interpreter vm;
 
    
    // Fiber 0: loop { print("A"); yield(50); }
    Function* fiberA = vm.addFunction("fiberA", 0);
    int loopA = fiberA->chunk.count;
    fiberA->chunk.addConstant(Value::makeString("A"));
    fiberA->chunk.write(OP_CONSTANT, 1);
    fiberA->chunk.write(0, 1);
    fiberA->chunk.write(OP_PRINT, 1);
    
    fiberA->chunk.addConstant(Value::makeInt(50));
    fiberA->chunk.write(OP_CONSTANT, 1);
    fiberA->chunk.write(1, 1);
    fiberA->chunk.write(OP_YIELD, 1);
    
    int offsetA = fiberA->chunk.count - loopA + 2;
    fiberA->chunk.write(OP_LOOP, 1);
    fiberA->chunk.writeShort(offsetA, 1);
    
    // Fiber 1: loop { print("B"); yield(75); }
    Function* fiberB = vm.addFunction("fiberB", 0);
    int loopB = fiberB->chunk.count;
    fiberB->chunk.addConstant(Value::makeString("B"));
    fiberB->chunk.write(OP_CONSTANT, 1);
    fiberB->chunk.write(0, 1);
    fiberB->chunk.write(OP_PRINT, 1);
    
    fiberB->chunk.addConstant(Value::makeInt(75));
    fiberB->chunk.write(OP_CONSTANT, 1);
    fiberB->chunk.write(1, 1);
    fiberB->chunk.write(OP_YIELD, 1);
    
    int offsetB = fiberB->chunk.count - loopB + 2;
    fiberB->chunk.write(OP_LOOP, 1);
    fiberB->chunk.writeShort(offsetB, 1);
    
    // Processo com 2 fibers
    Process* proc = vm.addProcess("dual", fiberA);
    vm.spawnProcess(proc);
    

    vm.addFiber(proc, fiberB);

    
    printf("=== Test Multiple Fibers ===\n");
    printf("Fiber 0 yields 50ms, Fiber 1 yields 75ms\n");
    printf("Should see interleaved A and B:\n\n");
    
    for (int frame = 0; frame < 20; frame++)
    {
        printf("Frame %2d (t=%.0fms): ", frame, vm.getCurrentTime() * 1000);
        vm.update(0.016f);
        printf("\n");
    }
    
    printf("\n✓ test_multiple_fibers passed\n\n");
}

void test_process_frame_simple()
{
    Interpreter vm;
  ;
    
    Function* main = vm.addFunction("main", 0);
    
    // 3 iterações manuais (sem loop infinito)
    for (int i = 0; i < 3; i++)
    {
        // print(i)
        main->chunk.addConstant(Value::makeInt(i + 1));
        main->chunk.write(OP_CONSTANT, 1);
        main->chunk.write(i, 1);
        main->chunk.write(OP_PRINT, 1);
        
        // frame(100)
        main->chunk.addConstant(Value::makeInt(100));
        main->chunk.write(OP_CONSTANT, 1);
        main->chunk.write(i + 10, 1);
        main->chunk.write(OP_FRAME, 1);
    }
    
    main->chunk.write(OP_RETURN, 1);
    
    Process* proc = vm.addProcess("simple", main);
    
    printf("=== Test Process Frame (3 frames) ===\n");
    
    for (int frame = 0; frame < 5; frame++)
    {
        printf("Frame %d: ", frame);
        vm.update(0.016f);
        printf("\n");
    }
    
    printf("✓ test_process_frame_simple passed\n\n");
}


 
void test_process_frame_suspend()
{
    Interpreter vm;
    
    printf("\n=== Test: Process Frame Suspend ===\n");
    
    Function* main = vm.addFunction("main", 0);
    
    String* counterName = vm.addGlobalEx("counter", Value::makeInt(0));
    if (!counterName)
    {
        printf("Failed to add global 'counter'\n");
        return;
    }
    
    main->chunk.addConstant(Value::makeString(counterName));
    
    int loopStart = main->chunk.count;
    
    // === CHECK: if (counter >= 5) return ===
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.addConstant(Value::makeInt(5));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(1, 1);
    main->chunk.write(OP_GREATER_EQUAL, 1);
    
    int exitJump = main->chunk.count;
    main->chunk.write(OP_JUMP_IF_FALSE, 1);
    main->chunk.writeShort(0, 1);
    
    main->chunk.write(OP_POP, 1);
    main->chunk.write(OP_RETURN, 1);
    
    // Patch jump
    int afterJump = main->chunk.count;
    int jumpDist = afterJump - exitJump - 3;
    main->chunk.code[exitJump + 1] = (jumpDist >> 8) & 0xff;
    main->chunk.code[exitJump + 2] = jumpDist & 0xff;
    
    main->chunk.write(OP_POP, 1);
    
    // === print(counter) ===
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_PRINT, 1);
    
    // === frame(100) ===
    main->chunk.addConstant(Value::makeInt(100));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(2, 1);
    main->chunk.write(OP_FRAME, 1);
    
    // === counter++ ===
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.addConstant(Value::makeInt(1));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(3, 1);
    main->chunk.write(OP_ADD, 1);
    main->chunk.write(OP_SET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_POP, 1);
    
    // === LOOP ===
    int loopEnd = main->chunk.count;
    int offset = loopEnd - loopStart + 3;  // +3 não +2!
    main->chunk.write(OP_LOOP, 1);
    main->chunk.writeShort(offset, 1);
    
    Process* proc = vm.addProcess("test", main);
    vm.spawnProcess(proc);
    
    printf("Process frame(100), max 5 iterations\n\n");
    
    for (int frame = 0; frame < 10; frame++)
    {
        printf("Frame %d: ", frame);
        vm.update(0.016f);
        printf(" (proc state=%s)\n", 
               proc->state == ProcessState::RUNNING ? "RUNNING" :
               proc->state == ProcessState::SUSPENDED ? "SUSPENDED" : "DEAD");
    }
    
    printf("✓ Frame working!\n");
}

void test_process_frame_safe()
{
    Interpreter vm;
    
    Function* main = vm.addFunction("main", 0);
    
    // Variáveis: x=0, count=0
    String* xName = vm.addGlobalEx("x", Value::makeInt(0));
    String* countName = vm.addGlobalEx("count", Value::makeInt(0));
    
    main->chunk.addConstant(Value::makeString(xName));
    main->chunk.addConstant(Value::makeString(countName));
    
    int loopStart = main->chunk.count;
    
    // === CHECK: if (count >= 5) return ===
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(1, 1);  // count
    main->chunk.addConstant(Value::makeInt(5));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(2, 1);
    main->chunk.write(OP_GREATER_EQUAL, 1);
    
    int exitJump = main->chunk.count;
    main->chunk.write(OP_JUMP_IF_FALSE, 1);
    main->chunk.writeShort(0, 1);  // placeholder
    
    main->chunk.write(OP_POP, 1);
    main->chunk.write(OP_RETURN, 1);
    
    // Patch jump
    int afterExit = main->chunk.count;
    main->chunk.code[exitJump + 1] = ((afterExit - exitJump - 3) >> 8) & 0xff;
    main->chunk.code[exitJump + 2] = (afterExit - exitJump - 3) & 0xff;
    
    main->chunk.write(OP_POP, 1);  // pop condition
    
    // === count++ ===
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(1, 1);
    main->chunk.addConstant(Value::makeInt(1));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(3, 1);
    main->chunk.write(OP_ADD, 1);
    main->chunk.write(OP_SET_GLOBAL, 1);
    main->chunk.write(1, 1);
    main->chunk.write(OP_POP, 1);
    
    // === x++ ===
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.addConstant(Value::makeInt(1));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(4, 1);
    main->chunk.write(OP_ADD, 1);
    main->chunk.write(OP_SET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_POP, 1);
    
    // === print(x) ===
    main->chunk.write(OP_GET_GLOBAL, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_PRINT, 1);
    
    // === frame(100) ===
    main->chunk.addConstant(Value::makeInt(100));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(5, 1);
    main->chunk.write(OP_FRAME, 1);
    
    // === LOOP ===
    int loopEnd = main->chunk.count;
    int offset = loopEnd - loopStart + 2;
    main->chunk.write(OP_LOOP, 1);
    main->chunk.writeShort(offset, 1);
    
    Process* proc = vm.addProcess("counter", main);
    // NÃO CHAMES spawnProcess se addProcess já adiciona!
    
    printf("=== Test Process Frame (5 iterations) ===\n");
    
    for (int frame = 0; frame < 10; frame++)
    {
        printf("Frame %d: ", frame);
        vm.update(0.016f);
        printf("\n");
    }
    
    printf("✓ test passed\n");
}

void test_process_frame_loop()
{
    Interpreter vm;
    
    Function* counterFunc = vm.addFunction("counter", 0);
    
    String* xName = vm.addGlobalEx("x", Value::makeInt(0));
    String* countName = vm.addGlobalEx("count", Value::makeInt(0));
    
    counterFunc->chunk.addConstant(Value::makeString(xName));
    counterFunc->chunk.addConstant(Value::makeString(countName));
    
    int loopStart = counterFunc->chunk.count;
    
    // if (count >= 5) return;
    counterFunc->chunk.write(OP_GET_GLOBAL, 1);
    counterFunc->chunk.write(1, 1);
    counterFunc->chunk.addConstant(Value::makeInt(5));
    counterFunc->chunk.write(OP_CONSTANT, 1);
    counterFunc->chunk.write(2, 1);
    counterFunc->chunk.write(OP_GREATER_EQUAL, 1);
    
    int exitJump = counterFunc->chunk.count;
    counterFunc->chunk.write(OP_JUMP_IF_FALSE, 1);
    counterFunc->chunk.writeShort(0, 1);
    
    counterFunc->chunk.write(OP_POP, 1);
    counterFunc->chunk.write(OP_RETURN, 1);
    
    // Patch jump
    int afterJump = counterFunc->chunk.count;
    int jumpDist = afterJump - exitJump - 3;
    counterFunc->chunk.code[exitJump + 1] = (jumpDist >> 8) & 0xff;
    counterFunc->chunk.code[exitJump + 2] = jumpDist & 0xff;
    
    counterFunc->chunk.write(OP_POP, 1);
    
    // count++
    counterFunc->chunk.write(OP_GET_GLOBAL, 1);
    counterFunc->chunk.write(1, 1);
    counterFunc->chunk.addConstant(Value::makeInt(1));
    counterFunc->chunk.write(OP_CONSTANT, 1);
    counterFunc->chunk.write(3, 1);
    counterFunc->chunk.write(OP_ADD, 1);
    counterFunc->chunk.write(OP_SET_GLOBAL, 1);
    counterFunc->chunk.write(1, 1);
    counterFunc->chunk.write(OP_POP, 1);
    
    // x++
    counterFunc->chunk.write(OP_GET_GLOBAL, 1);
    counterFunc->chunk.write(0, 1);
    counterFunc->chunk.addConstant(Value::makeInt(1));
    counterFunc->chunk.write(OP_CONSTANT, 1);
    counterFunc->chunk.write(4, 1);
    counterFunc->chunk.write(OP_ADD, 1);
    counterFunc->chunk.write(OP_SET_GLOBAL, 1);
    counterFunc->chunk.write(0, 1);
    counterFunc->chunk.write(OP_POP, 1);
    
    // print(x)
    counterFunc->chunk.write(OP_GET_GLOBAL, 1);
    counterFunc->chunk.write(0, 1);
    counterFunc->chunk.write(OP_PRINT, 1);
    
    // frame(100)
    counterFunc->chunk.addConstant(Value::makeInt(100));
    counterFunc->chunk.write(OP_CONSTANT, 1);
    counterFunc->chunk.write(5, 1);
    counterFunc->chunk.write(OP_FRAME, 1);
    
    // loop - CORREÇÃO AQUI!
    int loopEnd = counterFunc->chunk.count;
    int offset = loopEnd - loopStart + 3;  // ← +3 em vez de +2
    counterFunc->chunk.write(OP_LOOP, 1);
    counterFunc->chunk.writeShort(offset, 1);
    
    printf("Loop: start=%d, end=%d, offset=%d\n", loopStart, loopEnd, offset);
    
    Process* blueprint = vm.addProcess("counter", counterFunc);
    Process* instance = vm.spawnProcess(blueprint);
    
    printf("=== Test Process Frame (5 iterations max) ===\n");
    
    for (int frame = 0; frame < 10; frame++)
    {
        printf("Frame %d: ", frame);
        vm.update(0.016f);
        printf("\n");
    }
    
    printf("✓ test passed\n");
}
 
void test_frame_no_loop()
{
    Interpreter vm;
    
    Function* main = vm.addFunction("main", 0);
    
    printf("=== Test Frame WITHOUT Loop ===\n");
    
    // Iteração 1
    main->chunk.addConstant(Value::makeInt(1));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_PRINT, 1);
    
    main->chunk.addConstant(Value::makeInt(100));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(1, 1);
    main->chunk.write(OP_FRAME, 1);
    
    // Iteração 2
    main->chunk.addConstant(Value::makeInt(2));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(2, 1);
    main->chunk.write(OP_PRINT, 1);
    
    main->chunk.addConstant(Value::makeInt(100));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(3, 1);
    main->chunk.write(OP_FRAME, 1);
    
    // Iteração 3
    main->chunk.addConstant(Value::makeInt(3));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(4, 1);
    main->chunk.write(OP_PRINT, 1);
    
    main->chunk.write(OP_RETURN, 1);
    
    Process* proc = vm.addProcess("simple", main);
    Process* inst = vm.spawnProcess(proc);
    
    for (int frame = 0; frame < 5; frame++)
    {
        printf("Frame %d: ", frame);
        vm.update(0.016f);
        printf(" (state=%s)\n", 
               inst->state == ProcessState::DEAD ? "DEAD" :
               inst->state == ProcessState::SUSPENDED ? "SUSPENDED" : "RUNNING");
    }
    
    printf("✓ Frame without loop works!\n");
}

void test_loop_with_counter()
{
    Interpreter vm;
    
    Function* main = vm.addFunction("main", 0);
    
    // Usa private[0] como contador
    // private[0] = 0
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.addConstant(Value::makeInt(0));
    main->chunk.write(0, 1);
    main->chunk.write(OP_SET_PRIVATE, 1);
    main->chunk.write(0, 1);  // private[0]
    main->chunk.write(OP_POP, 1);
    
    int loopStart = main->chunk.count;
    
    // if (private[0] >= 3) goto end
    main->chunk.write(OP_GET_PRIVATE, 1);
    main->chunk.write(0, 1);
    main->chunk.addConstant(Value::makeInt(3));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(1, 1);
    main->chunk.write(OP_GREATER_EQUAL, 1);
    
    int exitJump = main->chunk.count;
    main->chunk.write(OP_JUMP_IF_FALSE, 1);
    main->chunk.writeShort(0, 1);
    
    main->chunk.write(OP_POP, 1);
    main->chunk.write(OP_RETURN, 1);  // sai
    
    // patch
    int afterExit = main->chunk.count;
    main->chunk.code[exitJump + 1] = ((afterExit - exitJump - 3) >> 8) & 0xff;
    main->chunk.code[exitJump + 2] = (afterExit - exitJump - 3) & 0xff;
    main->chunk.write(OP_POP, 1);
    
    // private[0]++
    main->chunk.write(OP_GET_PRIVATE, 1);
    main->chunk.write(0, 1);
    main->chunk.addConstant(Value::makeInt(1));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(2, 1);
    main->chunk.write(OP_ADD, 1);
    main->chunk.write(OP_SET_PRIVATE, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_POP, 1);
    
    // print(private[0])
    main->chunk.write(OP_GET_PRIVATE, 1);
    main->chunk.write(0, 1);
    main->chunk.write(OP_PRINT, 1);
    
    // frame(100)
    main->chunk.addConstant(Value::makeInt(100));
    main->chunk.write(OP_CONSTANT, 1);
    main->chunk.write(3, 1);
    main->chunk.write(OP_FRAME, 1);
    
    // loop
    int loopEnd = main->chunk.count;
    int offset = loopEnd - loopStart + 3;
    main->chunk.write(OP_LOOP, 1);
    main->chunk.writeShort(offset, 1);
    
    Process* proc = vm.addProcess("test", main);
    Process* inst = vm.spawnProcess(proc);
    
    for (int i = 0; i < 10; i++)
    {
        printf("Frame %d: ", i);
        vm.update(0.016f);
        printf("\n");
    }
}

void test_multiple_instances()
{
    Interpreter vm;
    
    // === BLUEPRINT: enemy ===
    Function* enemyFunc = vm.addFunction("enemy", 0);
    
    int loopStart = enemyFunc->chunk.count;
    
    // === CHECK: if (private[0] >= 3) return ===
    // Usa private[0] como contador de frames
    enemyFunc->chunk.write(OP_GET_PRIVATE, 1);
    enemyFunc->chunk.write(0, 1);
    enemyFunc->chunk.addConstant(Value::makeInt(3));
    enemyFunc->chunk.write(OP_CONSTANT, 1);
    enemyFunc->chunk.write(0, 1);
    enemyFunc->chunk.write(OP_GREATER_EQUAL, 1);
    
    int exitJump = enemyFunc->chunk.count;
    enemyFunc->chunk.write(OP_JUMP_IF_FALSE, 1);
    enemyFunc->chunk.writeShort(0, 1);
    
    enemyFunc->chunk.write(OP_POP, 1);
    enemyFunc->chunk.write(OP_RETURN, 1);
    
    // Patch jump
    int afterJump = enemyFunc->chunk.count;
    int jumpDist = afterJump - exitJump - 3;
    enemyFunc->chunk.code[exitJump + 1] = (jumpDist >> 8) & 0xff;
    enemyFunc->chunk.code[exitJump + 2] = jumpDist & 0xff;
    
    enemyFunc->chunk.write(OP_POP, 1);
    
    // === private[0]++ (contador) ===
    enemyFunc->chunk.write(OP_GET_PRIVATE, 1);
    enemyFunc->chunk.write(0, 1);
    enemyFunc->chunk.addConstant(Value::makeInt(1));
    enemyFunc->chunk.write(OP_CONSTANT, 1);
    enemyFunc->chunk.write(1, 1);
    enemyFunc->chunk.write(OP_ADD, 1);
    enemyFunc->chunk.write(OP_SET_PRIVATE, 1);
    enemyFunc->chunk.write(0, 1);
    enemyFunc->chunk.write(OP_POP, 1);
    
    // === print("Enemy ") ===
    enemyFunc->chunk.addConstant(Value::makeString("Enemy "));
    enemyFunc->chunk.write(OP_CONSTANT, 1);
    enemyFunc->chunk.write(2, 1);
    enemyFunc->chunk.write(OP_PRINT, 1);
    
    // === print(ID) - usa private[ID] ===
    enemyFunc->chunk.write(OP_GET_PRIVATE, 1);
    enemyFunc->chunk.write((int)PrivateIndex::ID, 1);
    enemyFunc->chunk.write(OP_PRINT, 1);
    
    // === frame(100) ===
    enemyFunc->chunk.addConstant(Value::makeInt(100));
    enemyFunc->chunk.write(OP_CONSTANT, 1);
    enemyFunc->chunk.write(3, 1);
    enemyFunc->chunk.write(OP_FRAME, 1);
    
    // === LOOP ===
    int loopEnd = enemyFunc->chunk.count;
    int offset = loopEnd - loopStart + 3;  // +3 NÃO +2!
    enemyFunc->chunk.write(OP_LOOP, 1);
    enemyFunc->chunk.writeShort(offset, 1);
    
    // Cria blueprint
    Process* blueprint = vm.addProcess("enemy", enemyFunc);
    
    // SPAWN 3 INSTÂNCIAS!
    Process* enemy1 = vm.spawnProcess(blueprint);
    enemy1->privates[(int)PrivateIndex::ID] = Value::makeInt(1);
    enemy1->privates[0] = Value::makeInt(0);  // contador = 0
    
    Process* enemy2 = vm.spawnProcess(blueprint);
    enemy2->privates[(int)PrivateIndex::ID] = Value::makeInt(2);
    enemy2->privates[0] = Value::makeInt(0);  // contador = 0
    
    Process* enemy3 = vm.spawnProcess(blueprint);
    enemy3->privates[(int)PrivateIndex::ID] = Value::makeInt(3);
    enemy3->privates[0] = Value::makeInt(0);  // contador = 0
    
    printf("=== Test: 3 Enemy Instances ===\n");
    printf("Each prints ID 3x then stops\n\n");
    
    for (int frame = 0; frame < 8; frame++)
    {
        printf("Frame %d: ", frame);
        vm.update(0.016f);
        printf("\n");
    }
    
    printf("\n✓ Multiple instances working!\n");
}

int main()
{
    // test_simple_add_print();
    // test_return_value();
    // test_function_call();
    // test_native_call();
    // test_stack_api();

    //test_fiber_yield();
   // test_process_frame();
  // test_process_frame_simple();
   // test_multiple_fibers();


//   test_process_frame_suspend();

//   test_process_frame_loop();
//   test_loop_with_counter();

//   test_frame_no_loop();

test_multiple_instances();

    return 0;
}