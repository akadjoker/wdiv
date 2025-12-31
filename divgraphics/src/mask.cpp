#include "engine.hpp"

Mask::Mask(int w, int h) : width(w), height(h)
{
    grid = new GridNode[width * height];
    for (int i = 0; i < width * height; i++)
    {
        grid[i].walkable = 1;
        grid[i].f = grid[i].g = grid[i].h = 0;
        grid[i].opened = 0;
        grid[i].parent_idx = -1;
        grid[i].next = nullptr;
        grid[i].prev = nullptr;
    }
}

Mask::~Mask()
{
    delete[] grid;
}

void Mask::setOccupied(int x, int y)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        grid[y * width + x].walkable = 0;
    }
}

void Mask::setFree(int x, int y)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        grid[y * width + x].walkable = 1;
    }
}

bool Mask::isOccupied(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return true;
    return grid[y * width + x].walkable == 0;
}

bool Mask::isWalkable(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return false;
    return grid[y * width + x].walkable == 1;
}

void Mask::loadFromImage(const char *imagePath, int threshold)
{
    Image img = LoadImage(imagePath);
    ImageResize(&img, width, height);

    Color *pixels = LoadImageColors(img);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Color c = pixels[y * width + x];
            int brightness = (c.r + c.g + c.b) / 3;
            if (brightness < threshold)
            {
                setOccupied(x, y);
            }
        }
    }

    UnloadImageColors(pixels);
    UnloadImage(img);
}

float Mask::manhattan(int dx, int dy)
{
    return dx + dy;
}

float Mask::euclidean(int dx, int dy)
{
    return sqrtf(dx * dx + dy * dy);
}

float Mask::octile(int dx, int dy)
{
    float F = sqrtf(2.0f) - 1.0f;
    return (dx < dy) ? F * dx + dy : F * dy + dx;
}

float Mask::chebyshev(int dx, int dy)
{
    return (dx > dy) ? dx : dy;
}

float Mask::calcHeuristic(int dx, int dy, PathHeuristic heur, int diag)
{
    switch (heur)
    {
    default:
    case PF_MANHATTAN:
        return (diag) ? octile(dx, dy) : manhattan(dx, dy);
    case PF_EUCLIDEAN:
        return euclidean(dx, dy);
    case PF_OCTILE:
        return octile(dx, dy);
    case PF_CHEBYSHEV:
        return chebyshev(dx, dy);
    }
    return 0;
}

GridNode *Mask::node_add(GridNode *list, GridNode *node)
{
    if (!list)
    {
        node->next = nullptr;
        node->prev = nullptr;
        return node;
    }

    if (list->f > node->f)
    {
        node->next = list;
        node->prev = nullptr;
        list->prev = node;
        return node;
    }

    GridNode *curr = list;
    while (curr->next && curr->next->f <= node->f)
    {
        curr = curr->next;
    }

    node->next = curr->next;
    node->prev = curr;
    if (curr->next)
        curr->next->prev = node;
    curr->next = node;

    return list;
}

GridNode *Mask::node_remove(GridNode *list, GridNode *node)
{
    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    if (list == node)
        return node->next;
    return list;
}

std::vector<Vector2> Mask::findPath(int sx, int sy, int ex, int ey,
                                    int diag,
                                    PathAlgorithm algo,
                                    PathHeuristic heur)
{
    std::vector<Vector2> path;

    // Reset grid
    for (int i = 0; i < width * height; i++)
    {
        grid[i].f = grid[i].g = grid[i].h = 0;
        grid[i].opened = 0;
        grid[i].parent_idx = -1;
        grid[i].next = nullptr;
        grid[i].prev = nullptr;
    }

    if (!isWalkable(sx, sy) || !isWalkable(ex, ey))
        return path;

    int start_idx = sx + sy * width;
    int end_idx = ex + ey * width;

    GridNode *openList = nullptr;
    GridNode *startNode = &grid[start_idx];

    startNode->g = 0;
    startNode->f = 0;
    startNode->opened = 1;
    openList = node_add(openList, startNode);

    int dx[] = {0, 1, 0, -1, -1, 1, 1, -1};
    int dy[] = {-1, 0, 1, 0, -1, -1, 1, 1};
    int dirs = diag ? 8 : 4;

    while (openList)
    {
        GridNode *current = openList;
        openList = node_remove(openList, current);
        current->opened = 2;

        if (current - grid == end_idx)
        {
            int idx = end_idx;
            while (idx != -1)
            {
                path.push_back({(float)(idx % width), (float)(idx / width)});
                idx = grid[idx].parent_idx;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        int cx = (current - grid) % width;
        int cy = (current - grid) / width;

        for (int i = 0; i < dirs; i++)
        {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (!isWalkable(nx, ny))
                continue;

            int n_idx = nx + ny * width;
            GridNode *neighbor = &grid[n_idx];

            if (neighbor->opened == 2)
                continue;

            float cost = ((nx != cx) && (ny != cy)) ? 1.414f : 1.0f;
            float ng = current->g + cost;

            if (!neighbor->opened || ng < neighbor->g)
            {
                neighbor->g = ng;

                if (algo == PATH_ASTAR)
                {
                    neighbor->h = calcHeuristic(abs(nx - ex), abs(ny - ey), heur, diag);
                }
                else
                {
                    neighbor->h = 0; // Dijkstra
                }

                neighbor->f = neighbor->g + neighbor->h;
                neighbor->parent_idx = current - grid;

                if (neighbor->opened == 1)
                {
                    openList = node_remove(openList, neighbor);
                }

                openList = node_add(openList, neighbor);
                neighbor->opened = 1;
            }
        }
    }

    return path;
}