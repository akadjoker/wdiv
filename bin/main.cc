
var gravity = 0.5;

class Sprite
{
    var x, y, vx, vy;

    def init(x, y)
    {
        self.x =  x;
        self.y =  y;
        self.vx =  (rand(200) - 100) / 10.0;
        self.vy =  (rand(200) - 100) / 10.0;
    
    }

    def move(tex)
    {
        self.x = self.x + self.vx;
        self.y = self.y + self.vy;

        
       
         self.vy = self.vy + 0.5;
        
        if (self.y > 400) 
        {
            self.y = 400;
            self.vy = self.vy * -0.85;
        }
        
        if (self.x < 0 || self.x > 800) 
        {
            self.vx = self.vx * -1.0;
        }
        DrawTexture(tex, self.x,self.y, WHITE);
    }
}

InitWindow(800, 450, "BuLang with Raylib");

var RED = Color(255, 0, 0, 255);
var BLACK = Color(0, 0, 0, 255);
var WHITE = Color(255, 255, 255, 255);
var LIGHTGRAY = Color(200, 200, 200, 255);
var tex = LoadTexture("assets/wabbit_alpha.png");

//SetTargetFPS(30);

lista = [];

while (!WindowShouldClose())
{
    BeginDrawing();

    ClearBackground(BLACK);
    
    DrawPixel(200, 200, RED);
    
    DrawTexture(tex, GetMouseX(), GetMouseY(), WHITE);

    if (IsMouseButtonDown(0))
    {
        for(var i=0;i<500;i++)
        {
            var vx = GetMouseX();
            var vy = GetMouseY();
            lista.push(Sprite(vx,vy));
        }

        //var sprite = Sprite(200.0,200.0);
       // lista.push(sprite);
    }

    if (lista.length() > 0)
    {
        for (var i = 0; i < lista.length(); i++)
        {
            var sprite = lista[i];
            sprite.move(tex);
            
        }
    }


    DrawText(format("count {}", lista.length()), 10, 30, 20, LIGHTGRAY);
    DrawFPS(10, 10);

    EndDrawing();
}

UnloadTexture(tex);
CloseWindow();


 