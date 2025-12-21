#include "code.hpp"
#include "value.hpp"

 
 

Code::Code(size_t capacity)
    :  m_capacity(capacity), count(0)
{
    code  = (uint8*) aAlloc(capacity * sizeof(uint8));
    lines = (int*)aAlloc(capacity * sizeof(int));

    constants.reserve(8);
}

 
 
int Code::addConstant(Value value)
{
    constants.push(value);
    return static_cast<int>(constants.size() - 1);
}

 



void Code::clear()
{
    if(code)
    {
    aFree(code);
    code=nullptr;
    }
    if(lines)
    {
    aFree(lines);
    lines=nullptr;
    }
    constants.destroy();
    m_capacity=0;
    count=0;
}

void Code::reserve(size_t capacity)
{
    if (capacity > m_capacity)
    {
       

        uint8 *newCode  = (uint8*) (std::realloc(code,  capacity * sizeof(uint8)));
        int *newLine = (int*)(std::realloc(lines, capacity * sizeof(int)));

        if (!newCode || !newLine)
        {
            aFree(newCode);
            aFree(newLine);
            DEBUG_BREAK_IF(newCode == nullptr || newLine == nullptr);
            return;
        }



        code = newCode;
        lines = newLine;      
        m_capacity = capacity;
    }
}



void Code::write(uint8 instruction, int line)
{
    if (m_capacity < count + 1)
    {
        size_t oldCapacity = m_capacity;
        m_capacity = GROW_CAPACITY(oldCapacity);
        uint8 *newCode  = (uint8*) (std::realloc(code,  m_capacity * sizeof(uint8)));
        int *newLine = (int*)(std::realloc(lines, m_capacity * sizeof(int)));
        if (!newCode || !newLine)
        {
            aFree(newCode);
            aFree(newLine);
            DEBUG_BREAK_IF(newCode == nullptr || newLine == nullptr);
            return;
        }
        code = newCode;
        lines = newLine;      

    }
    
    code[count] = instruction;  
    lines[count] = line;
    count++;
}

void Code::writeShort(uint16 value, int line)
{
    write((value >> 8) & 0xff, line);   
    write(value & 0xff, line);          
}

uint8 Code::operator[](size_t index)
{
    DEBUG_BREAK_IF(index > m_capacity);
    return code[index];
}