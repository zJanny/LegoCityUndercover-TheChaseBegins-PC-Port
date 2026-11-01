#pragma once
#include "render/window.hpp"

namespace chase::render {

void requestFrameKeep();
bool frameKeepRequested();
bool frameKept();
void keepFrameIfRequested(int width, int height);
void drawKeptFrame(int width, int height);
void releaseKeptFrame();

}
