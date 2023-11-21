#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"

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

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;
		void MeshTransformationFunction(const std::vector<Mesh>& meshes_in, std::vector<Mesh>& meshes_out) const;




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

		//Textures:
		Texture* m_pTexture{};

		//utility functions:
		void RenderTri(const Vertex& v0, const Vertex& v1, const Vertex& v2) const;
		void RenderTriWithCurrTexturePtr(const Vertex& v0, const Vertex& v1, const Vertex& v2) const;

		void WeekOneRasterizationOnly(); //part 1, slide 37
		void WeekOneProjectionStage(); //part 2 slide 38
		void WeekOneBaryCentricCoordinates();//part 3 slide 39
		void WeekOneDepthBuffer();//part 4 slide 40
		void WeekOneBBX();//part 5 slide 41
	


		void WeekTwo();
	
	};
}
