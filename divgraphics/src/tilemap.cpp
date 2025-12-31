#include "engine.hpp"
#include <cstdio>
#include <algorithm>
#include <cmath>

extern GraphLib gGraphLib;
extern Scene gScene;

Tilemap::Tilemap()
    : tiles(nullptr), width(0), height(0), tile_size(32),
      tileset_id(0), spacing(0), margin(0), tileset_cols(16),
      grid_type(ORTHO), brush_id(1), brush_radius(1.0f),graph(-1)
 
{
}

Tilemap::~Tilemap()
{
    if (tiles)
        delete[] tiles;

    
}

// Setup [web:26]
void Tilemap::init(int w, int h, int size, int graph)
{
    if (tiles)
        delete[] tiles;

    width = w;
    height = h;
    tile_size = size;
    tiles = new Tile[w * h]{};
    this->graph = graph;
}

void Tilemap::clear()
{
    if (!tiles)
        return;

    for (int i = 0; i < width * height; i++)
        tiles[i] = {0, false, 0, {}, 0};
}
 
void Tilemap::setTile(int gx, int gy, const Tile &t)
{
    if (gx >= 0 && gx < width && gy >= 0 && gy < height)
        tiles[gy * width + gx] = t;
}

 
Vector2 Tilemap::gridToWorld(int gx, int gy)
{
    if (grid_type == ORTHO)
    {
        return {(float)gx * tile_size, (float)gy * tile_size};
    }
    else // HEXAGON
    {
        float offset = (gy % 2) * (tile_size / 2.0f);
        return {(float)gx * tile_size + offset, (float)gy * tile_size * 0.75f};
    }
}

void Tilemap::worldToGrid(Vector2 pos, int &gx, int &gy)
{
    if (grid_type == ORTHO)
    {
        gx = (int)(pos.x / tile_size);
        gy = (int)(pos.y / tile_size);
    }
    else // HEXAGON
    {
        gy = (int)(pos.y / (tile_size * 0.75f));
        float offset = (gy % 2) * (tile_size / 2.0f);
        gx = (int)((pos.x - offset) / tile_size);
    }
}

void Tilemap::paintRect(int cx, int cy, int radius, uint16 id)
{
    for (int dy = -radius; dy <= radius; dy++)
        for (int dx = -radius; dx <= radius; dx++)
            setTile(cx + dx, cy + dy, {id, true, 0, {}, 0});
}

void Tilemap::paintCircle(int cx, int cy, float radius, uint16 id)
{
    int r = (int)radius;
    for (int dy = -r; dy <= r; dy++)
    {
        for (int dx = -r; dx <= r; dx++)
        {
            if (dx * dx + dy * dy <= radius * radius)
                setTile(cx + dx, cy + dy, {id, true, 0, {}, 0});
        }
    }
}

void Tilemap::erase(int cx, int cy, int radius)
{
    for (int dy = -radius; dy <= radius; dy++)
        for (int dx = -radius; dx <= radius; dx++)
            setTile(cx + dx, cy + dy, {0, false, 0, {}, 0});
}

void Tilemap::eraseCircle(int cx, int cy, float radius)
{
    int r = (int)radius;
    for (int dy = -r; dy <= r; dy++)
    {
        for (int dx = -r; dx <= r; dx++)
        {
            if (dx * dx + dy * dy <= radius * radius)
                setTile(cx + dx, cy + dy, {0, false, 0, {}, 0});
        }
    }
}

void Tilemap::fill(int gx, int gy, uint16 id)
{
    Tile *start = getTile(gx, gy);
    if (!start)
        return;

    uint16 old_id = start->id;
    if (old_id == id)
        return;

    struct Coord
    {
        int x, y;
    };

    std::vector<Coord> stack;
    stack.reserve(256);
    stack.push_back({gx, gy});

    while (!stack.empty())
    {
        Coord c = stack.back();  
        stack.pop_back();

        Tile *t = getTile(c.x, c.y);
        if (!t || t->id != old_id)
            continue;

        setTile(c.x, c.y, {id, true, 0, {}, 0});

        stack.push_back({c.x + 1, c.y});
        stack.push_back({c.x - 1, c.y});
        stack.push_back({c.x, c.y + 1});
        stack.push_back({c.x, c.y - 1});
    }
}

