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

enemy();
 