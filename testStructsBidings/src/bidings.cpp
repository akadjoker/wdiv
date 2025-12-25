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

    void registerRectangle(Interpreter &vm)
    {
        auto *rect = vm.registerNativeStruct(
            "Rectangle",
            sizeof(Rectangle),
            nullptr,
            nullptr);

        vm.addStructField(rect, "x", offsetof(Rectangle, x), FieldType::FLOAT);
        vm.addStructField(rect, "y", offsetof(Rectangle, y), FieldType::FLOAT);
        vm.addStructField(rect, "width", offsetof(Rectangle, width), FieldType::FLOAT);
        vm.addStructField(rect, "height", offsetof(Rectangle, height), FieldType::FLOAT);
    }

    // ========================================
    // COLOR
    // ========================================

    void registerColor(Interpreter &vm)
    {
        auto *color = vm.registerNativeStruct(
            "Color",
            sizeof(Color),
            nullptr,
            nullptr);

        vm.addStructField(color, "r", offsetof(Color, r), FieldType::INT);
        vm.addStructField(color, "g", offsetof(Color, g), FieldType::INT);
        vm.addStructField(color, "b", offsetof(Color, b), FieldType::INT);
        vm.addStructField(color, "a", offsetof(Color, a), FieldType::INT);
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
            nullptr);

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

    // Value native_ClearBackground(Interpreter *vm, int argc, Value *args)
    // {
    //     if (!args[0].isNativeStructInst())
    //     {
    //         vm->runtimeError("ClearBackground expects Color");
    //         return Value::makeNil();
    //     }

    //     auto *inst = vm->getNativeStructInstance(args[0].asNativeStructInstId());
    //     Color *color = (Color *)inst->data;

    //     ClearBackground(*color);
    //     return Value::makeNil();
    // }

    // Value native_DrawPixel(Interpreter *vm, int argc, Value *args)
    // {
    //     int x = args[0].asInt();
    //     int y = args[1].asInt();

    //     auto *inst = vm->getNativeStructInstance(args[2].asNativeStructInstId());
    //     Color *color = (Color *)inst->data;

    //     DrawPixel(x, y, *color);
    //     return Value::makeNil();
    // }

    // Value native_DrawLine(Interpreter *vm, int argc, Value *args)
    // {
    //     int x1 = args[0].asInt();
    //     int y1 = args[1].asInt();
    //     int x2 = args[2].asInt();
    //     int y2 = args[3].asInt();

    //     auto *inst = vm->getNativeStructInstance(args[4].asNativeStructInstId());
    //     Color *color = (Color *)inst->data;

    //     DrawLine(x1, y1, x2, y2, *color);
    //     return Value::makeNil();
    // }

    // Value native_DrawCircle(Interpreter *vm, int argc, Value *args)
    // {
    //     int x = args[0].asInt();
    //     int y = args[1].asInt();
    //     float radius = args[2].asDouble();

    //     auto *inst = vm->getNativeStructInstance(args[3].asNativeStructInstId());
    //     Color *color = (Color *)inst->data;

    //     DrawCircle(x, y, radius, *color);
    //     return Value::makeNil();
    // }

    // Value native_DrawCircleV(Interpreter *vm, int argc, Value *args)
    // {
    //     auto *posInst = vm->getNativeStructInstance(args[0].asNativeStructInstId());
    //     Vector2 *pos = (Vector2 *)posInst->data;

    //     float radius = args[1].asDouble();

    //     auto *colorInst = vm->getNativeStructInstance(args[2].asNativeStructInstId());
    //     Color *color = (Color *)colorInst->data;

    //     DrawCircleV(*pos, radius, *color);
    //     return Value::makeNil();
    // }

    // Value native_DrawRectangle(Interpreter *vm, int argc, Value *args)
    // {
    //     int x = args[0].asInt();
    //     int y = args[1].asInt();
    //     int width = args[2].asInt();
    //     int height = args[3].asInt();

    //     auto *inst = vm->getNativeStructInstance(args[4].asNativeStructInstId());
    //     Color *color = (Color *)inst->data;

    //     DrawRectangle(x, y, width, height, *color);
    //     return Value::makeNil();
    // }

    // Value native_DrawRectangleRec(Interpreter *vm, int argc, Value *args)
    // {
    //     auto *rectInst = vm->getNativeStructInstance(args[0].asNativeStructInstId());
    //     Rectangle *rect = (Rectangle *)rectInst->data;

    //     auto *colorInst = vm->getNativeStructInstance(args[1].asNativeStructInstId());
    //     Color *color = (Color *)colorInst->data;

    //     DrawRectangleRec(*rect, *color);
    //     return Value::makeNil();
    // }

    // ========================================
    // TEXTURES
    // ========================================

    Value native_LoadTexture(Interpreter *vm, int argc, Value *args)
    {
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

    // Value native_DrawTexture(Interpreter *vm, int argc, Value *args)
    // {
    //     Texture2D *tex = (Texture2D *)args[0].asPointer();
    //     int x = args[1].asInt();
    //     int y = args[2].asInt();

    //     auto *colorInst = vm->getNativeStructInstance(args[3].asNativeStructInstId());
    //     Color *tint = (Color *)colorInst->data;

    //     DrawTexture(*tex, x, y, *tint);
    //     return Value::makeNil();
    // }

    // Value native_DrawTextureV(Interpreter *vm, int argc, Value *args)
    // {
    //     Texture2D *tex = (Texture2D *)args[0].asPointer();

    //     auto *posInst = vm->getNativeStructInstance(args[1].asNativeStructInstId());
    //     Vector2 *pos = (Vector2 *)posInst->data;

    //     auto *colorInst = vm->getNativeStructInstance(args[2].asNativeStructInstId());
    //     Color *tint = (Color *)colorInst->data;

    //     DrawTextureV(*tex, *pos, *tint);
    //     return Value::makeNil();
    // }

}