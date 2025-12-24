 


struct Vec2 
{
    x, y,name,age
};

 


process enemy(x,y)
{
    print("create process");
 
 
}


enemy(1,1);


var v = Vec2(1,2,1,23);
print(v);


var arr = [1, 2, 3];
var empty = [];
var mixed = [1, "hello", true, nil,v];

print(mixed);

// Acesso
print(arr[0]);     // 1
print(arr[-1]);    // 3 (Python-style!)

// Modificar
arr[1] = 10;
print(arr[1]);     // 10

// Métodos
arr.push(4);
print(arr.len());  // 4

var x = arr.pop();
print(x);          // 4

arr.clear();
print(arr.len());  // 0

// Loop
var nums = [10, 20, 30];
for (var i = 0; i < nums.len(); i++) {
    print(nums[i]);
}

// 2D arrays
var matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];

print(matrix[1][2]);  // 6