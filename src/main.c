#include "clay.h"
#include "renderer/clay_raylib.h"

#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


constexpr int width = 600;
constexpr int height = 400;
const char* title = "Clay Demo";

constexpr Clay_Color CLAY_BLACK = { 0, 0, 0, 255 };
constexpr Clay_Color CLAY_RED = { 255, 0, 0, 255 };


typedef uint64_t u64;


void HandleClayErrors(Clay_ErrorData errorData) {
    fputs(errorData.errorText.chars, stderr);
}

int main(void) {
    Clay_Raylib_Initialize(width, height, title, FLAG_WINDOW_RESIZABLE);

    // Clay Memory Initialization
    const u64 clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayArena = Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory, malloc(clayRequiredMemory));

    [[maybe_unused]]
    Clay_Context *context = Clay_Initialize(
        clayArena,
        (Clay_Dimensions) {
            .width = (float) GetScreenWidth(),
            .height = (float) GetScreenHeight()
        },
        (Clay_ErrorHandler) { .errorHandlerFunction = HandleClayErrors }
    );

    Clay_SetMeasureTextFunction(Raylib_MeasureText, nullptr);

    // Main loop
    while (!WindowShouldClose()) {
        // Build the Clay layout
        Clay_BeginLayout();

        CLAY(
            CLAY_ID("root"),
            { .layout = {
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(20),
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }}
        ) {
            CLAY_TEXT(CLAY_STRING("CChat"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = CLAY_BLACK }));
            CLAY_TEXT(CLAY_STRING("Sample text"), CLAY_TEXT_CONFIG({ .fontSize = 64, .textColor = CLAY_RED }));
        }
        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        // Actual render
        BeginDrawing();
            ClearBackground(WHITE);
            Clay_Raylib_Render(renderCommands);
        EndDrawing();
    }

    return 0;
}
