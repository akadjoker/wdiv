struct Vec2 
{
    x, y
};


process bunny1(startX, startY) 
{
    x = startX;
    y = startY;  
    
    var vx = (rand(200) - 100) / 10.0;
    var vy = (rand(200) - 100) / 10.0;
    var gravity = 0.5;
    var live = (rand(100) + 200);
    var speed = Vec2(vx,vy);
    
    loop 
    {
        
        x = x + vx;
        y = y + vy;
        vy = vy + gravity;
        
        if (y > 600) 
        {
            y = 600;
            vy = vy * -0.85;
        }
        
        if (x < 0 || x > 800) 
        {
            vx = vx * -1;
        }

        live = live - 1;
        frame;
        
        if (live < 0) 
        {
            break;
        }
        
    }

//    print("Bunny process ended");
}

process bunny(x,y) 
{
    var vx = 2;
    var vy = 0;

animate:
    x += vx;
    y += vy;
    frame(16);

    gosub check_bounds;
    goto animate;

check_bounds:
    if (y > 600) { y = 600; vy *= -0.8; }
    if (x < 0 || x > 800) { vx *= -1; }
    return;
}


process flecha(delay, ly)
{
	
    y=ly;
 
	loop
    {
		x += 5; 
        if (x > 800) 
        {
        x = -2;
        }

		frame(delay);
	}
}


process main()
{
    x=100;
    y=100;
    loop
    {
         if (key(32)) 
         {
             print("breaking loop\n");
             break;
         }

         if(mouse_down(0))
         {
             bunny1(400, 300);
         }

         if(mouse_down(1))
         {
            for (var i = 0; i < 1000; i++)
            {
                 bunny1(mouseX(), mouseY());
            }
         }
         
        x+=1;
        if (x > 800) x = 0;
         
        
        frame(1);
    }
}
 
flecha(10, 140);
flecha(200, 190);
flecha(400, 240);
flecha(1800, 290);
main();
