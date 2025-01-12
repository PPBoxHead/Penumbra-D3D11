#include "Shader.hpp"

#include <d3dcompiler.h>
#include <cstring>

#include "../utils/ConsoleLogger.hpp"


using namespace Microsoft::WRL;


bool Shader::InitializeShader(ID3D11Device* device, const SHADER_DESC& desc, const D3D11_INPUT_ELEMENT_DESC* layout, UINT numElements) {
    ComPtr<ID3DBlob> blob;

    if (desc.vertexShaderPath &&
        CompileShader(device, desc.vertexShaderPath, desc.vertexEntryPoint, desc.vertexTarget, blob) &&
        CreateShader(device, blob, m_vertexShader)) {
        // Create input layout inside the Initialize method
        HRESULT result = device->CreateInputLayout(
            layout,
            numElements,
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            m_inputLayout.GetAddressOf()
        );

        if (FAILED(result)) {
            CONSOLE_LOG_ERROR("Failed to create input layout.");
            return false;
        }
    }

    if (desc.pixelShaderPath &&
        CompileShader(device, desc.pixelShaderPath, desc.pixelEntryPoint, desc.pixelTarget, blob) &&
        CreateShader(device, blob, m_pixelShader)) {
    }

    if (desc.geometryShaderPath &&
        CompileShader(device, desc.geometryShaderPath, desc.geometryEntryPoint, desc.geometryTarget, blob) &&
        CreateShader(device, blob, m_geometryShader)) {
    }

    if (desc.hullShaderPath &&
        CompileShader(device, desc.hullShaderPath, desc.hullEntryPoint, desc.hullTarget, blob) &&
        CreateShader(device, blob, m_hullShader)) {
    }

    if (desc.domainShaderPath &&
        CompileShader(device, desc.domainShaderPath, desc.domainEntryPoint, desc.domainTarget, blob) &&
        CreateShader(device, blob, m_domainShader)) {
    }

    if (desc.computeShaderPath &&
        CompileShader(device, desc.computeShaderPath, desc.computeEntryPoint, desc.computeTarget, blob) &&
        CreateShader(device, blob, m_computeShader)) {
    }

    return true;
}

bool Shader::CompileShader(ID3D11Device* device, const std::optional<std::wstring>& filePath,
    const std::string& entryPoint, const std::string& target,
    ComPtr<ID3DBlob>& blob) {
    if (!filePath) return true;

    UINT compile_flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR |
        D3DCOMPILE_ENABLE_STRICTNESS;
    compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ComPtr<ID3DBlob> errorBlob;
    HRESULT result = D3DCompileFromFile(
        filePath->c_str(),
        nullptr,
        nullptr,
        entryPoint.c_str(),
        target.c_str(),
        compile_flags,
        0,
        &blob,
        &errorBlob
    );

    if (FAILED(result)) {
        if (errorBlob) {
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return false;
    }
    return true;
}

template <typename ShaderType>
bool Shader::CreateShader(ID3D11Device* device, const ComPtr<ID3DBlob>& blob, ComPtr<ShaderType>& shader) {
    HRESULT result;

    // Specialize for each shader type
    if constexpr (std::is_same_v<ShaderType, ID3D11VertexShader>) {
        result = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader.GetAddressOf());
    }
    else if constexpr (std::is_same_v<ShaderType, ID3D11PixelShader>) {
        result = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader.GetAddressOf());
    }
    else if constexpr (std::is_same_v<ShaderType, ID3D11GeometryShader>) {
        result = device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader.GetAddressOf());
    }
    else if constexpr (std::is_same_v<ShaderType, ID3D11HullShader>) {
        result = device->CreateHullShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader.GetAddressOf());
    }
    else if constexpr (std::is_same_v<ShaderType, ID3D11DomainShader>) {
        result = device->CreateDomainShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader.GetAddressOf());
    }
    else if constexpr (std::is_same_v<ShaderType, ID3D11ComputeShader>) {
        result = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader.GetAddressOf());
    }
    else {
        // Unsupported shader type
        return false;
    }

    return SUCCEEDED(result);
}

void Shader::SetShaders(ID3D11DeviceContext* context) {
    context->IASetInputLayout(m_inputLayout.Get());
    if (m_vertexShader != nullptr)
        context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    if (m_pixelShader != nullptr)
        context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    if (m_geometryShader != nullptr)
        context->GSSetShader(m_geometryShader.Get(), nullptr, 0);
    if (m_hullShader != nullptr)
        context->HSSetShader(m_hullShader.Get(), nullptr, 0);
    if (m_domainShader != nullptr)
        context->DSSetShader(m_domainShader.Get(), nullptr, 0);
    if (m_computeShader != nullptr)
        context->CSSetShader(m_computeShader.Get(), nullptr, 0);
}

bool Shader::CreateConstantBuffer(ID3D11Device* device, const std::string& name, const CONSTANT_BUFFER_DESC& desc) {
    if (m_constantBuffers.count(name)) {
        CONSOLE_LOG_CRITICAL_ERROR("Constant buffer already exists: " + name);
    }

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = static_cast<UINT>(desc.bufferSize);
    bufferDesc.Usage = desc.usage;
    bufferDesc.BindFlags = desc.bindFlags;
    bufferDesc.CPUAccessFlags = desc.cpuAccessFlags;

    ComPtr<ID3D11Buffer> buffer;
    if (FAILED(device->CreateBuffer(&bufferDesc, nullptr, &buffer))) {
        CONSOLE_LOG_ERROR("Constant bufer with name: ", name, " couldn't be created");
        return false;
    }

    m_constantBuffers[name] = buffer;
    //ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Created constant buffer: ", m_constantBuffers.find(name)->first);
    return true;
}


