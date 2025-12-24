var a = 1;
var b = 2;

switch(a) {
    case 1: {
        print("Outer case 1");
        
        switch(b) {  
            case 2: {
                print("Inner case 2");
            }
            case 3: {
                print("Inner case 3");
            }
        }
        
        print("Back to outer");
    }
    case 2: {
        print("Outer case 2");
    }
}


var a = 12;  // 0b1100
var b = 10;  // 0b1010

print(a & b);   // 8  (0b1000)
print(a | b);   // 14 (0b1110)
print(a ^ b);   // 6  (0b0110)
print(~a);      // -13 (complemento de 2)
print(a << 2);  // 48 (12 * 4)
print(a >> 2);  // 3  (12 / 4)

var name ="LUIS";
var hp =123;
var score =199;


// write() - imprime diretamente
write("Player: {}, HP: {}\n", name, hp);
// Output: Player: Hero, HP: 100

// format() - retorna string
var msg = format("Score: {} ", score);


print(msg);


var a = msg.length();

print(a);


print(msg.upper());
print(msg.lower());


var name ="luis";

var completo =name.concat(" santos");

print(completo);

print(completo.sub(5,11).replace("santos","rosa"));


var s = "Hello World";

// At - acesso a char
print(s.at(0));     // "H"
print(s.at(6));     // "W"
print(s.at(-1));    // "d" (Python-style negativo!)
print(s.at(100));   // "" (out of bounds = string vazia)

// Contains
print(s.contains("World"));  // true
print(s.contains("xyz"));    // false
print(s.contains(""));       // true (string vazia sempre contém)

// Trim
var padded = "  hello  ";
print(padded.trim());  // "hello"

var tabs = "\t\n  text  \n\t";
print(tabs.trim());    // "text"

// StartsWith / EndsWith
print(s.starts_with("Hello"));  // true
print(s.starts_with("World"));  // false
print(s.ends_with("World"));    // true
print(s.ends_with("Hello"));    // false

// IndexOf
print(s.index_of("World"));  // 6
print(s.index_of("xyz"));    // -1 (não encontrado)
print(s.index_of("l"));      // 2 (primeira ocorrência)

// Repeat
print("Ha".repeat(3));   // "HaHaHa"
print("=".repeat(10));   // "=========="
print("x".repeat(0));    // ""
// 3 (split cada char)

// Method chaining!
var text = "  Hello World  ";
var result = text.trim().lower().replace("world", "universe");
print(result);  // "hello universe"

// Validação
var email = "user@example.com";
if (email.contains("@") && email.contains(".")) {
    print("Valid email format");
}

// Parsing
var url = "https://example.com/path";
if (url.starts_with("https://")) {
    var domain = url.sub(8, url.index_of("/", 8));
    print(domain);  // "example.com"
}

var url = "https://example.com/path/to/file";
var protocolEnd = url.index_of("://");
print(protocolEnd);  // 5


var firstSlash = url.index_of("/", 8);  //  Procura '/' depois de "https://"
print(firstSlash);   // 18

var domain = url.sub(protocolEnd + 3, firstSlash);
print(domain);  // "example.com"



{
var email = "user@example.com";
var atPos = email.length();

}

