
// var gravity = 0.5;

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

//     def move(tex)
//     {
//         self.x +=  self.vx;
//         self.y +=  self.vy;

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
//         DrawTexture(tex, self.x,self.y, WHITE);
//     }
// }

// InitWindow(800, 450, "BuLang with Raylib");

// var RED = Color(255, 0, 0, 255);
// var BLACK = Color(0, 0, 0, 255);
// var WHITE = Color(255, 255, 255, 255);
// var LIGHTGRAY = Color(200, 200, 200, 255);
// var tex = LoadTexture("assets/wabbit_alpha.png");

// //SetTargetFPS(30);

// lista = [];

// while (!WindowShouldClose())
// {
//     BeginDrawing();

//     ClearBackground(BLACK);

//     DrawPixel(200, 200, RED);

//     DrawTexture(tex, GetMouseX(), GetMouseY(), WHITE);

//     if (IsMouseButtonDown(0))
//     {
//         for(var i=0;i<500;i++)
//         {
//             var vx = GetMouseX();
//             var vy = GetMouseY();
//             lista.push(Sprite(vx,vy));
//         }

//         //var sprite = Sprite(200.0,200.0);
//        // lista.push(sprite);
//     }

//     if (lista.length() > 0)
//     {
//         for (var i = 0; i < lista.length(); i++)
//         {
//             var sprite = lista[i];
//             sprite.move(tex);

//         }
//     }

//     DrawText(format("count {}", lista.length()), 10, 30, 20, LIGHTGRAY);
//     DrawFPS(10, 10);

//     EndDrawing();
// }

// UnloadTexture(tex);
// CloseWindow();

//  def add(a, b) {
//     return a + b;
// }


// def fib(n) {
//     if (n <= 1) return n;
//     return fib(n-1) + fib(n-2);
// }


// class Point {
//   var x, y;

//   def init(x, y) {
//     self.x = x;
//     self.y = y;
//   }
// }

// var p = Point(10, 20);
// print(p.x); // 10
// print(p.y); // 20
// print("✅ Test 2 passed");

// class Counter {
//   var count;

//   def init() { self.count = 0; }

//   def increment() { self.count = self.count + 1; }

//   def get() { return self.count; }
// }

// var c = Counter();
// c.increment();
// c.increment();
// print(c.get()); // 2
// print("✅ Test 3 passed");




// print(add(2,2));
// // Recursão

// print(fib(7));
 

// class Builder {
//     var x, y;
    
//     def init() {
//         self.x = 0;
//         self.y = 0;
//     }
    
//     def setX(v) {
//         self.x = v;
//         return self;
//     }
    
//     def setY(v) {
//         self.y = v;
//         return self;
//     }
// }

// var b = Builder();
// b.setX(10).setY(20);
// print(b.x);  // 10
// print(b.y);  // 20
// print("✅ Test 4 passed");

// class Animal {
//     var name;
    
//     def init(name) {
//         self.name = name;
//     }
    
//     def speak() {
//         print(self.name + " makes sound");
//     }
// }

// class Dog : Animal {
//     def init(name) {
//         self.name = name;
//     }
// }

// var d = Dog("Rex");
// d.speak();  // Rex makes sound
// print("✅ Test 5 passed");


// class Animal {
//     var name;
    
//     def init(name) {
//         self.name = name;
//         print("Animal init: " + name);
//     }
// }

// class Dog : Animal {
//     var breed;
    
//     def init(name, breed) {
//         super.init(name);
//         self.breed = breed;
//         print("Dog init: " + breed);
//     }
// }

// var d = Dog("Rex", "Labrador");
// print(d.name);   // Rex
// print(d.breed);  // Labrador
// print("✅ Test 6 passed");


// class Zoo 
// {
//   var aardvark, baboon, cat, donkey, elephant, fox;
//   def init() 
//   {
//     self.aardvark = 1;
//     self.baboon   = 1;
//     self.cat      = 1;
//     self.donkey   = 1;
//     self.elephant = 1;
//     self.fox      = 1;
//   }
//   def ant()    { return self.aardvark; }
//   def banana() { return self.baboon; }
//   def tuna()   { return self.cat; }
//   def hay()    { return self.donkey; }
//   def grass()  { return self.elephant; }
//   def mouse()  { return self.fox; }
// }

// // var zoo = Zoo();
// // var sum = 0;
// // var start = clock();
// // while (sum < 100000000) 
// // {
// //   sum = sum + zoo.ant()
// //             + zoo.banana()
// //             + zoo.tuna()
// //             + zoo.hay()
// //             + zoo.grass()
// //             + zoo.mouse();
// // }

// // var end =  clock() - start;
// // print(end);
// // print(sum);


// struct Vec2 
// {
//     x, y
// };

// var p=Vec2(1,1);

// print(p);

// def addPoint(p)
// {
//     p.x =100;
//     p.y =100;

    

// }

// addPoint(p);

// print(p);

// test_add.bu

// test_operators.bu


// test_with_class.bu

var GRAVITY = 0.6;

class Test {
    var y, vy;
    
    def init() {
        self.y = 100;
        self.vy = 0;
    }
    
    def aframe() {
        write("y: {}, vy: {}\n", self.y, self.vy);
        
        self.vy = self.vy + GRAVITY;
        write("After gravity: vy = {}\n", self.vy);
        
        self.y = self.y + self.vy;
        write("After move: y = {}\n", self.y);
    }
}

var t = Test();

write("=== FRAME 1 ===\n");
t.aframe();

write("\n=== FRAME 2 ===\n");
t.aframe();

write("\n=== FRAME 3 ===\n");
t.aframe();