#include "code.hpp"
#include "value.hpp"

 
 

Code::Code(size_t capacity)
    :  m_capacity(capacity), count(0)
{
    code  = (uint8*) aAlloc(capacity * sizeof(uint8));
    lines = (int*)aAlloc(capacity * sizeof(int));

    constants.reserve(8);
    m_frozen=false;
}

void Code::freeze()
{
    m_frozen=true;
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

 


 

void Code::writeShort(uint16 value, int line)
{
    write((value >> 8) & 0xff, line);   
    write(value & 0xff, line);          
}

void Code::reserve(size_t capacity)
{
    if (capacity > m_capacity)
    {
        uint8 *newCode = (uint8*)aRealloc(code, capacity * sizeof(uint8));
        if (!newCode) return;  // Falhou, code ainda é válido
        
        int *newLine = (int*)aRealloc(lines, capacity * sizeof(int));
        if (!newLine)
        {
            code = newCode;  // CRITICAL: Salva o novo ponteiro!
            // lines ainda é o antigo (realloc não mudou)
            DEBUG_BREAK_IF(true);
            return;
        }
        
        code = newCode;
        lines = newLine;
        m_capacity = capacity;
    }
}



void Code::write(uint8 instruction, int line)
{
    DEBUG_BREAK_IF(m_frozen);
    if (m_capacity < count + 1)
    {
        size_t newCapacity = GROW_CAPACITY(m_capacity);
        Warning("Reserv %ld to %ld",m_capacity,newCapacity);
        reserve(newCapacity);  
    }
    
    code[count] = instruction;
    lines[count] = line;
    count++;
}

 
// Corrige operator[]
uint8 Code::operator[](size_t index)
{
    DEBUG_BREAK_IF(index >= count);  // count, não m_capacity
    return code[index];
}
