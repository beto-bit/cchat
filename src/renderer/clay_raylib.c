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

[[gnu::always_inline]]
static inline Rectangle clay_bbox_to_raylib_rectangle(Clay_BoundingBox box) {
    return (Rectangle) {
        .x = box.x,
        .y = box.y,
        .height = box.height,
        .width = box.width
    };
}

[[gnu::always_inline]]
static inline bool equal(float a, float b) {
    constexpr float EPSILON = 1e-9f;
    return fabsf(a - b) < EPSILON;
}


Clay_Dimensions Raylib_MeasureText(
    Clay_StringSlice text,
    Clay_TextElementConfig *cfg,
    [[maybe_unused]] void *userData
) {
    // Measure string size for Font
    Clay_Dimensions textSize = { 0 };

    float maxTextWidth = 0.0f;
    float lineTextWidth = 0.0f;
    int maxLineCharCount = 0;
    int lineCharCount = 0;

    float textHeight = cfg->fontSize;

    // Use Raylib default font
    Font fontToUse = GetFontDefault();

    float scaleFactor = cfg->fontSize / (float) fontToUse.baseSize;

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

    textSize.width = maxTextWidth * scaleFactor + (float) (lineCharCount * cfg->letterSpacing);
    textSize.height = textHeight;

    return textSize;
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

    DrawTextEx(
        GetFontDefault(),
        temp_render_buffer,
        (Vector2) { boundingBox.x, boundingBox.y },
        (float) textData->fontSize,
        (float) textData->letterSpacing,
        clay_color_to_raylib_color(textData->textColor)
    );
}

static void clay_render_rectangle(Clay_BoundingBox boundingbox, Clay_RectangleRenderData* rectangleData) {
    if (rectangleData->cornerRadius.topLeft > 0) {
        float radius = (rectangleData->cornerRadius.topLeft * 2) / fminf(boundingbox.width, boundingbox.height);
        DrawRectangleRounded(
            clay_bbox_to_raylib_rectangle(boundingbox),
            radius,
            0,
            clay_color_to_raylib_color(rectangleData->backgroundColor)
        );
    } else {
        DrawRectangle(
            (int) boundingbox.x,
            (int) boundingbox.y,
            (int) boundingbox.width,
            (int) boundingbox.height,
            clay_color_to_raylib_color(rectangleData->backgroundColor)
        );
    }
}

static void clay_render_image(Clay_BoundingBox boundingBox, Clay_ImageRenderData* imageData) {
    Texture2D imageTexture = *(Texture2D*) imageData->imageData;
    Clay_Color tintColor = imageData->backgroundColor;

    if (
        equal(tintColor.r, 0)
        && equal(tintColor.g, 0)
        && equal(tintColor.b, 0)
        && equal(tintColor.a, 0)
    ) {
        tintColor = (Clay_Color) { 255, 255, 255, 255 };
    }

    DrawTexturePro(
        imageTexture,
        (Rectangle) { 0, 0, (float) imageTexture.width, (float) imageTexture.height },
        (Rectangle) { boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height },
        (Vector2) {},
        0,
        clay_color_to_raylib_color(tintColor)
    );
}

