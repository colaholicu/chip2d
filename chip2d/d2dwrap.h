#pragma once

#include <d2d1.h>
#include <D2d1_1.h>

namespace D2DW
{   
    HRESULT createBasicD2D(HWND hWindow);
    HRESULT destroyBasicD2D();
    HRESULT drawRect(D2D1_COLOR_F color, float x, float y, float cx, float cy);
    HRESULT beginDraw(bool bClear = false);
    HRESULT endDraw();
}