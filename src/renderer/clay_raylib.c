#include "clay_raylib.h"

#include "clay.h"
#include "raylib.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static char* temp_render_buffer = nullptr;
static size_t temp_render_buffer_len = 0;


[[gnu::always_inline]]
static inline Color clay_color_to_raylib_color(Clay_Color clayColor) {
    return (Color) {
        .r = (unsigned char) roundf(clayColor.r),
        .g = (unsigned char) roundf(clayColor.g),
        .b = (unsigned char) roundf(clayColor.b),
        .a = (unsigned char) roundf(clayColor.a),
    };
}


Clay_Dimensions Raylib_MeasureText(
    Clay_StringSlice text,
    Clay_TextElementConfig *config,
    [[maybe_unused]] void *userData
) {
    // Measure string size for Font
    Clay_Dimensions textSize = { 0 };

    float maxTextWidth = 0.0f;
    float lineTextWidth = 0.0f;
    int maxLineCharCount = 0;
    int lineCharCount = 0;

    float textHeight = config->fontSize;

    // Use Raylib default font
    Font fontToUse = GetFontDefault();

    float scaleFactor = config->fontSize / (float) fontToUse.baseSize;

    for (int idx = 0; idx < text.length; ++idx, ++lineCharCount) {
        if (text.chars[idx] == '\n') {
            maxTextWidth = fmaxf(maxTextWidth, lineTextWidth);
            maxLineCharCount = CLAY__MAX(maxLineCharCount, lineCharCount);

            lineTextWidth = 0;
            lineCharCount = 0;
            continue;
        }

        int jdx = text.chars[idx] - 32;
        if (fontToUse.glyphs[jdx].advanceX != 0)
            lineTextWidth += (float) fontToUse.glyphs[jdx].advanceX;
        else
            lineTextWidth += (fontToUse.recs[jdx].width + (float) fontToUse.glyphs[jdx].offsetX);
    }

    maxTextWidth = fmaxf(maxTextWidth, lineTextWidth);
    maxLineCharCount = CLAY__MAX(maxLineCharCount, lineCharCount);

    textSize.width = maxTextWidth * scaleFactor + (float) (lineCharCount * config->letterSpacing);
    textSize.height = textHeight;

    return textSize;
}


static void clay_render_text(Clay_BoundingBox boundingBox, Clay_TextRenderData* textData) {
    size_t sstrlen = (size_t) textData->stringContents.length + 1;

    // Grow the temp buffer
    if (sstrlen > temp_render_buffer_len) {
        free(temp_render_buffer);
        temp_render_buffer = (char *) malloc(sstrlen);
        temp_render_buffer_len = sstrlen;
    }

    // Raylib uses C-Strings, need to clone string and append null terminator
    memcpy(temp_render_buffer, textData->stringContents.chars, sstrlen - 1);
    temp_render_buffer[textData->stringContents.length] = '\0';

    DrawText(
        temp_render_buffer,
        (int) boundingBox.x,
        (int) boundingBox.y,
        textData->fontSize,
        clay_color_to_raylib_color(textData->textColor)
    );
}


void Clay_Raylib_Initialize(int width, int height, const char *title, unsigned int flags) {
    SetConfigFlags(flags);
    InitWindow(width, height, title);
}

void Clay_Raylib_Close() {
    free(temp_render_buffer);
    temp_render_buffer_len = 0;

    CloseWindow();
}

void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands) {
    for (int idx = 0; idx < renderCommands.length; ++idx)
    {
        Clay_RenderCommand* renderCommand = Clay_RenderCommandArray_Get(&renderCommands, idx);
        Clay_BoundingBox boundingBox = {
            .x = roundf(renderCommand->boundingBox.x),
            .y = roundf(renderCommand->boundingBox.y),
            .width = roundf(renderCommand->boundingBox.width),
            .height = roundf(renderCommand->boundingBox.height)
        };

        switch (renderCommand->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_TEXT:
                clay_render_text(boundingBox, &renderCommand->renderData.text);
                break;

            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
            }
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            }
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
            }
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            }

            case CLAY_RENDER_COMMAND_TYPE_NONE:
            default:
                fputs("Error: Unhandled Render Command", stderr);
                exit(1);
        }
    }
}
