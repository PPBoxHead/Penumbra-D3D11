#ifndef SHADER_H
#define SHADER_H

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>


// Descriptor for shader initialization
struct SHADER_DESC {
    std::optional<std::wstring> vertexShaderPath;    // Path to Vertex Shader
    std::optional<std::wstring> pixelShaderPath;     // Path to Pixel Shader
    std::optional<std::wstring> geometryShaderPath;  // Path to Geometry Shader
    std::optional<std::wstring> hullShaderPath;      // Path to Hull Shader
    std::optional<std::wstring> domainShaderPath;    // Path to Domain Shader
    std::optional<std::wstring> computeShaderPath;   // Path to Compute Shader

    // Default entry points
    std::string vertexEntryPoint = "vs_main";
    std::string pixelEntryPoint = "ps_main";
    std::string geometryEntryPoint = "gs_main";
    std::string hullEntryPoint = "hs_main";
    std::string domainEntryPoint = "ds_main";
    std::string computeEntryPoint = "cs_main";

    // Default shader targets
    std::string vertexTarget = "vs_5_0";
    std::string pixelTarget = "ps_5_0";
    std::string geometryTarget = "gs_5_0";
    std::string hullTarget = "hs_5_0";
    std::string domainTarget = "ds_5_0";
    std::string computeTarget = "cs_5_0";
};

// Descriptor for constant buffer creation
struct CONSTANT_BUFFER_DESC {
    // Size of the buffer in bytes
    size_t bufferSize;
    // Default usage: dynamic
    D3D11_USAGE usage = D3D11_USAGE_DYNAMIC;
    // Default: constant buffer
    UINT bindFlags = D3D11_BIND_CONSTANT_BUFFER;
    // Default: writable by CPU
    UINT cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
};

namespace ShaderStage {
    constexpr UINT VertexShader = 0x1;
    constexpr UINT PixelShader = 0x2;
    constexpr UINT GeometryShader = 0x4;
    constexpr UINT HullShader = 0x8;
    constexpr UINT DomainShader = 0x10;
    constexpr UINT ComputeShader = 0x20;
}

class Shader {
    public:
        Shader() = default;
        ~Shader() = default;

        bool Initialize(ID3D11Device* device, const SHADER_DESC& desc, const D3D11_INPUT_ELEMENT_DESC* layout, UINT numElements);
        void SetShaders(ID3D11DeviceContext* context);
        bool CreateConstantBuffer(ID3D11Device* device, const std::string& name, const CONSTANT_BUFFER_DESC& desc);
        void UpdateConstantBuffer(ID3D11DeviceContext* context, const std::string& name, const void* data, size_t dataSize);
        void BindConstantBuffer(ID3D11DeviceContext* context, const std::string& name, UINT slot, UINT shaderFlags);
    
    private:
        bool CompileShader(ID3D11Device* device, const std::optional<std::wstring>& filePath,
            const std::string& entryPoint, const std::string& target,
            Microsoft::WRL::ComPtr<ID3DBlob>& blob);

        template <typename ShaderType>
        bool CreateShader(ID3D11Device* device, const Microsoft::WRL::ComPtr<ID3DBlob>& blob,
            Microsoft::WRL::ComPtr<ShaderType>& shader);
    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_geometryShader;
        Microsoft::WRL::ComPtr<ID3D11HullShader> m_hullShader;
        Microsoft::WRL::ComPtr<ID3D11DomainShader> m_domainShader;
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;

        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11Buffer>> m_constantBuffers;

};

#endif // !SHADER_H
