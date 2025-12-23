#include "interpreter.hpp"
#include "pool.hpp"

static uint64_t PROCESS_IDS = 0;

void ProcessDef::finalize()
{
    for (int i = 0; i < MAX_FIBERS; i++)
    {
        Fiber *fiber = &fibers[i];

        if (fiber->frameCount > 0)
        {
            for (int j = 0; j < fiber->frameCount; j++)
            {
                CallFrame *frame = &fiber->frames[j];
                if (frame->func && frame->ip == nullptr)
                {
                    frame->ip = frame->func->chunk->code;
                }
            }

            if (fiber->ip == nullptr && fiber->frames[0].func)
            {
                fiber->ip = fiber->frames[0].func->chunk->code;
            }
        }
    }
}
void ProcessDef::release()
{
    // for (int i = 0; i < argsNames.size(); i++)
    // {
    //     destroyString(argsNames[i]);
    // }
}
void Process::finalize()
{
    for (int i = 0; i < MAX_FIBERS; i++)
    {
        Fiber *fiber = &fibers[i];

        if (fiber->frameCount > 0)
        {
            for (int j = 0; j < fiber->frameCount; j++)
            {
                CallFrame *frame = &fiber->frames[j];
                if (frame->func && frame->ip == nullptr)
                {
                    frame->ip = frame->func->chunk->code;
                }
            }

            if (fiber->ip == nullptr && fiber->frames[0].func)
            {
                fiber->ip = fiber->frames[0].func->chunk->code;
            }
        }
    }
}

int Interpreter::getProcessPrivateIndex(const char *name)
{
    // Mapeamento fixo
    struct
    {
        const char *name;
        int index;
    } privates[] = {
        {"x", 0},      // PrivateIndex::X
        {"y", 1},      // PrivateIndex::Y
        {"z", 2},      // PrivateIndex::Z
        {"graph", 3},  // PrivateIndex::GRAPH
        {"angle", 4},  // PrivateIndex::ANGLE
        {"size", 5},   // PrivateIndex::SIZE
        {"flags", 6},  // PrivateIndex::FLAGS
        {"id", 7},     // PrivateIndex::ID
        {"father", 8}, // PrivateIndex::FATHER
    };

    for (uint8 i = 0; i < 9; i++)
    {
        if (strcmp(name, privates[i].name) == 0)
        {
            return privates[i].index;
        }
    }

    return -1;
}

ProcessDef *Interpreter::addProcess(const char *name, Function *func)
{
    String *pName = createString(name);
    ProcessDef *existing = nullptr;
    if (processesMap.get(pName, &existing))
    {
        destroyString(pName);
        return existing;
    }

    ProcessDef *proc = new ProcessDef();

    proc->name = pName;
    for (int i = 0; i < MAX_FIBERS; i++)
    {
        proc->fibers[i].state = FiberState::DEAD;
        proc->fibers[i].resumeTime = 0;
        proc->fibers[i].stackTop = proc->fibers[i].stack;
        proc->fibers[i].frameCount = 0;
        proc->fibers[i].ip = nullptr;
    }

    for (int i = 0; i < MAX_PRIVATES; i++)
    {
        proc->privates[i] = Value::makeDouble(0);
    }

    initFiber(&proc->fibers[0], func);
    proc->current = &proc->fibers[0];

    currentFiber = proc->current;

    processesMap.set(pName, proc);
    processes.push(proc);
    return proc;
}

Process *Interpreter::spawnProcess(ProcessDef *blueprint)
{
    Process *instance = ProcessPool::instance().create();

    instance->name = blueprint->name;
    instance->id = PROCESS_IDS++;
    instance->state = FiberState::RUNNING;
    instance->resumeTime = 0;
    instance->nextFiberIndex = 1;
    instance->currentFiberIndex = 0;
    instance->current = nullptr;
    instance->initialized = false;
    instance->exitCode = 0;

    // Clona privates
    for (int i = 0; i < MAX_PRIVATES; i++)
    {
        instance->privates[i] = blueprint->privates[i];
    }

    // Clona fibers
    for (int i = 0; i < MAX_FIBERS; i++)
    {
        Fiber *srcFiber = &blueprint->fibers[i];
        Fiber *dstFiber = &instance->fibers[i];

        if (srcFiber->state == FiberState::DEAD)
        {
            dstFiber->state = FiberState::DEAD;
        }
        // Copia estado
        dstFiber->state = srcFiber->state;
        dstFiber->resumeTime = srcFiber->resumeTime;
        dstFiber->frameCount = srcFiber->frameCount;

        // Copia stack
        size_t stackSize = srcFiber->stackTop - srcFiber->stack;
        for (size_t j = 0; j < stackSize; j++)
        {
            dstFiber->stack[j] = srcFiber->stack[j];
        }
        dstFiber->stackTop = dstFiber->stack + stackSize;

        dstFiber->ip = srcFiber->ip;

        // Copia frames
        for (int j = 0; j < srcFiber->frameCount; j++)
        {
            dstFiber->frames[j].func = srcFiber->frames[j].func;

            dstFiber->frames[j].ip = srcFiber->frames[j].ip;

            // Ajusta slots para nova stack
            ptrdiff_t offset = srcFiber->frames[j].slots - srcFiber->stack;
            dstFiber->frames[j].slots = dstFiber->stack + offset;
        }
    }

    instance->current = &instance->fibers[0];

    aliveProcesses.push(instance);

    if (hooks.onStart)
    {
        hooks.onStart(instance);
    }

    return instance;
}

