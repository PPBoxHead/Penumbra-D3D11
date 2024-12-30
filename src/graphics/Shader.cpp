#include "Shader.hpp"

#include <d3dcompiler.h>
#include <stdexcept>

using namespace Microsoft::WRL;

bool Shader::Initialize(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath) {
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;

    // Compile Vertex Shader
    if (!CompileShader(vsPath, "vs_main", "vs_5_0", vsBlob))
        return false;

    // Create Vertex Shader
    if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader)))
        return false;

    // Define Input Layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (FAILED(device->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout)))
        return false;

    // Compile Pixel Shader
    if (!CompileShader(psPath, "ps_main", "ps_5_0", psBlob))
        return false;

    // Create Pixel Shader
    if (FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader)))
        return false;

    // Create a constant buffer (example: size of 16 bytes for a simple structure)
    CreateConstantBuffer(device, 16);

    return true;
}

bool Shader::CompileShader(const std::wstring& filePath, const std::string& entryPoint, const std::string& target, ComPtr<ID3DBlob>& shaderBlob) {
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr, entryPoint.c_str(), target.c_str(),
        D3DCOMPILE_ENABLE_STRICTNESS, 0, &shaderBlob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }

    return true;
}

void Shader::CreateConstantBuffer(ID3D11Device* device, size_t dataSize) {
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = static_cast<UINT>(dataSize);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&bufferDesc, nullptr, &m_constantBuffer)))
    {
        throw std::runtime_error("Failed to create constant buffer");
    }
}

void Shader::SetShader(ID3D11DeviceContext* context) {
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
}

void Shader::UpdateConstantBuffer(ID3D11DeviceContext* context, void* data, size_t dataSize) {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (FAILED(context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        throw std::runtime_error("Failed to map constant buffer");
    }

    memcpy(mappedResource.pData, data, dataSize);
    context->Unmap(m_constantBuffer.Get(), 0);
}
