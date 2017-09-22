#pragma once

#ifndef __DX_COMMON_H__
#define __DX_COMMON_H__

#include <d3d11.h>
#include "DirectXMath.h"

using namespace DirectX;

typedef struct _VERTEX
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
}VERTEX;

struct CBNeverChanges
{
    XMMATRIX mView;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMFLOAT4 vMeshColor;
    XMFLOAT2 fBrightnessParam;
};

#endif //__DX_COMMON_H__