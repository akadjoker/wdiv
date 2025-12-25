def patrol() {
    var count = 0;
    while (count < 10) {
        write("Patrol tick {}\n", count);
        yield(100);  // Suspende 100ms
        count++;
    }
    print("Patrol done!");
}

def attack() {
    var count = 0;
    while (count < 5) 
    {
        write("Attack tick {}\n", count);
        yield(200);  // Suspende 200ms
        count++;
    }
    print("Attack done!");
}

process enemy() {
    x = 100;
    
    fiber patrol();  // Fiber 1
    fiber attack();  // Fiber 2
    
    // Fiber 0 (main) continua aqui
    var i = 0;
    while (i < 2000) {
       // write("Main fiber: x={}\n", x);
        x++;
        frame;
        i++;
    }
}

struct State {
    var x, y;
};

def teste(state) 
{
    while (true) 
    {
    print("A");
    state.x = state.x + 1;  
    yield(100);
    print("B");
    state.y = state.y + 1;
    yield(100);
    }
}

process enemy2() 
{
    var state = State(1, 1);
    
    fiber teste(state);   
   
    
    while (true) 
    {
        write("{} {}\n", state.x, state.y);
        frame;
    }
}

enemy2();


enemy();
 