void Tilemap::getCollidingTiles(Rectangle bounds, std::vector<Rectangle> &out)
{
    int gx0, gy0, gx1, gy1;

    worldToGrid({bounds.x, bounds.y}, gx0, gy0);
    worldToGrid({bounds.x + bounds.width, bounds.y + bounds.height}, gx1, gy1);

    // Clamp + margin [web:20]
    gx0 = std::max(0, gx0 - 1);
    gy0 = std::max(0, gy0 - 1);
    gx1 = std::min(width - 1, gx1 + 1);
    gy1 = std::min(height - 1, gy1 + 1);

    for (int gy = gy0; gy <= gy1; gy++)
    {
        for (int gx = gx0; gx <= gx1; gx++)
        {
            Tile *t = getTile(gx, gy);
            if (!t || t->id == 0)
                continue;

            Vector2 world = gridToWorld(gx, gy);
            Rectangle tile_rect = {
                world.x, world.y,
                (float)tile_size, (float)tile_size};

            if (CheckCollisionRecs(bounds, tile_rect))
                out.push_back(tile_rect);
        }
    }
}

void Tilemap::getCollidingSolids(Rectangle bounds, std::vector<Rectangle> &out)
{
    int gx0, gy0, gx1, gy1;

    worldToGrid({bounds.x, bounds.y}, gx0, gy0);
    worldToGrid({bounds.x + bounds.width, bounds.y + bounds.height}, gx1, gy1);

    gx0 = std::max(0, gx0 - 1);
    gy0 = std::max(0, gy0 - 1);
    gx1 = std::min(width - 1, gx1 + 1);
    gy1 = std::min(height - 1, gy1 + 1);

    for (int gy = gy0; gy <= gy1; gy++)
    {
        for (int gx = gx0; gx <= gx1; gx++)
        {
            Tile *t = getTile(gx, gy);
            if (!t || t->id == 0 || !t->solid)
                continue;

            Vector2 world = gridToWorld(gx, gy);
            Rectangle tile_rect = {
                world.x, world.y,
                (float)tile_size, (float)tile_size};

            if (CheckCollisionRecs(bounds, tile_rect))
                out.push_back(tile_rect);
        }
    }
}

void Tilemap::render(Vector2 scroll)
{
    int start_x = (int)(scroll.x / tile_size) - 1;
    int start_y = (int)(scroll.y / tile_size) - 1;
    int end_x = start_x + (GetScreenWidth() / tile_size) + 3;
    int end_y = start_y + (GetScreenHeight() / tile_size) + 3;

    for (int gy = start_y; gy < end_y; gy++)
    {
        for (int gx = start_x; gx < end_x; gx++)
        {
            Tile *t = getTile(gx, gy);
            if (!t || t->id == 0)
                continue;

            Vector2 world = gridToWorld(gx, gy);
            float screen_x = world.x - scroll.x;
            float screen_y = world.y - scroll.y;

            DrawRectangleLines((int)screen_x, (int)screen_y,
                               tile_size, tile_size, GRAY);
            DrawText(TextFormat("%d", t->id),
                     (int)screen_x + 4, (int)screen_y + 4, 10, WHITE);
        }
    }
}

void Tilemap::renderWithTexture(Vector2 scroll)
{
    if (graph==-1)
    {
        render(scroll);
        return;
    }

    Graph *g = gGraphLib.getGraph(graph);
    if (!g)
        return;
    
      

    int start_x = (int)(scroll.x / tile_size) - 1;
    int start_y = (int)(scroll.y / tile_size) - 1;
    int end_x = start_x + (GetScreenWidth() / tile_size) + 3;
    int end_y = start_y + (GetScreenHeight() / tile_size) + 3;

   for (int gy = start_y; gy < end_y; gy++)
    {
        for (int gx = start_x; gx < end_x; gx++)
        {
            Tile* t = getTile(gx, gy);
            if (!t || t->id == 0) continue;
            
            // Posição mundo -> screen
            Vector2 world = gridToWorld(gx, gy);
            float screen_x = world.x - scroll.x;
            float screen_y = world.y - scroll.y;
            
            // Calcula clip do tile no atlas
            int tile_id = t->id - 1;
            int atlas_x = (tile_id % tileset_cols) * (tile_size + spacing) + margin;
            int atlas_y = (tile_id / tileset_cols) * (tile_size + spacing) + margin;
            
            Rectangle clip = {
                (float)atlas_x,
                (float)atlas_y,
                (float)tile_size,
                (float)tile_size
            };
            
 
            RenderClipSize(
                atlas,
                screen_x, screen_y,
                (float)tile_size, (float)tile_size,
                clip,
                false, false,  // flip (podes adicionar por tile depois)
                WHITE,
                0  // blend mode
            );
        }
    }
}

void Tilemap::renderGrid(Vector2 scroll)
{
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();

    for (int x = 0; x < screen_w; x += tile_size)
        DrawLine(x, 0, x, screen_h, {50, 50, 50, 100});

    for (int y = 0; y < screen_h; y += tile_size)
        DrawLine(0, y, screen_w, y, {50, 50, 50, 100});
}

