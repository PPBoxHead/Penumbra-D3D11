#include "Shader.hpp"

#include <d3dcompiler.h>
#include <stdexcept>
#include <cstring>


using namespace Microsoft::WRL;

bool Shader::Initialize(ID3D11Device* device, const ShaderDesc& desc) {
    // Compile and create shaders based on the descriptor
    if (!CompileAndCreateShader(device, desc.vertexShaderPath, desc.vertexEntryPoint, desc.vertexTarget, m_vertexShader))
        return false;

    if (!CompileAndCreateShader(device, desc.pixelShaderPath, desc.pixelEntryPoint, desc.pixelTarget, m_pixelShader))
        return false;

    if (!CompileAndCreateShader(device, desc.geometryShaderPath, desc.geometryEntryPoint, desc.geometryTarget, m_geometryShader))
        return false;

    if (!CompileAndCreateShader(device, desc.hullShaderPath, desc.hullEntryPoint, desc.hullTarget, m_hullShader))
        return false;

    if (!CompileAndCreateShader(device, desc.domainShaderPath, desc.domainEntryPoint, desc.domainTarget, m_domainShader))
        return false;

    if (!CompileAndCreateShader(device, desc.computeShaderPath, desc.computeEntryPoint, desc.computeTarget, m_computeShader))
        return false;

    return true;
}

bool Shader::CompileAndCreateShader(
    ID3D11Device* device,
    const std::optional<std::wstring>& filePath,
    const std::string& entryPoint,
    const std::string& target,
    ComPtr<ID3D11DeviceChild> shader)
{
    if (!filePath) // Skip if the shader is not provided
        return true;

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(
        filePath->c_str(),
        nullptr,
        nullptr,
        entryPoint.c_str(),
        target.c_str(),
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return false;
    }

    // Create the appropriate shader type
    if (target.find("vs")) {
        hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11VertexShader**>(shader.GetAddressOf()));
    }
    else if (target.find("ps")) {
        hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11PixelShader**>(shader.GetAddressOf()));
    }
    else if (target.find("gs")) {
        hr = device->CreateGeometryShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11GeometryShader**>(shader.GetAddressOf()));
    }
    else if (target.find("hs")) {
        hr = device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11HullShader**>(shader.GetAddressOf()));
    }
    else if (target.find("ds")) {
        hr = device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11DomainShader**>(shader.GetAddressOf()));
    }
    else if (target.find("cs")) {
        hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11ComputeShader**>(shader.GetAddressOf()));
    }

    return SUCCEEDED(hr);
}

void Shader::SetShaders(ID3D11DeviceContext* context) {
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader(reinterpret_cast<ID3D11VertexShader*>(m_vertexShader.Get()), nullptr, 0);
    context->PSSetShader(reinterpret_cast<ID3D11PixelShader*>(m_pixelShader.Get()), nullptr, 0);
    context->GSSetShader(reinterpret_cast<ID3D11GeometryShader*>(m_geometryShader.Get()), nullptr, 0);
    context->HSSetShader(reinterpret_cast<ID3D11HullShader*>(m_hullShader.Get()), nullptr, 0);
    context->DSSetShader(reinterpret_cast<ID3D11DomainShader*>(m_domainShader.Get()), nullptr, 0);
    context->CSSetShader(reinterpret_cast<ID3D11ComputeShader*>(m_computeShader.Get()), nullptr, 0);
}

bool Shader::CreateConstantBuffer(ID3D11Device* device, const std::string& name, const ConstantBufferDesc& desc) {
    if (m_constantBuffers.count(name) > 0) {
        throw std::runtime_error("Constant buffer with name '" + name + "' already exists");
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(desc.bufferSize);
    bufferDesc.Usage = desc.usage;
    bufferDesc.BindFlags = desc.bindFlags;
    bufferDesc.CPUAccessFlags = desc.cpuAccessFlags;

    ComPtr<ID3D11Buffer> buffer;
    if (FAILED(device->CreateBuffer(&bufferDesc, nullptr, &buffer))) {
        throw std::runtime_error("Failed to create constant buffer '" + name + "'");
    }

    m_constantBuffers[name] = buffer;
    return true;
}

void Shader::UpdateConstantBuffer(ID3D11DeviceContext* context, const std::string& name, void* data, size_t dataSize) {
    auto it = m_constantBuffers.find(name);
    if (it == m_constantBuffers.end()) {
        throw std::runtime_error("Constant buffer '" + name + "' not found");
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (FAILED(context->Map(it->second.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
        throw std::runtime_error("Failed to map constant buffer '" + name + "'");
    }

    memcpy(mappedResource.pData, data, dataSize);
    context->Unmap(it->second.Get(), 0);
}

void Shader::BindConstantBuffer(ID3D11DeviceContext* context, const std::string& name, UINT slot, bool forVertexShader, bool forPixelShader) {
    auto it = m_constantBuffers.find(name);
    if (it == m_constantBuffers.end()) {
        throw std::runtime_error("Constant buffer '" + name + "' not found");
    }

    ID3D11Buffer* buffer = it->second.Get();
    if (forVertexShader) {
        context->VSSetConstantBuffers(slot, 1, &buffer);
    }
    if (forPixelShader) {
        context->PSSetConstantBuffers(slot, 1, &buffer);
    }
}
