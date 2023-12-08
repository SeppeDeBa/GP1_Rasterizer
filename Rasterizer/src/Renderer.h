#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		//week 1
		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const; 
		//week 2
		void MeshTransformationFunction(const std::vector<Mesh>& meshes_in, std::vector<Mesh>& meshes_out) const;
		//week 3
		void VertexTransformationFunctionImproved(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& meshWorldMatrix);
		void ChangeRenderMode();






	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		float m_AspectRatio{};

		//rendering
		enum class RenderMode
		{
			FinalColor,
			DepthBuffer
		};
		RenderMode m_RenderMode{ RenderMode::FinalColor };


		//meshes:
		Mesh m_Mesh{};
		//Textures:
		Texture* m_pTexture{};
		Texture* m_pNormalMap{};
		Texture* m_pSpecularMap{};
		Texture* m_pPhongExponentMap{};//also called Glossiness map
		

		//utility functions:
		void RenderTri(const Vertex& v0, const Vertex& v1, const Vertex& v2) const;
		void RenderTriWithCurrTexturePtr(const Vertex& v0, const Vertex& v1, const Vertex& v2) const;
		void RenderTriWeekThree(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const;

		//week 1
		void WeekOneRasterizationOnly(); //part 1, slide 37
		void WeekOneProjectionStage(); //part 2 slide 38
		void WeekOneBaryCentricCoordinates();//part 3 slide 39
		void WeekOneDepthBuffer();//part 4 slide 40
		void WeekOneBBX();//part 5 slide 41
	
		//week 2
		void WeekTwo();
	

		//week 3
		//got kind of stuck, fully reworking in final version. Frustum culling works, rendering did not
		void WeekThree();
		//week 3 helper functions
		void VertexNDCToRaster(Vertex_Out& vertex);
		float Remap(float valueToRemap, float min, float max) const;
		void InitMesh();
		bool isOutsideFrustum(const Vertex_Out& vertex) const;
		bool isInFrustum(const Vertex_Out& vertex) const;




		//shading and final hand in variables
		void RenderTriangleFinalVersion(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const;



		bool m_UseNormalMap{ true };
		void ToggleNormalMap() { m_UseNormalMap = !m_UseNormalMap; };
		bool m_Rotate{ true };
		const float m_RotationSpeed{ 1.f };
		void ToggleRotation() { m_Rotate = !m_Rotate; };

		
		Light m_MainLight{ 7.f, Vector3{.577f, -.577f, .577f}, ColorRGB{.025f, .025f, .025f} };

		ColorRGB PixelShading(const Vertex_Out& v) const;
		
		enum class ShadingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined
		};
		ShadingMode m_ShadingMode{ ShadingMode::Diffuse };




		//slide 12
		const float PhongShininess{ 25.f };
		ColorRGB Phong(float specularity, float exp, const Vector3& l, const Vector3& v, const Vector3& n) const;
		void FinalVersion();
		

	};
}