void Shader::UpdateConstantBuffer(ID3D11DeviceContext* context, const std::string& name, const void* data, size_t dataSize) {
    auto it = m_constantBuffers.find(name);
    if (it == m_constantBuffers.end()) {
        ConsoleLogger::Print(ConsoleLogger::LogType::C_CRITICAL_ERROR, "Constant buffer not found: " + name);
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (FAILED(context->Map(it->second.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
        ConsoleLogger::Print(ConsoleLogger::LogType::C_CRITICAL_ERROR, "Failed to map constant buffer: " + name);
    }

    std::memcpy(mappedResource.pData, data, dataSize);
    context->Unmap(it->second.Get(), 0);
}

void Shader::BindConstantBuffer(ID3D11DeviceContext* context, const std::string& name, UINT slot, UINT shaderFlags) // Use custom flags, not D3D11_BIND_* constants
{
    auto it = m_constantBuffers.find(name);
    if (it == m_constantBuffers.end()) {
        CONSOLE_LOG_CRITICAL_ERROR("Constant buffer not found: " + name);
    }

    ID3D11Buffer* buffer = it->second.Get();

    // Validate shader flags
    if (shaderFlags == 0) {
        CONSOLE_LOG_CRITICAL_ERROR("Invalid shader flags for constant buffer: " + name);
    }
    // Use custom flags to decide where to bind the constant buffer
    if (shaderFlags & ShaderStage::VertexShader) {
        context->VSSetConstantBuffers(slot, 1, &buffer);
    }
    if (shaderFlags & ShaderStage::PixelShader) {
        context->PSSetConstantBuffers(slot, 1, &buffer);
    }
    if (shaderFlags & ShaderStage::GeometryShader) {
        context->GSSetConstantBuffers(slot, 1, &buffer);
    }
    if (shaderFlags & ShaderStage::HullShader) {
        context->HSSetConstantBuffers(slot, 1, &buffer);
    }
    if (shaderFlags & ShaderStage::DomainShader) {
        context->DSSetConstantBuffers(slot, 1, &buffer);
    }
    if (shaderFlags & ShaderStage::ComputeShader) {
        context->CSSetConstantBuffers(slot, 1, &buffer);
    }
}

bool Shader::CreateShaderResourceView(const std::string& name, ID3D11ShaderResourceView* srv) {
    if (m_shaderResourceViews.count(name)) {
        CONSOLE_LOG_WARNING("Shader resource view already exists: " + name);
        return false;
    }

    m_shaderResourceViews[name] = srv;
    //ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Added shader resource view: " + name);
    return true;
}

bool Shader::CreateSamplerState(const std::string& name, ID3D11SamplerState* sampler) {
    if (m_samplerStates.count(name)) {
        CONSOLE_LOG_WARNING("Sampler state already exists: " + name);
        return false;
    }

    m_samplerStates[name] = sampler;
    //ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Added sampler state: " + name);
    return true;
}

void Shader::BindShaderResourceView(ID3D11DeviceContext* context, const std::string& name, UINT slot, UINT shaderFlags) {
    auto it = m_shaderResourceViews.find(name);
    if (it == m_shaderResourceViews.end()) {
        CONSOLE_LOG_CRITICAL_ERROR("Shader resource view not found: " + name);
        return;
    }

    ID3D11ShaderResourceView* srv = it->second.Get();

    // Validate shader flags
    if (shaderFlags == 0) {
        CONSOLE_LOG_CRITICAL_ERROR("Invalid shader flags for SRV: " + name);
        return;
    }

    // Bind SRV to the appropriate shader stages
    if (shaderFlags & ShaderStage::VertexShader) {
        context->VSSetShaderResources(slot, 1, &srv);
    }
    if (shaderFlags & ShaderStage::PixelShader) {
        context->PSSetShaderResources(slot, 1, &srv);
    }
    if (shaderFlags & ShaderStage::GeometryShader) {
        context->GSSetShaderResources(slot, 1, &srv);
    }
    if (shaderFlags & ShaderStage::HullShader) {
        context->HSSetShaderResources(slot, 1, &srv);
    }
    if (shaderFlags & ShaderStage::DomainShader) {
        context->DSSetShaderResources(slot, 1, &srv);
    }
    if (shaderFlags & ShaderStage::ComputeShader) {
        context->CSSetShaderResources(slot, 1, &srv);
    }
}

void Shader::BindSamplerState(ID3D11DeviceContext* context, const std::string& name, UINT slot, UINT shaderFlags) {
    auto it = m_samplerStates.find(name);
    if (it == m_samplerStates.end()) {
        CONSOLE_LOG_CRITICAL_ERROR("Sampler state not found: " + name);
        return;
    }

    ID3D11SamplerState* sampler = it->second.Get();

    // Validate shader flags
    if (shaderFlags == 0) {
        CONSOLE_LOG_CRITICAL_ERROR("Invalid shader flags for sampler: " + name);
        return;
    }

    // Bind sampler to the appropriate shader stages
    if (shaderFlags & ShaderStage::VertexShader) {
        context->VSSetSamplers(slot, 1, &sampler);
    }
    if (shaderFlags & ShaderStage::PixelShader) {
        context->PSSetSamplers(slot, 1, &sampler);
    }
    if (shaderFlags & ShaderStage::GeometryShader) {
        context->GSSetSamplers(slot, 1, &sampler);
    }
    if (shaderFlags & ShaderStage::HullShader) {
        context->HSSetSamplers(slot, 1, &sampler);
    }
    if (shaderFlags & ShaderStage::DomainShader) {
        context->DSSetSamplers(slot, 1, &sampler);
    }
    if (shaderFlags & ShaderStage::ComputeShader) {
        context->CSSetSamplers(slot, 1, &sampler);
    }
}
