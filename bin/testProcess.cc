def distance(x1, y1, x2, y2) {
    var dx = x2 - x1;
    var dy = y2 - y1;
    write("dx={}, dy={}\n", dx, dy);
    return sqrt(dx*dx + dy*dy);
}

 

process player() 
{
    x = 400;
    y = 300;
    var n=0;
    loop 
    {
        write("Player: x={}, y={}\n", x, y);
        x++;
        y++;

        n++;
        if (n > 20) exit(0); // ou return/stop do processo

        frame;

    }
}

process enemy(startX, startY, speed) 
{
    x = startX;
    y = startY;
    
    var steps = 0;
    while (steps < 10) 
    {
        var dist = distance(x, y, 400, 300);
        write("Enemy: x={}, y={}, dist={}\n", x, y, dist);

   
        
        x += speed;
        y += speed * 0.5;

        
        steps++;
        frame;
    }
    
    print("Enemy done!");
    
}

 
player();
enemy(100, 100, 5);
enemy(200, 150, 3);



 