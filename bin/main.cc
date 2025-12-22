 var a = 1;
    var b = 2;
    
    switch(a) {
        case 1: {
            print("Outer case 1");
            
            switch(b) {  // Se OP_DUP: funciona ✅
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
    