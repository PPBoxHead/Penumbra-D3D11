#ifndef VERTEX_FORMAT_H
#define VERTEX_FORMAT_H

#include <DirectXMath.h>

// This is a self implementation of the next file from the Adria-DX11 Engine
// https://github.com/mateeeeeee/Adria-DX11/blob/master/Adria/Graphics/GfxVertexFormat.h

struct SimpleVertexData {
	DirectX::XMFLOAT3 vPosition;
};

struct ColoredVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT4 vColor;
};

struct ColoredNormalVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT4 vColor;
	DirectX::XMFLOAT3 vNormal;
};

struct ColoredTexturedVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT4 vColor;
	DirectX::XMFLOAT2 vTexCoordinate;
};

struct TexturedVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT2 vTexCoordinate;
};

struct TexturedNormalVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT2 vTexCoordinate;
	DirectX::XMFLOAT3 vNormal;
};

struct NormalVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT3 vNormal;
};

struct CompleteVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT4 vColor;
	DirectX::XMFLOAT3 vNormal;
	DirectX::XMFLOAT2 vTexCoordinate;
	DirectX::XMFLOAT3 vTangent;
	DirectX::XMFLOAT3 vBitangent;
};


#endif // !VERTEX_FORMAT_H

