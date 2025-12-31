#include "engine.hpp"

extern GraphLib gGraphLib;
Scene gScene;

int GraphLib::load(const char *name, const char *texturePath)
{
    Texture2D tex = LoadTexture(texturePath);
    if (tex.id == 0)
    {
        return 0; // erro ao carregar
    }

    // SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    //   SetTextureWrap(tex, TEXTURE_WRAP_REPEAT); // SÓ AQUI, 1x quando carrega

    Graph g = {};
    g.id = (int)graphs.size();
    g.texture = textures.size();
    g.width = tex.width;
    g.height = tex.height;
    g.clip = {0, 0, (float)tex.width, (float)tex.height};
    strncpy(g.name, name, MAXNAME - 1);
    g.name[MAXNAME - 1] = '\0';
    g.points.resize(1);
    Vector2 &v = g.points.back();
    v.x = g.clip.width / 2.0f;
    v.y = g.clip.height / 2.0f;

    graphs.push_back(g);
    textures.push_back(tex);

    return g.id;
}

int GraphLib::loadAtlas(const char *name, const char *texturePath, int count_x, int count_y)
{
    Texture2D tex = LoadTexture(texturePath);
    if (tex.id == 0)
        return -1;

    int tile_w = tex.width / count_x;
    int tile_h = tex.height / count_y;

    int firstId = (int)graphs.size();

    for (int y = 0; y < count_y; y++)
    {
        for (int x = 0; x < count_x; x++)
        {
            Graph g = {};
            g.id = graphs.size();
            g.texture = textures.size();
            g.width = tile_w;
            g.height = tile_h;
            g.clip = {(float)(x * tile_w), (float)(y * tile_h), (float)tile_w, (float)tile_h};
            snprintf(g.name, MAXNAME, "%s_%d_%d", name, x, y);
            g.points.resize(1);
            Vector2 &v = g.points.back();
            v.x = g.clip.width / 2.0f;
            v.y = g.clip.height / 2.0f;

            graphs.push_back(g);
        }
    }

    textures.push_back(tex);

    return firstId;
}

int GraphLib::addSubGraph(int id, const char *name, int ix, int iy, int iw, int ih)
{
    Graph *original = getGraph(id);
    if (!original)
        return 0;

    Graph g = {};
    g.id = (int)graphs.size();
    g.texture = original->texture;
    g.width = iw;
    g.height = ih;
    g.clip = {(float)ix, (float)iy, (float)iw, (float)ih};
    strncpy(g.name, name, MAXNAME - 1);
    g.name[MAXNAME - 1] = '\0';
    g.points.resize(1);
    Vector2 &v = g.points.back();
    v.x = g.clip.width / 2.0f;
    v.y = g.clip.height / 2.0f;

    graphs.push_back(g);

    return g.id;
}

Graph *GraphLib::getGraph(int id)
{
    if (id < 0 || id >= (int)graphs.size())
        return &graphs[0];
    return &graphs[id];
}

void GraphLib::create()
{

    Image image = GenImageChecked(32, 32, 4, 4, WHITE, BLACK);
    defaultTexture = LoadTextureFromImage(image);
    UnloadImage(image);

    Graph g = {};
    g.id = 0;
    g.texture = 0;
    g.width = defaultTexture.width;
    g.height = defaultTexture.height;
    g.clip = {0, 0, (float)g.width, (float)g.height};
    strncpy(g.name, "dummy", 4);
    g.name[5] = '\0';

    graphs.push_back(g);
    textures.push_back(defaultTexture);
}

void GraphLib::destroy()
{
    UnloadTexture(defaultTexture);
    for (size_t i = 0; i < textures.size(); i++)
    {
        UnloadTexture(textures[i]);
    }
    textures.clear();
    graphs.clear();
}