static void clay_render_border(Clay_BoundingBox boundingBox, Clay_BorderRenderData* borderData) {
    // Alias
    Clay_BorderRenderData* cfg = borderData;

    // Left border
    if (cfg->width.left > 0) {
        DrawRectangle(
            (int)roundf(boundingBox.x),
            (int)roundf(boundingBox.y + cfg->cornerRadius.topLeft),
            (int)cfg->width.left,
            (int)roundf(boundingBox.height - cfg->cornerRadius.topLeft - cfg->cornerRadius.bottomLeft),
            clay_color_to_raylib_color(cfg->color)
        );
    }

    // Right border
    if (cfg->width.right > 0) {
        DrawRectangle(
            (int)roundf(boundingBox.x + boundingBox.width - cfg->width.right),
            (int)roundf(boundingBox.y + cfg->cornerRadius.topRight),
            (int)cfg->width.right,
            (int)roundf(boundingBox.height - cfg->cornerRadius.topRight - cfg->cornerRadius.bottomRight),
            clay_color_to_raylib_color(cfg->color)
        );
    }

    // Top border
    if (cfg->width.top > 0) {
        DrawRectangle(
            (int)roundf(boundingBox.x + cfg->cornerRadius.topLeft),
            (int)roundf(boundingBox.y),
            (int)roundf(boundingBox.width - cfg->cornerRadius.topLeft - cfg->cornerRadius.topRight),
            (int)cfg->width.top,
            clay_color_to_raylib_color(cfg->color)
        );
    }

    // Bottom border
    if (cfg->width.bottom > 0) {
        DrawRectangle(
            (int)roundf(boundingBox.x + cfg->cornerRadius.bottomLeft),
            (int)roundf(boundingBox.y + boundingBox.height - cfg->width.bottom),
            (int)roundf(boundingBox.width - cfg->cornerRadius.bottomLeft - cfg->cornerRadius.bottomRight),
            (int)cfg->width.bottom,
            clay_color_to_raylib_color(cfg->color)
        );
    }

    if (cfg->cornerRadius.topLeft > 0) {
        DrawRing(
            (Vector2) {
                .x = roundf(boundingBox.x + cfg->cornerRadius.topLeft),
                .y = roundf(boundingBox.y + cfg->cornerRadius.topLeft)
            },
            roundf(cfg->cornerRadius.topLeft - cfg->width.top),
            cfg->cornerRadius.topLeft,
            180,
            270,
            10,
            clay_color_to_raylib_color(cfg->color)
        );
    }
    if (cfg->cornerRadius.topRight > 0) {
        DrawRing(
            (Vector2) {
                .x = roundf(boundingBox.x + boundingBox.width - cfg->cornerRadius.topRight),
                .y = roundf(boundingBox.y + cfg->cornerRadius.topRight)
            },
            roundf(cfg->cornerRadius.topRight - cfg->width.top),
            cfg->cornerRadius.topRight,
            270,
            360,
            10,
            clay_color_to_raylib_color(cfg->color)
        );
    }
    if (cfg->cornerRadius.bottomLeft > 0) {
        DrawRing(
            (Vector2) {
                .x = roundf(boundingBox.x + cfg->cornerRadius.bottomLeft),
                .y = roundf(boundingBox.y + boundingBox.height - cfg->cornerRadius.bottomLeft)
            },
            roundf(cfg->cornerRadius.bottomLeft - cfg->width.bottom),
            cfg->cornerRadius.bottomLeft,
            90,
            180,
            10,
            clay_color_to_raylib_color(cfg->color)
        );
    }
    if (cfg->cornerRadius.bottomRight > 0) {
        DrawRing(
            (Vector2) {
                .x = roundf(boundingBox.x + boundingBox.width - cfg->cornerRadius.bottomRight),
                .y = roundf(boundingBox.y + boundingBox.height - cfg->cornerRadius.bottomRight)
            },
            roundf(cfg->cornerRadius.bottomRight - cfg->width.bottom),
            cfg->cornerRadius.bottomRight,
            0.1f,
            90,
            10,
            clay_color_to_raylib_color(cfg->color)
        );
    }
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

            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                clay_render_rectangle(boundingBox, &renderCommand->renderData.rectangle);
                break;

            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                clay_render_image(boundingBox, &renderCommand->renderData.image);
                break;

            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                BeginScissorMode(
                    (int) roundf(boundingBox.x),
                    (int) roundf(boundingBox.y),
                    (int) roundf(boundingBox.width),
                    (int) roundf(boundingBox.height)
                );
                break;
            }

            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                EndScissorMode();
                break;

            case CLAY_RENDER_COMMAND_TYPE_BORDER:
                clay_render_border(boundingBox, &renderCommand->renderData.border);
                break;

            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            }

            case CLAY_RENDER_COMMAND_TYPE_NONE:
            default:
                fputs("Error: Unhandled Render Command", stderr);
                exit(1);
        }
    }
}
