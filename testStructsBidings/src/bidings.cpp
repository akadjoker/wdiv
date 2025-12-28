#include "bidings.hpp"
#include <cmath>

namespace RaylibBindings
{

    // ========================================
    // VECTOR2
    // ========================================

    static void vec2Ctor(Interpreter *vm, void *data, int argc, Value *args)
    {
        Vector2 *v = (Vector2 *)data;

        // aceita int/double/float
        auto toFloat = [&](int i) -> float
        {
            if (args[i].isFloat())
                return args[i].asFloat();
            if (args[i].isDouble())
                return (float)args[i].asDouble();
            if (args[i].isInt())
                return (float)args[i].asInt();
            vm->runtimeError("Vector2 expects numbers");
            return 0.0f;
        };

        if (argc == 2)
        {
            v->x = toFloat(0);
            v->y = toFloat(1);
        }
        else if (argc == 0)
        {
            v->x = 0;
            v->y = 0;
        }
        else
        {
            vm->runtimeError("Vector2(x,y) expects 2 args");
        }
    }

    void registerVector2(Interpreter &vm)
    {
        auto *vec2 = vm.registerNativeStruct(
            "Vector2",
            sizeof(Vector2),
            nullptr,
            nullptr, // Sem destructor
            "raylib");

        vm.addStructField(vec2, "x", offsetof(Vector2, x), FieldType::FLOAT);
        vm.addStructField(vec2, "y", offsetof(Vector2, y), FieldType::FLOAT);
    }

    // ========================================
    // VECTOR3
    // ========================================

    void registerVector3(Interpreter &vm)
    {
        auto *vec3 = vm.registerNativeStruct(
            "Vector3",
            sizeof(Vector3),
            nullptr,
            nullptr, "raylib");

        vm.addStructField(vec3, "x", offsetof(Vector3, x), FieldType::FLOAT);
        vm.addStructField(vec3, "y", offsetof(Vector3, y), FieldType::FLOAT);
        vm.addStructField(vec3, "z", offsetof(Vector3, z), FieldType::FLOAT);
    }

    // ========================================
    // RECTANGLE
    // ========================================

