process teste() {
    print("A");
    gosub sub;
    print("C");
    exit;

sub:
    print("B");
    return;
}

process teste2() 
{
    print("start");
    gosub sub;
    print("after gosub");
sub:
    print("in sub");
    return;
}


process teste3() {
    var i = 0;

loop_start:
    write("loop {} \n",i);
    gosub sub;
    i += 1;
    if (i < 3) goto loop_start;
    exit;

sub:
    print("  in sub");
    return;
}


process teste4() {
    print("start");
    gosub a;
    print("end");
    exit;

a:
    print("A");
    gosub b;
    print("A return");
    return;

b:
    print("B");
    return;
}

process loop_test() 
{
    var i = 0;

mainloop:
    gosub body;
    i += 1;
    if (i < 5) goto mainloop;
    exit;

body:
    write("i = {} \n", i);
    return;
}


loop_test();
teste2();
teste3();
teste4();