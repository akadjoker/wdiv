# bunnymark_raylib.py
import raylib as rl

class Sprite:
    def __init__(self):
        self.x = 400.0
        self.y = 300.0
        self.vx = 0.0
        self.vy = 0.0

lista = []

rl.init_window(800, 600, "Bunnymark Python")
rl.set_target_fps(60)

# Texture (assume que tens bunny.png)
texture = rl.load_texture("bunny.png")

while not rl.window_should_close():
    # Add sprites
    if rl.is_mouse_button_down(rl.MOUSE_LEFT_BUTTON):
        for i in range(100):
            s = Sprite()
            s.x = rl.get_mouse_x()
            s.y = rl.get_mouse_y()
            s.vx = (i % 200 - 100) * 0.1
            s.vy = (i % 200 - 100) * 0.1
            lista.append(s)
    
    # Update
    for s in lista:
        s.x += s.vx
        s.y += s.vy
        s.vy += 0.5
        
        if s.y > 600:
            s.y = 600
            s.vy *= -0.85
        if s.x < 0 or s.x > 800:
            s.vx *= -1
    
    # Draw
    rl.begin_drawing()
    rl.clear_background(rl.WHITE)
    
    for s in lista:
        rl.draw_texture(texture, int(s.x), int(s.y), rl.WHITE)
    
    rl.draw_fps(10, 10)
    rl.draw_text(f"Sprites: {len(lista)}", 10, 30, 20, rl.BLACK)
    rl.end_drawing()

rl.close_window()
 