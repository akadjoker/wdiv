struct State {
    var x, y;
};

def teste(state) 
{
    while (true) 
    {
    print("A");
    state.x = state.x + 1;  
    yield(100);
    print("B");
    state.y = state.y + 1;
    yield(100);
    }
}

process enemy() 
{
    var state = State(1, 1);
    
    fiber teste(state);   
    self.x=0;
    
    while (true) 
    {
        write("{} {}\n", state.x, state.y);
        frame;
    }
}

enemy();