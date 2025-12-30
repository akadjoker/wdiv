def add(a, b) {
        return a + b;
    }
    
    process counter(max) {
        var i = 0;
        while (i < max) {
            print(i);
            i++;
            frame;
        }
    }
    
    // Spawn processos
    var p1 = counter(3);
    var p2 = counter(5);
    
    print("Spawned:");
    print(p1);
    print(p2);
