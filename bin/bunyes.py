# bunnymark_logic.py
import time

start = time.perf_counter()
class Sprite:
    def __init__(self):
        self.x = 0.0
        self.y = 0.0
        self.vx = 0.0
        self.vy = 0.0

lista = []

# Cria 100K sprites
print("Creating 100K sprites...")
for i in range(100000):
    s = Sprite()
    s.x = 400.0
    s.y = 300.0
    s.vx = (i % 200 - 100) * 0.1
    s.vy = (i % 200 - 100) * 0.1
    lista.append(s)

print("Updating 60 frames...")

# Update puro (SEM rendering!)
for f in range(60):
    for i in range(len(lista)):
        s = lista[i]
        
        s.x = s.x + s.vx
        s.y = s.y + s.vy
        s.vy = s.vy + 0.5  # gravity
        
        if s.y > 400.0:
            s.y = 400.0
            s.vy = s.vy * -0.85
        
        if s.x < 0.0 or s.x > 800.0:
            s.vx = s.vx * -1.0

elapsed = (time.perf_counter() - start) 
print(f"elapsed: {elapsed:.2f} ms")