struct Function
{
    int arity;
    Code chunk;
    String name;
    
    bool hasReturn;
    bool isProcess; 

    Function(const char *n, int a)
    : arity(a), name(n), hasReturn(false),isProcess(false)  {}
};

struct CallFrame
{
    Function *func;
    uint8 *ip;
    Value *slots;
};

struct Fiber
{
    Value *stack;
    uint32_t stackSize;
    uint32_t stackTop;

    CallFrame *frames;
    uint32_t frameCount;

    enum State
    {
        RUNNING,
        SUSPENDED,
        DEAD
    } state;
};

struct Process
{
    Fiber *current;
    Array fibers;
};

struct VM
{
    Value *stack;
    uint32_t stackSize;
    uint32_t stackTop;

    CallFrame *frames;
    uint32_t frameCount;

    HashMap<String *, String *, StringHasher, StringEq> stringTable;
    HashMap<String *, Value, StringHasher, StringEq> globals;

    Fiber *currentFiber;
    Process *currentProcess;

    // allocators
    StringPool stringPool;
    Arena arena;
};
