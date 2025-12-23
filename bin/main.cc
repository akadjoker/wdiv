
process tiro(x,y)
{

    print("Tiro");
    print(father.x);
    print(father.y);

    father.x=1000;

}


process tmp()
{

}

process nave(x,y,size,z)
{
  
    tiro(100,100);
    tiro(100,100);
 
 
}

tmp();
tmp();
tmp();

nave(200,4000,32,-1);

 tmp();

print("byby");