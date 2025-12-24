def worker(id, interval) 
{
    var count = 0;
    while (count < 5) 
    {
        write("Worker {} tick {}\n", id, count);
        yield(interval);
        count++;
    }
    format("Worker {} DONE\n", id);
}

process factory() 
{
    x = 0;
    
    fiber worker(1, 100);   // Rápido
    fiber worker(2, 300);   // Médio
    fiber worker(3, 500);   // Lento
    
    // Main fiber
    var i = 0;
    while (i < 30) 
    {
        write("Factory: x={}\n", x);
        x++;
        frame;
        i++;
    }
}

factory();