uint32 Interpreter::getTotalProcesses() const
{
    return static_cast<uint32>(processes.size());
}

uint32 Interpreter::getTotalAliveProcesses() const
{
    return uint32(aliveProcesses.size());
}

int Interpreter::addGlobal(const char *name, Value value)
{
    String *pName = createString(name);
    if (globals.exist(pName))
    {
        destroyString(pName);
        return -1;
    }
    globals.set(pName, value);
    globalList.push(value);

    return (int)(globalList.size() - 1);
}

String *Interpreter::addGlobalEx(const char *name, Value value)
{
    String *pName = createString(name);
    if (globals.exist(pName))
    {
        destroyString(pName);
        return nullptr;
    }
    globals.set(pName, value);
    globalList.push(value);

    return pName;
}

Value Interpreter::getGlobal(uint32 index)
{
    if (index >= globalList.size())
        return Value::makeNil();
    return globalList[index];
}

void Interpreter::addFiber(Process *proc, Function *func)
{
    if (proc->nextFiberIndex >= MAX_FIBERS)
    {
        runtimeError("Too many fibers in process");
        return;
    }

    int index = proc->nextFiberIndex++;
    initFiber(&proc->fibers[index], func);
}

void Process::release()
{
}

void Interpreter::update(float deltaTime)
{
    currentTime += deltaTime;
    lastFrameTime = deltaTime;

    // for (size_t i = 0; i < aliveProcesses.size(); i++)
    // {
    //     Process *proc = aliveProcesses[i];

    //     // Suspenso?
    //     if (proc->state == FiberState::SUSPENDED)
    //     {
    //         if (currentTime >= proc->resumeTime)
    //         {
    //             proc->state = FiberState::RUNNING;
    //         }
    //         else
    //         {
    //             continue;
    //         }
    //     }

    //     // Morto?
    //     if (proc->state == FiberState::DEAD)
    //     {
    //         aliveProcesses[i] = aliveProcesses.back();
    //         //Warning(" Process %s (id=%u) is dead. Cleaning up. ", proc->name->chars(), proc->id);
    //         cleanProcesses.push(proc);
    //         aliveProcesses.pop();
    //         continue;
    //     }

    //     currentProcess = proc;
    //     run_process_step(proc);
    //     if (hooks.onUpdate)
    //         hooks.onUpdate(proc, deltaTime);

    // }

    size_t i = 0;
    while (i < aliveProcesses.size())
    {
        Process *proc = aliveProcesses[i];

        // Suspended?
        if (proc->state == FiberState::SUSPENDED)
        {
            if (currentTime >= proc->resumeTime)
                proc->state = FiberState::RUNNING;
            else
            {
                i++;
                continue;
            }
        }

        // Dead? -> remove da lista
        if (proc->state == FiberState::DEAD)
        {
            // remove sem manter ordem
            aliveProcesses[i] = aliveProcesses.back();
            cleanProcesses.push(proc);
            aliveProcesses.pop();
            continue;
        }

        currentProcess = proc;
        run_process_step(proc);
        if (hooks.onUpdate)
            hooks.onUpdate(proc, deltaTime);

        i++;
    }

    if (cleanProcesses.size() >= 1)
    {
        // Warning(" Cleaning up %zu processes ", cleanProcesses.size());

        for (size_t j = 0; j < cleanProcesses.size(); j++)
        {
            Process *proc = cleanProcesses[j];
            // Warning(" Releasing process %s (id=%u) ", proc->name->chars(), proc->id);

            if (hooks.onDestroy)
                hooks.onDestroy(proc, proc->exitCode);

            proc->release();
            ProcessPool::instance().destory(proc);
        }
        cleanProcesses.clear();
    }
}

void Interpreter::run_process_step(Process *proc)
{
    // printf("  [run_process_step] Starting\n");

    Fiber *fiber = get_ready_fiber(proc);
    if (!fiber)
    {
        // Warning("  [run_process_step] No ready fiber");
        return;
    }

    proc->current = fiber;
    FiberResult result = run_fiber(fiber);

    // Warning("  [run_process_step] result.reason=%d, instructions=%d",   (int)result.reason, result.instructionsRun);

    if (proc->state == FiberState::DEAD)
    {
        proc->initialized = false;
        return;
    }

    if (result.reason == FiberResult::FIBER_YIELD)
    {
        fiber->state = FiberState::SUSPENDED;
        fiber->resumeTime = currentTime + result.yieldMs / 1000.0f;
        return;
    }

    if (result.reason == FiberResult::PROCESS_FRAME)
    {
        proc->state = FiberState::SUSPENDED;
        proc->resumeTime = currentTime + (lastFrameTime * result.framePercent / 100.0f);

        // proc->frameCounter = result.framePercent / 100;
        if (!proc->initialized)
        {
            proc->initialized = true;
        }

        return;
    }

    if (result.reason == FiberResult::FIBER_DONE)
    {
        fiber->state = FiberState::DEAD;
        // Warning("  [run_process_step] Fiber DONE");
        return;
    }
}

void Interpreter::render()
{
    if (!hooks.onRender)
        return;
    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *proc = aliveProcesses[i];
        if (proc->state != FiberState::DEAD && proc->initialized)
        {
            hooks.onRender(proc);
        }
    }
}