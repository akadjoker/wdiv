#include "bidings.hpp"
#include <cmath>

namespace RaylibBindings
{

  


    // ========================================
    // VECTOR2
    // ========================================

    void registerVector2(Interpreter &vm)
    {
        auto *vec2 = vm.registerNativeStruct(
            "Vector2",
            sizeof(Vector2),
            nullptr, // Sem constructor
            nullptr  // Sem destructor
     
        );

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
            nullptr);

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
            nullptr);

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
        // Info("Color(%d, %d, %d, %d)", v->r, v->g, v->b, v->a);
    }

    // void color_dtor(Interpreter *vm, void *buffer)
    // {
    //     Color *v = (Color *)buffer;
    //     //Info("Color(%d, %d, %d, %d)", v->r, v->g, v->b, v->a);
    // }

    void registerColor(Interpreter &vm)
    {
        auto *color = vm.registerNativeStruct(
            "Color",
            sizeof(Color),
            color_ctor,
            nullptr);

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
            nullptr
     );

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
        int width = TO_INT(args[0]);
        int height = TO_INT(args[1]);
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
        SetTargetFPS(TO_INT(args[0]));
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
        int x = TO_INT(args[1]);
        int y = TO_INT(args[2]);
        int fontSize = TO_INT(args[3]);

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

        int x = TO_INT(args[0]);
        int y = TO_INT(args[1]);

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

        int x1 = TO_INT(args[0]);
        int y1 = TO_INT(args[1]);
        int x2 = TO_INT(args[2]);
        int y2 = TO_INT(args[3]);

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
        int x = TO_INT(args[0]);
        int y = TO_INT(args[1]);
        float radius = TO_DOUBLE(args[2]);

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

        float radius = TO_DOUBLE(args[1]);

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
        int x = TO_INT(args[0]);
        int y = TO_INT(args[1]);
        int width = TO_INT(args[2]);
        int height = TO_INT(args[3]);

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
    // TEXTURES
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
        int x = TO_INT(args[1]);
        int y = TO_INT(args[2]);

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

    Value native_DrawFps(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("DrawFps expects 2 arguments");
            return Value::makeNil();
        }
        int x =  TO_INT(args[0]);
        int y = TO_INT(args[1]);

        DrawFPS(x, y);
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

        return Value::makeBool(IsMouseButtonDown(TO_INT(args[0])));
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

        return Value::makeBool(IsMouseButtonPressed(TO_INT(args[0])));
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

        return Value::makeBool(IsMouseButtonReleased(TO_INT(args[0])));
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

        return Value::makeBool(IsMouseButtonUp(TO_INT(args[0])));
    }

    Value native_GetMouseX(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeInt(GetMouseX());
    }

    Value native_GetMouseY(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeInt(GetMouseY());
    }

    Value native_GetMousePosition(Interpreter *vm, int argc, Value *args)
    {
        Vector2 v = GetMousePosition();
        return Value::makeInt(-1);
    }

}