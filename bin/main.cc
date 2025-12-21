
def patrol(a,b) 
{
    print("Before yield");
        yield(5.0); 
    print("After yield");
}

fiber patrol(1,2);