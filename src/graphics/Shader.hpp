#ifndef SHADER_H
#define SHADER_H

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <vector>



class Shader {
	public:
		Shader() = default;
		~Shader() = default;

		bool Initialize(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath);
		void SetShader(ID3D11DeviceContext* context);
		void UpdateConstantBuffer(ID3D11DeviceContext* context, void* data, size_t dataSize);

	private:
		bool CompileShader(const std::wstring& filePath, const std::string& entryPoint, const std::string& target, Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob);
		void CreateConstantBuffer(ID3D11Device* device, size_t dataSize);

	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
};


#endif // !SHADER_H

