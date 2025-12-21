process bunny(startX, startY) 
{
    x = startX;
    y = startY;
    graph = 1;
    
    
    var vx = (rand(200) - 100) / 10.0;
    var vy = (rand(200) - 100) / 10.0;
    var gravity = 0.5;
    
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
        
        frame;
    }
}
process main()
{
    loop
    {
         if (key(32)) 
         {
             print("breaking loop\n");
             break;
         }

         if(mouse_down(0))
         {
             bunny(400, 300);
         }

         if(mouse_down(1))
         {
            for (var i = 0; i < 100; i++)
            {
                 bunny(mouseX(), mouseY());
            }
         }
         
         
        
        frame;
    }
}

main();