    void rectangle_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        Rectangle *v = (Rectangle *)buffer;
        v->x = 0;
        v->y = 0;
        v->width = 0;
        v->height = 0;
    }

    void registerRectangle(Interpreter &vm)
    {
        auto *rect = vm.registerNativeStruct(
            "Rectangle",
            sizeof(Rectangle),
            rectangle_ctor,
            nullptr, "raylib");

        vm.addStructField(rect, "x", offsetof(Rectangle, x), FieldType::FLOAT);
        vm.addStructField(rect, "y", offsetof(Rectangle, y), FieldType::FLOAT);
        vm.addStructField(rect, "width", offsetof(Rectangle, width), FieldType::FLOAT);
        vm.addStructField(rect, "height", offsetof(Rectangle, height), FieldType::FLOAT);
    }

    // ========================================
    // COLOR
    // ========================================

    void color_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        Color *v = (Color *)buffer;
        v->r = args[0].asByte();
        v->g = args[1].asByte();
        v->b = args[2].asByte();
        v->a = args[3].asByte();
        //   Info("Create Color(%d, %d, %d, %d)", v->r, v->g, v->b, v->a);
    }

    void color_dtor(Interpreter *vm, void *buffer)
    {
        Color *v = (Color *)buffer;

        //   Info("Delet Color(%d, %d, %d, %d)", v->r, v->g, v->b, v->a);
    }

    void registerColor(Interpreter &vm)
    {
        auto *color = vm.registerNativeStruct(
            "Color",
            sizeof(Color),
            color_ctor,
            color_dtor, "raylib");

        vm.addStructField(color, "r", offsetof(Color, r), FieldType::BYTE);
        vm.addStructField(color, "g", offsetof(Color, g), FieldType::BYTE);
        vm.addStructField(color, "b", offsetof(Color, b), FieldType::BYTE);
        vm.addStructField(color, "a", offsetof(Color, a), FieldType::BYTE);
    }

    // ========================================
    // CAMERA2D
    // ========================================

    void registerCamera2D(Interpreter &vm)
    {
        auto *camera = vm.registerNativeStruct(
            "Camera2D",
            sizeof(Camera2D),
            nullptr,
            nullptr,
            "raylib");

        vm.addStructField(camera, "offset", offsetof(Camera2D, offset), FieldType::POINTER); // Vector2
        vm.addStructField(camera, "target", offsetof(Camera2D, target), FieldType::POINTER); // Vector2
        vm.addStructField(camera, "rotation", offsetof(Camera2D, rotation), FieldType::FLOAT);
        vm.addStructField(camera, "zoom", offsetof(Camera2D, zoom), FieldType::FLOAT);
    }

    // ========================================
    // WINDOW
    // ========================================

    Value native_InitWindow(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            return Value::makeNil();
        }
        int width = args[0].asInt();
        int height = args[1].asInt();
        const char *title = args[2].asString()->chars();

        InitWindow(width, height, title);

        return Value::makeNil();
    }

    Value native_CloseWindow(Interpreter *vm, int argc, Value *args)
    {
        CloseWindow();
        return Value::makeNil();
    }

    Value native_WindowShouldClose(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeBool(WindowShouldClose());
    }

    Value native_SetTargetFPS(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("SetTargetFPS expects 1 argument");
            return Value::makeNil();
        }
        SetTargetFPS(args[0].asInt());
        return Value::makeNil();
    }

    Value native_GetFPS(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeInt(GetFPS());
    }

    Value native_GetFrameTime(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeDouble(GetFrameTime());
    }

    // ========================================
    // DRAWING
    // ========================================

    Value native_DrawText(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("DrawText expects 5 arguments");
            return Value::makeNil();
        }

        if (!args[0].isString())
        {
            Error("DrawText expects argument 0 to be string");
            return Value::makeNil();
        }

        if (!args[4].isNativeStructInstance())
        {
            Error("DrawText expects argument 4 to be Color");
            return Value::makeNil();
        }

        auto *colorInst = args[4].asNativeStructInstance();
        Color *tint = (Color *)colorInst->data;

        const char *text = args[0].asString()->chars();
        int x = args[1].asInt();
        int y = args[2].asInt();
        int fontSize = args[3].asInt();

        DrawText(text, x, y, fontSize, *tint);
        return Value::makeNil();
    }

    Value native_BeginDrawing(Interpreter *vm, int argc, Value *args)
    {
        BeginDrawing();
        return Value::makeNil();
    }

    Value native_EndDrawing(Interpreter *vm, int argc, Value *args)
    {
        EndDrawing();
        return Value::makeNil();
    }

    Value native_ClearBackground(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ClearBackground expects 1 argument");
            return Value::makeNil();
        }
        if (!args[0].isNativeStructInstance())
        {
            Error("ClearBackground expects Color");
            return Value::makeNil();
        }

        auto *inst = args[0].asNativeStructInstance();
        Color *c = (Color *)inst->data;

        ClearBackground(*c);
        return Value::makeNil();
    }

    Value native_DrawPixel(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("DrawPixel expects 3 arguments");
            return Value::makeNil();
        }

        if (!args[2].isNativeStructInstance())
        {
            Error("DrawPixel expects Color");
            return Value::makeNil();
        }

        int x = args[0].asInt();
        int y = args[1].asInt();

        auto *inst = args[2].asNativeStructInstance();
        Color *color = (Color *)inst->data;

        DrawPixel(x, y, *color);
        return Value::makeNil();
    }

    Value native_DrawLine(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("DrawLine expects 5 arguments");
            return Value::makeNil();
        }

        if (!args[4].isNativeStructInstance())
        {
            Error("DrawLine expects Color");
            return Value::makeNil();
        }

        int x1 = args[0].asInt();
        int y1 = args[1].asInt();
        int x2 = args[2].asInt();
        int y2 = args[3].asInt();

        auto *inst = args[4].asNativeStructInstance();
        Color *color = (Color *)inst->data;

        DrawLine(x1, y1, x2, y2, *color);
        return Value::makeNil();
    }

    Value native_DrawCircle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("DrawCircle expects 4 arguments");
            return Value::makeNil();
        }
        if (!args[3].isNativeStructInstance())
        {
            Error("DrawCircle expects Color");
            return Value::makeNil();
        }
        int x = args[0].asInt();
        int y = args[1].asInt();
        float radius = args[2].asDouble();

        auto *inst = args[3].asNativeStructInstance();
        Color *color = (Color *)inst->data;

        DrawCircle(x, y, radius, *color);
        return Value::makeNil();
    }

    Value native_DrawCircleV(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("DrawCircleV expects 3 arguments");
            return Value::makeNil();
        }
        if (!args[2].isNativeStructInstance())
        {
            Error("DrawCircleV expects Color");
            return Value::makeNil();
        }
        auto *posInst = args[0].asNativeStructInstance();
        Vector2 *pos = (Vector2 *)posInst->data;

        float radius = args[1].asDouble();

        auto *colorInst = args[2].asNativeStructInstance();
        Color *color = (Color *)colorInst->data;

        DrawCircleV(*pos, radius, *color);
        return Value::makeNil();
    }

    Value native_DrawRectangle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("DrawRectangle expects 5 arguments");
            return Value::makeNil();
        }
        if (!args[4].isNativeStructInstance())
        {
            Error("DrawRectangle expects Color");
            return Value::makeNil();
        }
        int x = args[0].asInt();
        int y = args[1].asInt();
        int width = args[2].asInt();
        int height = args[3].asInt();

        auto *inst = args[4].asNativeStructInstance();
        Color *color = (Color *)inst->data;

        DrawRectangle(x, y, width, height, *color);
        return Value::makeNil();
    }

    Value native_DrawRectangleRec(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("DrawRectangleRec expects 2 arguments");
            return Value::makeNil();
        }
        if (!args[0].isNativeStructInstance())
        {
            Error("DrawRectangleRec expects Rectangle");
            return Value::makeNil();
        }
        if (!args[1].isNativeStructInstance())
        {
            Error("DrawRectangleRec expects Color");
            return Value::makeNil();
        }
        auto *rectInst = args[0].asNativeStructInstance();
        Rectangle *rect = (Rectangle *)rectInst->data;

        auto *colorInst = args[1].asNativeStructInstance();
        Color *color = (Color *)colorInst->data;

        DrawRectangleRec(*rect, *color);
        return Value::makeNil();
    }

    // ========================================
    // TEXTURE
    // ========================================
    Value native_LoadTexture(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("LoadTexture expects 1 argument");
            return Value::makeNil();
        }
        if (!args[0].isString())
        {
            Error("LoadTexture expects string");
            return Value::makeNil();
        }
        const char *filename = args[0].asString()->chars();

        Texture2D tex = LoadTexture(filename);
        Texture2D *texPtr = new Texture2D(tex);

        return Value::makePointer(texPtr);
    }

    Value native_UnloadTexture(Interpreter *vm, int argc, Value *args)
    {
        Texture2D *tex = (Texture2D *)args[0].asPointer();

        UnloadTexture(*tex);
        delete tex;

        return Value::makeNil();
    }

    Value native_DrawTexture(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("DrawTexture expects 4 arguments");
            return Value::makeNil();
        }
        if (!args[0].isPointer())
        {
            Error("DrawTexture expects Texture2D");
            return Value::makeNil();
        }
        if (!args[3].isNativeStructInstance())
        {
            Error("DrawTexture expects Color");
            return Value::makeNil();
        }
        Texture2D *tex = (Texture2D *)args[0].asPointer();
        int x = args[1].asInt();
        int y = args[2].asInt();

        auto *colorInst = args[3].asNativeStructInstance();
        Color *tint = (Color *)colorInst->data;

        DrawTexture(*tex, x, y, *tint);
        return Value::makeNil();
    }

    Value native_DrawTextureV(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("DrawTextureV expects 3 arguments");
            return Value::makeNil();
        }
        if (!args[0].isPointer())
        {
            Error("DrawTextureV expects Texture2D");
            return Value::makeNil();
        }
        if (!args[1].isNativeStructInstance())
        {
            Error("DrawTextureV expects Vector2");
            return Value::makeNil();
        }
        if (!args[2].isNativeStructInstance())
        {
            Error("DrawTextureV expects Color");
            return Value::makeNil();
        }

        Texture2D *tex = (Texture2D *)args[0].asPointer();

        auto *posInst = args[1].asNativeStructInstance();
        Vector2 *pos = (Vector2 *)posInst->data;

        auto *colorInst = args[2].asNativeStructInstance();
        Color *tint = (Color *)colorInst->data;

        DrawTextureV(*tex, *pos, *tint);
        return Value::makeNil();
    }

    // void texture_dtor(Interpreter *vm, void *buffer)
    // {
    //     Texture *v = (Texture *)buffer;
    //     if (v->id == 0)
    //         return;
    //     UnloadTexture(*v);
    //     v->id = 0;
    //     v->width = 0;
    //     v->height = 0;
    //     v->mipmaps = 0;
    //     v->format = 0;
    // }

    // Value native_LoadTexture(Interpreter *vm, int argc, Value *args)
    // {
    //     if (argc != 1)
    //     {
    //         Error("LoadTexture expects 1 argument");
    //         return Value::makeNil();
    //     }
    //     if (!args[0].isString())
    //     {
    //         Error("LoadTexture expects string");
    //         return Value::makeNil();
    //     }
    //     const char *filename = args[0].asString()->chars();

    //     Value v = vm->createNativeStruct(vm->getLastRegisteredInstanceId(), 0, nullptr);
    //     auto *inst = v.asNativeStructInstance();

    //     Texture2D data = LoadTexture(filename);

    //     *(Texture2D *)inst->data = data;

    //     return v;
    // }

    // Value native_UnloadTexture(Interpreter *vm, int argc, Value *args)
    // {
    //     auto *texInst = args[0].asNativeStructInstance();
    //     Texture2D *tex = (Texture2D *)texInst->data;

    //     if (tex->id == 0)
    //         return Value::makeNil();

    //     UnloadTexture(*tex);
    //     tex->id = 0;

    //     return Value::makeNil();
    // }

    // Value native_DrawTexture(Interpreter *vm, int argc, Value *args)
    // {
    //     if (argc != 4)
    //     {
    //         Error("DrawTexture expects 4 arguments");
    //         return Value::makeNil();
    //     }
    //     if (!args[0].isNativeStructInstance())
    //     {
    //         Error("DrawTexture expects Texture2D");
    //         return Value::makeNil();
    //     }

    //     if (!args[3].isNativeStructInstance())
    //     {
    //         Error("DrawTexture expects Color");
    //         return Value::makeNil();
    //     }
    //     auto *texInst = args[0].asNativeStructInstance();
    //     Texture2D *tex = (Texture2D *)texInst->data;
    //     int x = args[1].asInt();
    //     int y = args[2].asInt();
    //     auto *colorInst = args[3].asNativeStructInstance();
    //     Color *tint = (Color *)colorInst->data;
    //     DrawTexture(*tex, x, y, *tint);
    //     return Value::makeNil();
    // }

    // Value native_DrawTextureEx(Interpreter *vm, int argc, Value *args)
    // {

    //     auto *texInst = args[0].asNativeStructInstance();
    //     Texture2D *tex = (Texture2D *)texInst->data;

    //     auto *posInst = args[1].asNativeStructInstance();
    //     Vector2 *pos = (Vector2 *)posInst->data;

    //     float rotation = args[2].asDouble();
    //     float scale = args[3].asDouble();

    //     auto *colorInst = args[4].asNativeStructInstance();
    //     Color *tint = (Color *)colorInst->data;

    //     DrawTextureEx(*tex, *pos, rotation, scale, *tint);
    //     return Value::makeNil();
    // }

    // Value native_DrawTextureV(Interpreter *vm, int argc, Value *args)
    // {

    //     auto *texInst = args[0].asNativeStructInstance();
    //     Texture2D *tex = (Texture2D *)texInst->data;

    //     auto *posInst = args[1].asNativeStructInstance();
    //     Vector2 *pos = (Vector2 *)posInst->data;

    //     auto *colorInst = args[2].asNativeStructInstance();
    //     Color *tint = (Color *)colorInst->data;

    //     DrawTextureV(*tex, *pos, *tint);
    //     return Value::makeNil();
    // }

    // void registerTexture(Interpreter &vm)
    // {
    //     auto *texture = vm.registerNativeStruct(
    //         "Texture",
    //         sizeof(Texture),
    //         nullptr,
    //         texture_dtor,
    //         "raylib");

    //     vm.addStructField(texture, "id", offsetof(Texture, id), FieldType::INT);
    //     vm.addStructField(texture, "width", offsetof(Texture, width), FieldType::INT);
    //     vm.addStructField(texture, "height", offsetof(Texture, height), FieldType::INT);
    //     vm.addStructField(texture, "mipmaps", offsetof(Texture, mipmaps), FieldType::INT);
    //     vm.addStructField(texture, "format", offsetof(Texture, format), FieldType::INT);

    //     // Textures
    //     vm.registerNative("LoadTexture", native_LoadTexture, 1, "raylib");
    //     vm.registerNative("UnloadTexture", native_UnloadTexture, 1, "raylib");
    //     vm.registerNative("DrawTexture", native_DrawTexture, 4, "raylib");
    //     // vm.registerNative("DrawTextureV", native_DrawTextureV, 3, "raylib");
    //     // vm.registerNative("DrawTextureEx", native_DrawTextureEx, 5, "raylib");
    // }

    //
    const char *formatBytes(size_t bytes)
    {
        static char buffer[64];

        if (bytes < 1024)
        {
            snprintf(buffer, sizeof(buffer), "%zu B", bytes);
        }
        else if (bytes < 1024 * 1024)
        {
            snprintf(buffer, sizeof(buffer), "%.2f KB", bytes / 1024.0f);
        }
        else if (bytes < 1024 * 1024 * 1024)
        {
            snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / (1024.0f * 1024.0f));
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%.2f GB", bytes / (1024.0f * 1024.0f * 1024.0f));
        }

        return buffer;
    }
    Value native_DrawFps(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("DrawFps expects 2 arguments");
            return Value::makeNil();
        }
        int x = args[0].asInt();
        int y = args[1].asInt();

        DrawFPS(x, y);
        // statistics

        int totalMemory = vm->getTotalBytes();
        int totalStrings = vm->getTotalStrings();
        int stringsBytes = vm->getStringsBytes();

        DrawText(TextFormat("Strings %d (%s)", totalStrings, formatBytes(stringsBytes)), x, y + 20, 20, WHITE);
        DrawText(TextFormat("Memory: %s", formatBytes(totalMemory)), x, y + 40, 20, WHITE);
        DrawText(TextFormat("Arrays :%d Maps: %d", vm->getTotalArrays(), vm->getTotalMaps()), x, y + 60, 20, WHITE);
        DrawText(TextFormat("Structs: %d Classes %d", vm->getTotalStructs(), vm->getTotalClasses()), x, y + 80, 20, WHITE);
        DrawText(TextFormat("Natives Structs %d Classes %d", vm->getTotalNativeStructs(), vm->getTotalNativeClasses()), x, y + 100, 20, WHITE);

        return Value::makeNil();
    }

    Value native_IsMouseButtonDown(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsMouseButtonDown expects 1 argument");
            return Value::makeNil();
        }
        if (!args[0].isInt())
        {
            Error("IsMouseButtonDown expects int");
            return Value::makeNil();
        }

        return Value::makeBool(IsMouseButtonDown(args[0].asInt()));
    }

    Value native_IsMouseButtonPressed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsMouseButtonPressed expects 1 argument");
            return Value::makeNil();
        }
        if (!args[0].isInt())
        {
            Error("IsMouseButtonPressed expects int");
            return Value::makeNil();
        }

        return Value::makeBool(IsMouseButtonPressed(args[0].asInt()));
    }

    Value native_IsMouseButtonReleased(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsMouseButtonReleased expects 1 argument");
            return Value::makeNil();
        }
        if (!args[0].isInt())
        {
            Error("IsMouseButtonReleased expects int");
            return Value::makeNil();
        }

        return Value::makeBool(IsMouseButtonReleased(args[0].asInt()));
    }

    Value native_IsMouseButtonUp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsMouseButtonUp expects 1 argument");
            return Value::makeNil();
        }
        if (!args[0].isInt())
        {
            Error("IsMouseButtonUp expects int");
            return Value::makeNil();
        }

        return Value::makeBool(IsMouseButtonUp(args[0].asInt()));
    }

    Value native_GetMouseX(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeInt(GetMouseX());
    }

    Value native_GetMouseY(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeInt(GetMouseY());
    }

    // ========================================
    // INPUT - KEYBOARD
    // ========================================

    Value native_IsKeyPressed(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsKeyPressed expects 1 argument");
            return Value::makeNil();
        }
        return Value::makeBool(IsKeyPressed(args[0].asInt()));
    }

    Value native_IsKeyDown(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsKeyDown expects 1 argument");
            return Value::makeNil();
        }
        return Value::makeBool(IsKeyDown(args[0].asInt()));
    }

    Value native_IsKeyReleased(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsKeyReleased expects 1 argument");
            return Value::makeNil();
        }
        return Value::makeBool(IsKeyReleased(args[0].asInt()));
    }

    Value native_IsKeyUp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsKeyUp expects 1 argument");
            return Value::makeNil();
        }
        return Value::makeBool(IsKeyUp(args[0].asInt()));
    }

    // ========================================
    // INPUT - MOUSE (fixar)
    // ========================================

    Value native_GetMousePosition(Interpreter *vm, int argc, Value *args)
    {

        Vector2 pos = GetMousePosition();

        Value v = vm->createNativeStruct(0, 0, nullptr);
        auto *inst = v.asNativeStructInstance();

        *(Vector2 *)inst->data = pos;

        // auto *def = inst->def;

        // NativeFieldDef fx, fy;
        // def->fields.get(vm->internString("x"), &fx);
        // def->fields.get(vm->internString("y"), &fy);

        // *(float *)((char *)inst->data + fx.offset) = pos.x;
        // *(float *)((char *)inst->data + fy.offset) = pos.y;

        return v;
    }

    // ========================================
    // TEXT
    // ========================================

    Value native_MeasureText(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("MeasureText expects 2 arguments");
            return Value::makeNil();
        }
        const char *text = args[0].asString()->chars();
        int fontSize = args[1].asInt();

        return Value::makeInt(MeasureText(text, fontSize));
    }

    // ========================================
    // TIMING
    // ========================================

    Value native_GetTime(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeDouble(GetTime());
    }

    // ========================================
    // MATH
    // ========================================

    Value native_Clamp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("Clamp expects 3 arguments");
            return Value::makeNil();
        }
        double value = args[0].asDouble();
        double minVal = args[1].asDouble();
        double maxVal = args[2].asDouble();

        return Value::makeDouble((value < minVal) ? minVal : (value > maxVal) ? maxVal
                                                                              : value);
    }

    Value native_Lerp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("Lerp expects 3 arguments");
            return Value::makeNil();
        }
        double start = args[0].asDouble();
        double end = args[1].asDouble();
        double amount = args[2].asDouble();

        return Value::makeDouble(start + (end - start) * amount);
    }

    // ========================================
    // REGISTRATION
    // ========================================

    void registerAll(Interpreter &vm)
    {
        // Native Structs
        registerVector2(vm);
        registerVector3(vm);
        registerRectangle(vm);
        registerColor(vm);
        registerCamera2D(vm);
       // registerTexture(vm);

        // Window
        vm.registerNative("InitWindow", native_InitWindow, 3, "raylib");
        vm.registerNative("CloseWindow", native_CloseWindow, 0, "raylib");
        vm.registerNative("WindowShouldClose", native_WindowShouldClose, 0, "raylib");
        vm.registerNative("SetTargetFPS", native_SetTargetFPS, 1, "raylib");
        vm.registerNative("GetFPS", native_GetFPS, 0, "raylib");
        vm.registerNative("GetFrameTime", native_GetFrameTime, 0, "raylib");

        // Drawing
        vm.registerNative("BeginDrawing", native_BeginDrawing, 0, "raylib");
        vm.registerNative("EndDrawing", native_EndDrawing, 0, "raylib");
        vm.registerNative("ClearBackground", native_ClearBackground, 1, "raylib");

        vm.registerNative("DrawTexture", native_DrawTexture, 4);
        vm.registerNative("LoadTexture", native_LoadTexture, 1);
        vm.registerNative("UnloadTexture", native_UnloadTexture, 1);

        // vm.registerNative("BeginMode2D",  native_BeginMode2D, 0,"raylib");
        // vm.registerNative("EndMode2D",  native_EndMode2D, 0,"raylib");
        // vm.registerNative("BeginMode3D",  native_BeginMode3D, 0,"raylib");
        // vm.registerNative("EndMode3D",  native_EndMode3D, 0,"raylib");
        // vm.registerNative("BeginTextureMode",  native_BeginTextureMode, 0,"raylib");

        vm.registerNative("DrawPixel", native_DrawPixel, 3, "raylib");
        vm.registerNative("DrawLine", native_DrawLine, 5, "raylib");
        vm.registerNative("DrawCircle", native_DrawCircle, 4, "raylib");
        vm.registerNative("DrawCircleV", native_DrawCircleV, 3, "raylib");
        vm.registerNative("DrawRectangle", native_DrawRectangle, 5, "raylib");
        vm.registerNative("DrawRectangleRec", native_DrawRectangleRec, 2, "raylib");

        // Text
        vm.registerNative("DrawText", native_DrawText, 5, "raylib");
        vm.registerNative("DrawFps", native_DrawFps, 2, "raylib");
        vm.registerNative("MeasureText", native_MeasureText, 2, "raylib");

        // Input - Keyboard
        vm.registerNative("IsKeyPressed", native_IsKeyPressed, 1, "raylib");
        vm.registerNative("IsKeyDown", native_IsKeyDown, 1, "raylib");
        vm.registerNative("IsKeyReleased", native_IsKeyReleased, 1, "raylib");
        vm.registerNative("IsKeyUp", native_IsKeyUp, 1, "raylib");

        // Input - Mouse
        vm.registerNative("IsMouseButtonPressed", native_IsMouseButtonPressed, 1, "raylib");
        vm.registerNative("IsMouseButtonDown", native_IsMouseButtonDown, 1, "raylib");
        vm.registerNative("IsMouseButtonReleased", native_IsMouseButtonReleased, 1, "raylib");
        vm.registerNative("GetMouseX", native_GetMouseX, 0, "raylib");
        vm.registerNative("GetMouseY", native_GetMouseY, 0, "raylib");
        vm.registerNative("GetMousePosition", native_GetMousePosition, 0, "raylib");

        // Timing
        vm.registerNative("GetTime", native_GetTime, 0, "raylib");
    }

}