// Save/Load [web:26]
bool Tilemap::save(const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f)
        return false;

    fwrite(&width, sizeof(int), 1, f);
    fwrite(&height, sizeof(int), 1, f);
    fwrite(&tile_size, sizeof(int), 1, f);
    fwrite(&grid_type, sizeof(int), 1, f);
    fwrite(&graph, sizeof(int), 1, f);
    fwrite(tiles, sizeof(Tile), width * height, f);

    fclose(f);
    printf("Tilemap saved: %s\n", filename);
    return true;
}

bool Tilemap::load(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return false;

    int w, h, size, gt,graph;
    fread(&w, sizeof(int), 1, f);
    fread(&h, sizeof(int), 1, f);
    fread(&size, sizeof(int), 1, f);
    fread(&gt, sizeof(int), 1, f);
    fread(&graph, sizeof(int), 1, f);

    init(w, h, size, graph);
    grid_type = (GridType)gt;
    fread(tiles, sizeof(Tile), w * h, f);

    fclose(f);
    printf("Tilemap loaded: %s\n", filename);
    return true;
}

// ============================================
// TILEMAP EDITOR IMPLEMENTATION
// ============================================

TilemapEditor::TilemapEditor()
    : tilemap(nullptr), scroll({0, 0})
{
}

void TilemapEditor::setTilemap(Tilemap *tm)
{
    tilemap = tm;
}

void TilemapEditor::update(Vector2 mouse_pos)
{
    if (!tilemap)
        return;

    int gx, gy;
    tilemap->worldToGrid({mouse_pos.x + scroll.x, mouse_pos.y + scroll.y}, gx, gy);

    // PAINT RECT (LEFT)
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        tilemap->paintRect(gx, gy, (int)tilemap->brush_radius, tilemap->brush_id);
    }

    // PAINT CIRCLE (MIDDLE)
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        tilemap->paintCircle(gx, gy, tilemap->brush_radius, tilemap->brush_id);
    }

    // ERASE (RIGHT)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        tilemap->eraseCircle(gx, gy, tilemap->brush_radius);
    }

    // FILL (F)
    if (IsMouseButtonPressed(KEY_F))
    {
        tilemap->fill(gx, gy, tilemap->brush_id);
    }

    // SCROLL (BACK BUTTON)
    if (IsMouseButtonDown(MOUSE_BUTTON_BACK))
    {
        scroll.x += GetMouseDelta().x;
        scroll.y += GetMouseDelta().y;
    }
}

void TilemapEditor::handleInput()
{
    if (!tilemap)
        return;

    // Select ID (1-9)
    for (int i = 1; i <= 9; i++)
    {
        if (IsKeyPressed(KEY_ZERO + i))
            tilemap->brush_id = i;
    }

    // Brush size
    if (IsKeyPressed(KEY_EQUAL))
        tilemap->brush_radius += 0.5f;
    if (IsKeyPressed(KEY_MINUS) && tilemap->brush_radius > 0.5f)
        tilemap->brush_radius -= 0.5f;

    // Toggle grid type
    if (IsKeyPressed(KEY_G))
    {
        tilemap->grid_type = (Tilemap::GridType)(1 - tilemap->grid_type);
    }

    // Save
    if (IsKeyPressed(KEY_S))
    {
        tilemap->save("tilemap.tm");
    }

    // Load
    if (IsKeyPressed(KEY_L))
    {
        tilemap->load("tilemap.tm");
    }

    // Clear
    if (IsKeyPressed(KEY_C) && IsKeyDown(KEY_LEFT_CONTROL))
    {
        tilemap->clear();
    }
}

void TilemapEditor::render()
{
    if (!tilemap)
        return;

    tilemap->render(scroll);
    tilemap->renderGrid(scroll);

    // UI
    DrawText(TextFormat("ID: %d (1-9)", tilemap->brush_id), 10, 10, 20, WHITE);
    DrawText(TextFormat("Brush: %.1f (+-)", tilemap->brush_radius), 10, 35, 20, WHITE);
    DrawText(TextFormat("Grid: %s (G)",
                        tilemap->grid_type == Tilemap::ORTHO ? "ORTHO" : "HEX"),
             10, 60, 20, WHITE);
    DrawText("LEFT=Rect | MID=Circle | RIGHT=Erase | F=Fill", 10, 85, 16, GRAY);
    DrawText("S=Save | L=Load | C+Ctrl=Clear", 10, 105, 16, GRAY);
}