#ifndef SHADER_H
#define SHADER_H

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>


// Descriptor for shader initialization
struct ShaderDesc {
    std::optional<std::wstring> vertexShaderPath;    // Path to Vertex Shader
    std::optional<std::wstring> pixelShaderPath;     // Path to Pixel Shader
    std::optional<std::wstring> geometryShaderPath;  // Path to Geometry Shader
    std::optional<std::wstring> hullShaderPath;      // Path to Hull Shader
    std::optional<std::wstring> domainShaderPath;    // Path to Domain Shader
    std::optional<std::wstring> computeShaderPath;   // Path to Compute Shader

    std::string vertexEntryPoint = "VSMain";         // Default entry points
    std::string pixelEntryPoint = "PSMain";
    std::string geometryEntryPoint = "GSMain";
    std::string hullEntryPoint = "HSMain";
    std::string domainEntryPoint = "DSMain";
    std::string computeEntryPoint = "CSMain";

    std::string vertexTarget = "vs_5_0";             // Default shader targets
    std::string pixelTarget = "ps_5_0";
    std::string geometryTarget = "gs_5_0";
    std::string hullTarget = "hs_5_0";
    std::string domainTarget = "ds_5_0";
    std::string computeTarget = "cs_5_0";
};

// Descriptor for constant buffer creation
struct ConstantBufferDesc {
    size_t bufferSize;                         // Size of the buffer in bytes
    D3D11_USAGE usage = D3D11_USAGE_DYNAMIC;   // Default usage: dynamic
    UINT bindFlags = D3D11_BIND_CONSTANT_BUFFER; // Default: constant buffer
    UINT cpuAccessFlags = D3D11_CPU_ACCESS_WRITE; // Default: writable by CPU
};

class Shader {
public:
    Shader() = default;
    ~Shader() = default;

    // Shader initialization
    bool Initialize(ID3D11Device* device, const ShaderDesc& desc);

    // Shader binding
    void SetShaders(ID3D11DeviceContext* context);

    // Constant buffer management
    bool CreateConstantBuffer(ID3D11Device* device, const std::string& name, const ConstantBufferDesc& desc);
    void UpdateConstantBuffer(ID3D11DeviceContext* context, const std::string& name, void* data, size_t dataSize);
    void BindConstantBuffer(ID3D11DeviceContext* context, const std::string& name, UINT slot, bool forVertexShader, bool forPixelShader);

private:
    bool CompileAndCreateShader(
        ID3D11Device* device,
        const std::optional<std::wstring>& filePath,
        const std::string& entryPoint,
        const std::string& target,
        Microsoft::WRL::ComPtr<ID3D11DeviceChild> shader);

    // Shader objects
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_geometryShader;
    Microsoft::WRL::ComPtr<ID3D11HullShader> m_hullShader;
    Microsoft::WRL::ComPtr<ID3D11DomainShader> m_domainShader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;

    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    // Constant buffers
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11Buffer>> m_constantBuffers;
};

#endif // !SHADER_H
