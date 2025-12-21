#include "interpreter.hpp"
#include "pool.hpp"

static uint64_t PROCESS_IDS = 0;

Process *Interpreter::addProcess(const char *name, Function *func)
{
    String *pName = createString(name);
    Process *existing = nullptr;
    if (processesMap.get(pName, &existing))
    {
        destroyString(pName);
        return existing;
    }

    Process *proc = new Process();

    proc->name = pName;
    proc->id = PROCESS_IDS++;
    proc->state = FiberState::RUNNING;
    proc->resumeTime = 0;
    proc->nextFiberIndex = 1;
    proc->current = nullptr;
    proc->next = nullptr;
    proc->prev = nullptr;

    for (int i = 0; i < MAX_FIBERS; i++)
    {
        proc->fibers[i].state = FiberState::DEAD;
        proc->fibers[i].resumeTime = 0;
        proc->fibers[i].stackTop = proc->fibers[i].stack;
        proc->fibers[i].frameCount = 0;
    }

    for (int i = 0; i < MAX_PRIVATES; i++)
    {
        proc->privates[i] = Value::makeDouble(0.0);
    }

    initFiber(&proc->fibers[0], func);
    proc->current = &proc->fibers[0];

    currentFiber = proc->current;

    processesMap.set(pName, proc);
    processes.push(proc);
    return proc;
}

Process *Interpreter::spawnProcess(Process *blueprint)
{
    // Process* instance = new Process();

    //Process *instance = (Process *)arena.Allocate(sizeof(Process));

    Process *instance = new Process();



    instance->name = blueprint->name; // Compartilha nome
    instance->id = PROCESS_IDS++;     // ID único
    instance->state = FiberState::RUNNING;
    instance->resumeTime = 0;
    instance->nextFiberIndex = blueprint->nextFiberIndex;
    instance->currentFiberIndex = 0; 
    instance->current = nullptr;
    instance->next = nullptr;
    instance->prev = nullptr;

    // CLONA PRIVATES (estado da entidade)
    for (int i = 0; i < MAX_PRIVATES; i++)
    {
        instance->privates[i] = blueprint->privates[i];
    }

    // CLONA FIBERS
    for (int i = 0; i < MAX_FIBERS; i++)
    {
        Fiber *srcFiber = &blueprint->fibers[i];
        Fiber *dstFiber = &instance->fibers[i];

        // Copia estado da fiber
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

        // Copia frames (call stack)
        for (int j = 0; j < srcFiber->frameCount; j++)
        {
            dstFiber->frames[j].func = srcFiber->frames[j].func; // Compartilha código
            dstFiber->frames[j].ip = srcFiber->frames[j].ip;

            // Ajusta slots para apontar para a nova stack
            ptrdiff_t offset = srcFiber->frames[j].slots - srcFiber->stack;
            dstFiber->frames[j].slots = dstFiber->stack + offset;
        }

        // Ajusta IP
        dstFiber->ip = srcFiber->ip;
    }

    instance->current = &instance->fibers[0];
    aliveProcesses.push_back(instance);


    if (hooks.onStart)
        hooks.onStart(instance);

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

void Interpreter::destroyProcess(Process *proc)
{
    if (!proc)
        return;

    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        if (aliveProcesses[i] == proc)
        {
            // Swap-and-pop
            aliveProcesses[i] = aliveProcesses.back();
            aliveProcesses.pop_back();
            break;
        }
    }

    if (proc->name)
    {
        destroyString(proc->name);
        proc->name = nullptr;
    }

    for (size_t i = 0; i < processes.size(); i++)
    {
        if (processes[i] == proc)
        {
            processes[i] = processes.back();
            processes.pop();
            break;
        }
    }
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

    // if (aliveProcesses.size() == 0)
    // {

    //     currentProcess = mainProcess;
    //     return;
    // }

    size_t i = 0;
    while (i < aliveProcesses.size())
    {
        Process* proc = aliveProcesses[i];

        // Suspended?
        if (proc->state == FiberState::SUSPENDED)
        {
            if (currentTime >= proc->resumeTime)
                proc->state = FiberState::RUNNING;
            else {
                i++;
                continue;
            }
        }

        // Dead? -> remove da lista
        if (proc->state == FiberState::DEAD)
        {
            // remove sem manter ordem
            aliveProcesses[i] = aliveProcesses.back();

            Warning(" Process %s (id=%u) is dead. Cleaning up. ", proc->name->chars(), proc->id);

            if (hooks.onDestroy)
                hooks.onDestroy(proc, proc->exitCode);
            cleanProcesses.push_back(proc);
            aliveProcesses.pop_back();
            continue; // NÃO incrementa i (porque trouxe outro para i)
        }

        currentProcess = proc;
        run_process_step(proc, 30);
        if (proc->state != FiberState::DEAD && hooks.onUpdate)
            hooks.onUpdate(proc, deltaTime);

        // Se morreu durante este step, remove já
        if (proc->state == FiberState::DEAD)
        {
            aliveProcesses[i] = aliveProcesses.back();
            if (hooks.onDestroy)
                hooks.onDestroy(proc, proc->exitCode);
            Warning(" Process %s (id=%u) is dead. Cleaning up. ", proc->name->chars(), proc->id);
            cleanProcesses.push_back(proc);
            aliveProcesses.pop_back();
            continue;
        }

        i++;
    }
    if (cleanProcesses.size() > 50)
    {
        Warning(" Cleaning up %zu processes ", cleanProcesses.size());
    
            for (size_t j = 0; j < cleanProcesses.size(); j++)
            {
                Process* proc = cleanProcesses[j];
                Warning(" Releasing process %s (id=%u) ", proc->name->chars(), proc->id);
                proc->release();
                delete proc;
                
            }
            cleanProcesses.clear();
        }
}

 
void Interpreter::run_process_step(Process *proc, int maxInstructions)
{
    // printf("  [run_process_step] Starting\n");

    int instructionsExecuted = 0;
    while (instructionsExecuted < maxInstructions)
    {
        Fiber *fiber = get_ready_fiber(proc);
        if (!fiber)
        {
            // Warning("  [run_process_step] No ready fiber");
            break;
        }

        proc->current = fiber;
        FiberResult result = run_fiber(fiber, maxInstructions - instructionsExecuted);

        // Warning("  [run_process_step] result.reason=%d, instructions=%d",   (int)result.reason, result.instructionsRun);

        instructionsExecuted += result.instructionsRun;

        if (proc->state == FiberState::DEAD)
            break;

        if (result.reason == FiberResult::FIBER_YIELD)
        {
            fiber->state = FiberState::SUSPENDED;
            fiber->resumeTime = currentTime + result.yieldMs / 1000.0f;
            continue;
        }

        if (result.reason == FiberResult::PROCESS_FRAME)
        {
            proc->state = FiberState::SUSPENDED;
            proc->resumeTime = currentTime + (0.01667f * static_cast<float>(result.framePercent) / 100.0f);
            // Warning("  [run_process_step] FRAME! resumeTime=%.3f", proc->resumeTime);
            break;
        }

        if (result.reason == FiberResult::FIBER_DONE)
        {
            fiber->state = FiberState::DEAD;
            // Warning("  [run_process_step] Fiber DONE");
            continue;
        }
    }
}