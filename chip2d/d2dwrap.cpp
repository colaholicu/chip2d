#include "stdafx.h"

#include "d2dwrap.h"
#include <wrl\client.h>
#include <wincodec.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <assert.h>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11")

using namespace Microsoft::WRL;

static ID2D1HwndRenderTarget* pRender = nullptr;
static ID2D1Factory1* pFactory = nullptr;
static ID2D1SolidColorBrush* pBrush = nullptr;

static HRESULT createFactory()
{
    pFactory = nullptr;
    return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), NULL, (void**)&pFactory);
}

static HRESULT createRenderTarget(HWND hWindow)
{
    pRender = nullptr;
    if (!pFactory)
        return E_INVALIDARG;

    RECT rc;
    if (!GetClientRect(hWindow, &rc))
        return E_FAIL;

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    return pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWindow, size), &pRender);
}

namespace D2DW
{
    HRESULT createBasicD2D(HWND hWindow)
    {
        HRESULT hr = createFactory();
        if (hr != S_OK)
            return hr;

        hr = createRenderTarget(hWindow);
        if (hr != S_OK)
            return hr;

        hr = pRender->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush) ;
        if (hr != S_OK)
            return hr;

        RECT rc = {0};
        GetClientRect(hWindow, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRender->Resize(size);

        return hr;
    }

    HRESULT destroyBasicD2D()
    {
        if (pBrush)
            pBrush->Release();
        pBrush = nullptr;

        if (pRender)
            pRender->Release();
        pRender = nullptr;

        if (pFactory)
            pFactory->Release();
        pFactory = nullptr;        

        return S_OK;
    }

    HRESULT beginDraw(bool bClear)
    {
        if (!pRender)
            return E_INVALIDARG;

        pRender->BeginDraw();
        pRender->SetTransform(D2D1::Matrix3x2F::Identity());
        if (bClear)
            pRender->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        return S_OK;
    }

    HRESULT endDraw()
    {
        if (!pRender)
            return E_INVALIDARG;

        return pRender->EndDraw();
    }

    HRESULT createBrush(D2D1_COLOR_F color, ID2D1SolidColorBrush*& pBrush)
    {
        return pRender->CreateSolidColorBrush(color, &pBrush);
    }

    HRESULT drawRect(D2D1_COLOR_F color, float x, float y, float cx, float cy)
    {
        HRESULT hr = S_OK;      
        if (!pBrush) 
            return E_POINTER;

        pBrush->SetColor(&color);

        D2D1_RECT_F rect = D2D1::RectF(x, y, cx, cy);
        pRender->FillRectangle(rect, pBrush);

        return hr;
    }
}