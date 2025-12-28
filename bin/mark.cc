 

process bunny(startX, startY) 
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
             bunny(400, 300);
         }

         if(mouse_down(1))
         {
            for (var i = 0; i < 100; i++)
            {
                 bunny(mouseX(), mouseY());
            }
         }
         
        x+=1;
        if (x > 800) x = 0;
         
        
        frame(1);
    }
}
 
 
main();