void InitScene()
{
    int screeWidth = GetScreenWidth();
    int screeHeight = GetScreenHeight();
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        gScene.layers[i].back = -1;
        gScene.layers[i].front = -1;

        gScene.layers[i].mode = LAYER_MODE_TILEX | LAYER_MODE_TILEY;

        gScene.layers[i].size.width = screeWidth;
        gScene.layers[i].size.height = screeHeight;
        gScene.layers[i].size.x = 0;
        gScene.layers[i].size.y = 0;

        gScene.layers[i].scroll_x = 0;
        gScene.layers[i].scroll_y = 0;
        gScene.layers[i].scroll_factor_x = 1;
        gScene.layers[i].scroll_factor_y = 1;
    }
}

void DestroyScene()
{
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        gScene.layers[i].destroy();
    }
    gGraphLib.destroy();
}

void RenderScene()
{
 // rlDisableBackfaceCulling();
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        gScene.layers[i].render();
    }

    for (size_t i = 0; i < gScene.nodesToRemove.size(); i++)
    {
        delete gScene.nodesToRemove[i];
    }
    gScene.nodesToRemove.clear();



//   //  rlEnableBackfaceCulling();
//     DrawRectangle(10, 10, 300, 200, ColorAlpha(BLACK, 0.7f));
//    // DrawText("F1: Toggle Quadtree | F2: Toggle Bounds", 10, 10, 16, WHITE);
//     DrawFPS(20, 20);
//     DrawText(TextFormat("Collision Statics: %d Dynamic: %d", gScene.staticEntities.size(), gScene.dynamicEntities.size()), 20,40, 15, WHITE);

    // Debug: desenha quadtree (F1 para toggle)
    static bool showQuadtree = false;
    if (IsKeyPressed(KEY_F1))
        showQuadtree = !showQuadtree;

    if (showQuadtree && gScene.staticTree)
    {
        gScene.staticTree->draw();
    }

    // Debug: desenha bounds das entities (F2 para toggle)
    static bool showBounds = false;
    if (IsKeyPressed(KEY_F2))
        showBounds = !showBounds;

    if (showBounds)
    {
        for (int i = 0; i < MAX_LAYERS; i++)
        {
            Layer &l = gScene.layers[i];
            for (size_t j = 0; j < l.nodes.size(); j++)
            {
                Entity *e = l.nodes[j];
                if (e->shape)
                {
                    Rectangle b = e->getBounds();
                    Color c = (e->flags & B_STATIC) ? YELLOW : RED;

                        int screenX = b.x;
                        int screenY = b.y;
                        screenX -= l.scroll_x;
                        screenY -= l.scroll_y;

                     DrawRectangleLines(screenX, screenY,
                                        (int)b.width, (int)b.height, c);
                }
            }
        }
    }

    static bool showShapes = false;
    if (IsKeyPressed(KEY_F3))
        showShapes = !showShapes;

    if (showShapes)
    {
        for (int i = 0; i < MAX_LAYERS; i++)
        {
            for (Entity *e : gScene.layers[i].nodes)
            {
                if (e->shape && (e->flags & B_COLLISION))
                {
                    Color col = (e->flags & B_STATIC) ? YELLOW : GREEN;
                    

                    e->shape->draw(e,col);
                }
            }
        }
    }


}

void InitCollision(int x, int y, int width, int height, CollisionCallback onCollision)
{
    Rectangle r;
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    gScene.initCollision(r);
    gScene.setCollisionCallback(onCollision);
    UpdateCollision();
}

void UpdateCollision()
{
    gScene.updateCollision();
}

 
Entity *CreateEntity(int graphId, int layer, double x, double y)
{
    return gScene.addEntity(graphId, layer, x, y);
}

void SetLayerMode(int layer, uint8 mode)
{
    if (layer < 0 || layer > MAX_LAYERS)
        layer = 0;

    gScene.layers[layer].mode = mode;
}

void SetLayerScrollFactor(int layer, double x, double y)
{
    if (layer < 0 || layer > MAX_LAYERS)
        layer = 0;

    gScene.layers[layer].scroll_factor_x = x;
    gScene.layers[layer].scroll_factor_y = y;
}

