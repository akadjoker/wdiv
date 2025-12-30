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
        int width = args[0].asNumber();
        int height = args[1].asNumber();
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
        SetTargetFPS(args[0].asNumber());
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
        int x = args[1].asNumber();
        int y = args[2].asNumber();
        int fontSize = args[3].asNumber();

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

        int x = args[0].asNumber();
        int y = args[1].asNumber();

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

        int x1 = args[0].asNumber();
        int y1 = args[1].asNumber();
        int x2 = args[2].asNumber();
        int y2 = args[3].asNumber();

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
        int x = args[0].asNumber();
        int y = args[1].asNumber();
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
        int x = args[0].asNumber();
        int y = args[1].asNumber();
        int width = args[2].asNumber();
        int height = args[3].asNumber();

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
        int x = args[1].asNumber();
        int y = args[2].asNumber();

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

        Value native_MeasureText(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("MeasureText expects 2 arguments");
            return Value::makeNil();
        }
        const char *text = args[0].asString()->chars();
        int fontSize = args[1].asNumber();

        return Value::makeInt(MeasureText(text, fontSize));
    }

    Value native_DrawFps(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("DrawFps expects 2 arguments");
            return Value::makeNil();
        }
        int x = args[0].asNumber();
        int y = args[1].asNumber();

        DrawFPS(x, y);
        return Value::makeNil();
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
        return Value::makeBool(IsKeyPressed(args[0].asNumber()));
    }

    Value native_IsKeyDown(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsKeyDown expects 1 argument");
            return Value::makeNil();
        }
        return Value::makeBool(IsKeyDown(args[0].asNumber()));
    }

    Value native_IsKeyReleased(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsKeyReleased expects 1 argument");
            return Value::makeNil();
        }
        return Value::makeBool(IsKeyReleased(args[0].asNumber()));
    }

    Value native_IsKeyUp(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("IsKeyUp expects 1 argument");
            return Value::makeNil();
        }
        return Value::makeBool(IsKeyUp(args[0].asNumber()));
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

        return Value::makeBool(IsMouseButtonDown(args[0].asNumber()));
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

        return Value::makeBool(IsMouseButtonPressed(args[0].asNumber()));
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

        return Value::makeBool(IsMouseButtonReleased(args[0].asNumber()));
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

        return Value::makeBool(IsMouseButtonUp(args[0].asNumber()));
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
    // TIMING
    // ========================================

    Value native_GetTime(Interpreter *vm, int argc, Value *args)
    {
        return Value::makeDouble(GetTime());
    }
    // ========================================
    // REGISTRATION
    // ========================================

    void registerAll(Interpreter &vm)
    {
        // Native Structs
        registerVector2(vm);
        registerRectangle(vm);
        registerColor(vm);
        registerCamera2D(vm);
      

        // Window
        vm.registerNative("InitWindow", native_InitWindow, 3);
        vm.registerNative("CloseWindow", native_CloseWindow, 0);
        vm.registerNative("WindowShouldClose", native_WindowShouldClose, 0);
        vm.registerNative("SetTargetFPS", native_SetTargetFPS, 1);
        vm.registerNative("GetFPS", native_GetFPS, 0);
        vm.registerNative("GetFrameTime", native_GetFrameTime, 0);

        // Drawing
        vm.registerNative("BeginDrawing", native_BeginDrawing, 0);
        vm.registerNative("EndDrawing", native_EndDrawing, 0);
        vm.registerNative("ClearBackground", native_ClearBackground, 1);

        // vm.registerNative("BeginMode2D",  native_BeginMode2D, 0,"raylib");
        // vm.registerNative("EndMode2D",  native_EndMode2D, 0,"raylib");
   

        vm.registerNative("DrawPixel", native_DrawPixel, 3);
        vm.registerNative("DrawLine", native_DrawLine, 5);
        vm.registerNative("DrawCircle", native_DrawCircle, 4);
        vm.registerNative("DrawCircleV", native_DrawCircleV, 3);
        vm.registerNative("DrawRectangle", native_DrawRectangle, 5);
        vm.registerNative("DrawRectangleRec", native_DrawRectangleRec, 2);

        // Text
        vm.registerNative("DrawText", native_DrawText, 5);
        vm.registerNative("DrawFps", native_DrawFps, 2);
        vm.registerNative("MeasureText", native_MeasureText, 2);

        // Input - Keyboard
        vm.registerNative("IsKeyPressed", native_IsKeyPressed, 1);
        vm.registerNative("IsKeyDown", native_IsKeyDown, 1);
        vm.registerNative("IsKeyReleased", native_IsKeyReleased, 1);
        vm.registerNative("IsKeyUp", native_IsKeyUp, 1);

        // Input - Mouse
        vm.registerNative("IsMouseButtonPressed", native_IsMouseButtonPressed, 1);
        vm.registerNative("IsMouseButtonDown", native_IsMouseButtonDown, 1);
        vm.registerNative("IsMouseButtonReleased", native_IsMouseButtonReleased, 1);
        vm.registerNative("GetMouseX", native_GetMouseX, 0);
        vm.registerNative("GetMouseY", native_GetMouseY, 0);
        vm.registerNative("GetMousePosition", native_GetMousePosition, 0);

        // Timing
        vm.registerNative("GetTime", native_GetTime, 0);
    }

}