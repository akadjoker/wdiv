
var gravity = 0.5;

struct Dados
{
    var a,b,c;
};

class Sprite
{
    var x, y, vx, vy;

    def init(x, y)
    {
        self.x = x;
        self.y = y;
        self.vx = (rand(200) - 100) / 10.0;
        self.vy = (rand(200) - 100) / 10.0;
    }

    def move(tex)
    {
        self.x += self.vx;
        self.y += self.vy;

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
        DrawTexture(tex, self.x, self.y, WHITE);
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
lista.push(Sprite(10,10));
lista.push(Sprite(10,10));
lista.push(Sprite(10,10));

var i=0;

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
    }

    if (lista.length() > 0)
    {
        for (i = 0; i < lista.length(); i++)
        {
           var sprite = lista[i];
          sprite.move(tex);

        }
    }

     if (IsMouseButtonDown(1))
    {
       var dados = Dados(1,2,3);
       print(dados);
    }
  

    //DrawText(format("count {} ", lista.length()), 400, 20, 20, RED);
    DrawFps(10, 10);

   

    EndDrawing();
}

UnloadTexture(tex);
CloseWindow();

//  var a =[];

//  var gravity = 0.5;

// class Sprite
// {
//     var x, y, vx, vy;

//     def init(x, y)
//     {
//         self.x =  x;
//         self.y =  y;
//         self.vx =  (rand(200) - 100) / 10.0;
//         self.vy =  (rand(200) - 100) / 10.0;

//     }

//     def move()
//     {
//         self.x += self.vx;
//         self.y += self.vy;

//          self.vy = self.vy + 0.5;

//         if (self.y > 400)
//         {
//             self.y = 400;
//             self.vy = self.vy * -0.85;
//         }

//         if (self.x < 0 || self.x > 800)
//         {
//             self.vx = self.vx * -1.0;
//         }

//     }
// }

//  a.push(Sprite(10,10));

//  for (var i = 0; i < 10000; i++)
//  {
//     a.push(Sprite(10,10));
//  }

//  for (var i = 0; i < 10000; i++)
//  {
//     a[i].move();
//      format("count {}", a.length());
//  }
 