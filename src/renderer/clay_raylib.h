#pragma once

#include "clay.h"
#include "raylib.h"


void Clay_Raylib_Initialize(int width, int height, const char* title, unsigned int flags);
void Clay_Raylib_Close();

void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands);

Clay_Dimensions Raylib_MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData);