void SetLayerSize(int layer, int x, int y, int width, int height)
{

    if (layer > MAX_LAYERS)
        layer = MAX_LAYERS - 1;

    if (layer < 0)
    {
        for (int i = 0; i < MAX_LAYERS; i++)
        {
            gScene.layers[i].size.x = x;
            gScene.layers[i].size.y = y;
            gScene.layers[i].size.width = width;
            gScene.layers[i].size.height = height;
        }
        return;
    }

    gScene.layers[layer].size.x = x;
    gScene.layers[layer].size.y = y;
    gScene.layers[layer].size.width = width;
    gScene.layers[layer].size.height = height;
}

void SetLayerBackGraph(int layer, int graph)
{
    if (layer < 0 || layer > MAX_LAYERS)
        layer = 0;

    gScene.layers[layer].back = graph;
}

void SetLayerFrontGraph(int layer, int graph)
{
    if (layer < 0 || layer > MAX_LAYERS)
        layer = 0;

    gScene.layers[layer].front = graph;
}

void SetScroll(double x, double y)
{
    gScene.scroll_x = x;
    gScene.scroll_y = y;

 
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        Layer &layer = gScene.layers[i];

      
        layer.scroll_x = x ;
        layer.scroll_y = y ;

        // Clamp aos limites do mundo (se layer tem bounds definidos)
        if (layer.size.width > 0 && layer.size.height > 0)
        {
            // Limite esquerdo/superior
            if (layer.scroll_x < layer.size.x)
                layer.scroll_x = layer.size.x;
            if (layer.scroll_y < layer.size.y)
                layer.scroll_y = layer.size.y;

            // Limite direito/inferior (world_width - screen_width)
            double max_scroll_x = layer.size.x + layer.size.width - screenWidth;
            double max_scroll_y = layer.size.y + layer.size.height - screenHeight;

            if (layer.scroll_x > max_scroll_x)
                layer.scroll_x = max_scroll_x;
            if (layer.scroll_y > max_scroll_y)
                layer.scroll_y = max_scroll_y;
        }
    }
}

int LoadGraph(const char *name, const char *texturePath)
{
    return gGraphLib.load(name, texturePath);
}

int LoadAtlas(const char *name, const char *texturePath, int count_x, int count_y)
{
    return gGraphLib.loadAtlas(name, texturePath, count_x, count_y);
}

int LoadSubGraph(int id, const char *name, int x, int iy, int iw, int ih)
{
    return gGraphLib.addSubGraph(id, name, x, iy, iw, ih);
}

void MainCamera::setTarget(double tx, double ty)
{
    target_x = tx;
    target_y = ty;
}

void MainCamera::update(double deltaTime)
{
    // Smooth lerp para a posição alvo
    double lerpFactor = 1.0 - pow(1.0 - smoothness, deltaTime * 60.0);

    x += (target_x + x) * lerpFactor;
    y += (target_y + y) * lerpFactor;

    // Aplica bounds com suavização
    if (use_bounds)
    {
        double screen_w = GetScreenWidth();
        double screen_h = GetScreenHeight();

        double min_x = bounds.x;
        double min_y = bounds.y;
        double max_x = bounds.x + bounds.width - screen_w;
        double max_y = bounds.y + bounds.height - screen_h;

        // Clamp suave (com easing nos limites)
        if (x < min_x)
        {
            double diff = min_x - x;
            x += diff * 0.5; // Easing ao bater no limite
        }
        else if (x > max_x)
        {
            double diff = x - max_x;
            x -= diff * 0.5;
        }

        if (y < min_y)
        {
            double diff = min_y - y;
            y += diff * 0.5;
        }
        else if (y > max_y)
        {
            double diff = y - max_y;
            y -= diff * 0.5;
        }
    }
}

void MainCamera::setBounds(double minX, double minY, double maxX, double maxY)
{

    bounds = {(float)minX, (float)minY, (float)(maxX - minX), (float)(maxY - minY)};
    use_bounds = true;
}
