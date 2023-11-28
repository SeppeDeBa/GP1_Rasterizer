//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"



using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_AspectRatio = (float)m_Width / (float)m_Height;
	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(m_AspectRatio, 60.f, { .0f,.0f,-10.f });

	//init textures
	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");
	
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer

	SDL_LockSurface(m_pBackBuffer);

	//===== week 1 =====
	//WeekOneRasterizationOnly();
	//WeekOneProjectionStage();
	//WeekOneBaryCentricCoordinates();
	//WeekOneDepthBuffer();
	//WeekOneBBX();


	//===== week 2 =====
	//WeekTwo();


	//===== week 3 =====
	WeekThree();
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo DONE > W1 Projection Stage
	for (const auto& vertex : vertices_in)
	{
		const Vector3 transformPos = m_Camera.viewMatrix.TransformPoint(vertex.position);

		float xProjection{};
		float yProjection{};


		//slide 16 LHS dividing
		if (transformPos.z != 0)
		{
			xProjection = transformPos.x / transformPos.z;
			yProjection = transformPos.y / transformPos.z;
		}
		else
		{
			xProjection = transformPos.x;
			yProjection = transformPos.y;
		}

		xProjection /= (float(m_Width) / float(m_Height)) * m_Camera.fov;//TODO: Replace with aspect ratio
		yProjection /= m_Camera.fov;

		//NDC conversion
		const float newPosX = ((xProjection + 1) / 2) * m_Width;
		const float newPosY = ((1 - yProjection) / 2) * m_Height;
		const Vector3 newPos = Vector3{ newPosX, newPosY, vertex.position.z };
		vertices_out.push_back(Vertex{ newPos, vertex.color });
	}



}

void dae::Renderer::MeshTransformationFunction(const std::vector<Mesh>& meshes_in, std::vector<Mesh>& meshes_out) const
{
	//copy mesh at start
	meshes_out = meshes_in;
	for (auto& mesh : meshes_out)
	{
		for (auto& vertex : mesh.vertices) //TODO: can maybe make a VertexTransformation function overload that is compatible with this?
		{
			const Vector3 transformPos = m_Camera.viewMatrix.TransformPoint(vertex.position);

			float xProjection{};
			float yProjection{};


			//slide 16 LHS dividing
			if (transformPos.z != 0)
			{
				xProjection = transformPos.x / transformPos.z;
				yProjection = transformPos.y / transformPos.z;
			}
			else
			{
				xProjection = transformPos.x;
				yProjection = transformPos.y;
			}

			xProjection /= (float(m_Width) / float(m_Height)) * m_Camera.fov;//TODO: Replace with aspect ratio
			yProjection /= m_Camera.fov;

			//NDC conversion
			const float newPosX = ((xProjection + 1) / 2) * m_Width;
			const float newPosY = ((1 - yProjection) / 2) * m_Height;
			const Vector3 newPos = Vector3{ newPosX, newPosY, vertex.position.z };
			vertex.position = newPos;
		}
	}


}

void dae::Renderer::VertexTransformationFunctionImproved(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& meshWorldMatrix)
{
	vertices_out.resize(vertices_in.size());

	//slide 11 week 8
	const Matrix worldViewProjectionMatrix = meshWorldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

	for (int i{}; i < vertices_in.size(); ++i)
	{
		Vector4 newSpacePos = worldViewProjectionMatrix.TransformPoint(Vector4{ vertices_in[i].position, 1 });

		//prespective divide also slide 11
		newSpacePos.x /= newSpacePos.w;
		newSpacePos.y /= newSpacePos.w;
		newSpacePos.z /= newSpacePos.w;
		//transformPos.w = transformPos.w; commented becaus not necessary?

		//coordinates are now defined in set boundary

		vertices_out[i].position = newSpacePos;
		vertices_out[i].color = vertices_in[i].color;
		vertices_out[i].uv = vertices_in[i].uv;
		vertices_out[i].normal = meshWorldMatrix.TransformVector(vertices_in[i].normal).Normalized(); //Normal and tangent in world space
		vertices_out[i].tangent = meshWorldMatrix.TransformVector(vertices_in[i].tangent).Normalized();
		//TODO Add direction when added to dataTypes
	}




}

void dae::Renderer::RenderTri(const Vertex& v0, const Vertex& v1, const Vertex& v2) const
{
	ColorRGB finalColor{ 0,0,0 };


	//=== build bbx === slide 32 
	//1. find top left and right bottom
		//1.1 vertex 0 and 1
	Vector2 topLeft{ std::min(v0.position.x, v1.position.x), std::min(v0.position.y, v1.position.y) };
	Vector2 bottomRight{ std::max(v0.position.x, v1.position.x), std::max(v0.position.y, v1.position.y) };
	//1.2 vertex 0&1 and 2
	topLeft = Vector2{ std::min(topLeft.x, v2.position.x), std::min(topLeft.y, v2.position.y) };
	bottomRight = Vector2{ std::max(bottomRight.x, v2.position.x), std::max(bottomRight.y, v2.position.y) };


	//TODO: Ask about slide 32, what do types matter besides having to be cast?
	//2. check if it doesn't exceed screen // offset of 1 according to slide 32
		//2.1 x check
	//testing if static cast would be better. Doesn't relly matter as the check here is not needed.
	topLeft.x = Clamp(static_cast<int>(topLeft.x), 0, m_Width - 1);
	bottomRight.x = Clamp((int)bottomRight.x, 0, m_Width - 1);

	//2.2 y check
	topLeft.y = Clamp((int)topLeft.y, 0, m_Height - 1);
	bottomRight.y = Clamp((int)bottomRight.y, 0, m_Height - 1);

	//RENDER LOGIC
	for (int px{ static_cast<int>(topLeft.x) }; px < bottomRight.x; ++px)
	{
		for (int py{ static_cast<int>(topLeft.y) }; py < bottomRight.y; ++py)
		{
			//define pixelPos with 0.5 offset
			const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };


			//sideA Check
			const Vector2 sideA = Vector2{ v1.position.x - v0.position.x,
										   v1.position.y - v0.position.y };

			const Vector2 v0ToPixel = Vector2{ currentPixel.x - v0.position.x,
											   currentPixel.y - v0.position.y };
			float crossResultSideA{ Vector2::Cross(sideA, v0ToPixel) };
			float weightV0{ crossResultSideA };


			//old system, leaving it only here as a reference, deleting it where it was used later.
			//now I check at the end if all is larger than 0 to not have to pass a bool around!
			//if (crossResultSideA < 0)
			//{
			//	pixelIsInTriangle = false;
			//}
			//else weightV0 = crossResultSideA;



			//sideB Check
			const Vector2 sideB = Vector2{ v2.position.x - v1.position.x,
										   v2.position.y - v1.position.y };

			const Vector2 v1ToPixel = Vector2{ currentPixel.x - v1.position.x,
													currentPixel.y - v1.position.y };
			float crossResultSideB{ Vector2::Cross(sideB, v1ToPixel) };
			float weightV1{ crossResultSideB };


			//sideC check
			const Vector2 sideC = Vector2{ v0.position.x - v2.position.x,
										   v0.position.y - v2.position.y };

			const Vector2 v2ToPixel = Vector2{ currentPixel.x - v2.position.x,
													currentPixel.y - v2.position.y };
			float crossResultSideC{ Vector2::Cross(sideC, v2ToPixel) };
			float weightV2{ crossResultSideC };


			//check if pixel is in triangle
			if (crossResultSideA > 0 && crossResultSideB > 0 && crossResultSideC > 0)
			{
				//all crosses are positive so pixel is in current triangle
			//calc total weights; (slide 26 reference)
				const float totalWeight{ weightV0 + weightV1 + weightV2 };

				weightV0 /= totalWeight;
				weightV1 /= totalWeight;
				weightV2 /= totalWeight;

				//slide 27-29 interpolate between depth values using barycentric weights
				const float interpolatedDepth = v0.position.z * weightV0 + v1.position.z * weightV1 + v2.position.z * weightV2;

				//if depth is smaller than what is stored in buffer, overwrite (slide 30)
				if (interpolatedDepth < m_pDepthBufferPixels[px * m_Height + py]) //Depth test (slide 29)
				{
					m_pDepthBufferPixels[px * m_Height + py] = interpolatedDepth; //Depth write (slide 29)

					finalColor = v0.color * weightV0 + v1.color * weightV1 + v2.color * weightV2;

					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}
}

void dae::Renderer::RenderTriWithCurrTexturePtr(const Vertex& v0, const Vertex& v1, const Vertex& v2) const
{
	ColorRGB finalColor{ 0,0,0 };


	//=== build bbx === slide 32 
	//1. find top left and right bottom
		//1.1 vertex 0 and 1
	Vector2 topLeft{ std::min(v0.position.x, v1.position.x), std::min(v0.position.y, v1.position.y) };
	Vector2 bottomRight{ std::max(v0.position.x, v1.position.x), std::max(v0.position.y, v1.position.y) };
	//1.2 vertex 0&1 and 2
	topLeft = Vector2{ std::min(topLeft.x, v2.position.x), std::min(topLeft.y, v2.position.y) };
	bottomRight = Vector2{ std::max(bottomRight.x, v2.position.x), std::max(bottomRight.y, v2.position.y) };


	//TODO: Ask about slide 32, what do types matter besides having to be cast?
	//2. check if it doesn't exceed screen // offset of 1 according to slide 32
		//2.1 x check
	//testing if static cast would be better. Doesn't relly matter as the check here is not needed.
	topLeft.x = Clamp(static_cast<int>(topLeft.x), 0, m_Width - 1);
	bottomRight.x = Clamp((int)bottomRight.x, 0, m_Width - 1);

	//2.2 y check
	topLeft.y = Clamp((int)topLeft.y, 0, m_Height - 1);
	bottomRight.y = Clamp((int)bottomRight.y, 0, m_Height - 1);

	//RENDER LOGIC
	for (int px{ static_cast<int>(topLeft.x) }; px < bottomRight.x; ++px)
	{
		for (int py{ static_cast<int>(topLeft.y) }; py < bottomRight.y; ++py)
		{
			//define pixelPos with 0.5 offset
			const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };


			//sideA Check
			const Vector2 sideA = Vector2{ v1.position.x - v0.position.x,
										   v1.position.y - v0.position.y }; 
			//todo: check into vector substraction -> overloaded operator - is already implented

			const Vector2 v0ToPixel = Vector2{ currentPixel.x - v0.position.x,
											   currentPixel.y - v0.position.y };
			float crossResultSideA{ Vector2::Cross(sideA, v0ToPixel) };
			float weightV2{ crossResultSideA };


			//old system, leaving it only here as a reference, deleting it where it was used later.
			//now I check at the end if all is larger than 0 to not have to pass a bool around!
			//if (crossResultSideA < 0)
			//{
			//	pixelIsInTriangle = false;
			//}
			//else weightV0 = crossResultSideA;



			//sideB Check
			const Vector2 sideB = Vector2{ v2.position.x - v1.position.x,
										   v2.position.y - v1.position.y };

			const Vector2 v1ToPixel = Vector2{ currentPixel.x - v1.position.x,
													currentPixel.y - v1.position.y };
			float crossResultSideB{ Vector2::Cross(sideB, v1ToPixel) };
			float weightV0{ crossResultSideB };


			//sideC check
			const Vector2 sideC = Vector2{ v0.position.x - v2.position.x,
										   v0.position.y - v2.position.y };

			const Vector2 v2ToPixel = Vector2{ currentPixel.x - v2.position.x,
													currentPixel.y - v2.position.y };
			float crossResultSideC{ Vector2::Cross(sideC, v2ToPixel) };
			float weightV1{ crossResultSideC };


			//check if pixel is in triangle
			if (crossResultSideA > 0 && crossResultSideB > 0 && crossResultSideC > 0)
			{
				//all crosses are positive so pixel is in current triangle
			//calc total weights; (slide 26 reference)
				const float totalWeight{ weightV0 + weightV1 + weightV2 };

				weightV0 /= totalWeight;
				weightV1 /= totalWeight;
				weightV2 /= totalWeight;

				//slide 27-29 interpolate between depth values using barycentric weights
				/*const float interpolatedDepth = v0.position.z * weightV0 + v1.position.z * weightV1 + v2.position.z * weightV2;*/
				//slide 21 week 7
				const float interpolatedDepth =
					1.f / 
					(
						(1 / v0.position.z * weightV0) +
						(1 / v1.position.z * weightV1) +
						(1 / v2.position.z * weightV2)
					);
					
					//old interpolatedDepth methodv0.position.z * weightV0 + v1.position.z * weightV1 + v2.position.z * weightV2;



				//if depth is smaller than what is stored in buffer, overwrite (slide 30)
				if (interpolatedDepth < m_pDepthBufferPixels[px * m_Height + py])
				{
					m_pDepthBufferPixels[px * m_Height + py] = interpolatedDepth;

					//finalColor = v0.color * weightV0 + v1.color * weightV1 + v2.color * weightV2;

					const Vector2 interpolatedUV = 
						(
							((v0.uv / v0.position.z) * weightV0) +
							((v1.uv / v1.position.z) * weightV1) +
							((v2.uv / v2.position.z) * weightV2)
							)
						* interpolatedDepth;	

					finalColor = m_pTexture->Sample(interpolatedUV);
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}
}

void dae::Renderer::RenderTriWeekThree(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const
{
	ColorRGB finalColor{ 0,0,0 };


	//=== build bbx === slide 32 
	//1. find top left and right bottom
		//1.1 vertex 0 and 1
	Vector2 topLeft{ std::min(v0.position.x, v1.position.x), std::min(v0.position.y, v1.position.y) };
	Vector2 bottomRight{ std::max(v0.position.x, v1.position.x), std::max(v0.position.y, v1.position.y) };
	//1.2 vertex 0&1 and 2
	topLeft = Vector2{ std::min(topLeft.x, v2.position.x), std::min(topLeft.y, v2.position.y) };
	bottomRight = Vector2{ std::max(bottomRight.x, v2.position.x), std::max(bottomRight.y, v2.position.y) };


	//TODO: Ask about slide 32, what do types matter besides having to be cast?
	//2. check if it doesn't exceed screen // offset of 1 according to slide 32
		//2.1 x check
	//testing if static cast would be better. Doesn't relly matter as the check here is not needed.
	topLeft.x = Clamp(static_cast<int>(topLeft.x), 0, m_Width - 1);
	bottomRight.x = Clamp((int)bottomRight.x, 0, m_Width - 1);

	//2.2 y check
	topLeft.y = Clamp((int)topLeft.y, 0, m_Height - 1);
	bottomRight.y = Clamp((int)bottomRight.y, 0, m_Height - 1);

	//RENDER LOGIC
	for (int px{ static_cast<int>(topLeft.x) }; px < bottomRight.x; ++px)
	{
		for (int py{ static_cast<int>(topLeft.y) }; py < bottomRight.y; ++py)
		{
			//define pixelPos with 0.5 offset
			const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };


			//sideA Check
			const Vector2 sideA = Vector2{ v1.position.x - v0.position.x,
										   v1.position.y - v0.position.y };
			//todo: check into vector substraction -> overloaded operator - is already implented

			const Vector2 v0ToPixel = Vector2{ currentPixel.x - v0.position.x,
											   currentPixel.y - v0.position.y };
			float crossResultSideA{ Vector2::Cross(sideA, v0ToPixel) };
			
			if (crossResultSideA < 0) continue; //changing to early out structure to avoid nesting (did some "youtubing" on how general good practices :))
			float weightV2{ crossResultSideA };





			//sideB Check
			const Vector2 sideB = Vector2{ v2.position.x - v1.position.x,
										   v2.position.y - v1.position.y };

			const Vector2 v1ToPixel = Vector2{ currentPixel.x - v1.position.x,
													currentPixel.y - v1.position.y };
			float crossResultSideB{ Vector2::Cross(sideB, v1ToPixel) };

			if (crossResultSideB < 0) continue;
			float weightV0{ crossResultSideB };


			//sideC check
			const Vector2 sideC = Vector2{ v0.position.x - v2.position.x,
										   v0.position.y - v2.position.y };

			const Vector2 v2ToPixel = Vector2{ currentPixel.x - v2.position.x,
													currentPixel.y - v2.position.y };
			float crossResultSideC{ Vector2::Cross(sideC, v2ToPixel) };
			if (crossResultSideA < 0) continue;
			float weightV1{ crossResultSideC };



			//all crosses are positive so pixel is in current triangle
		//calc total weights; (slide 26 reference)
			const float totalWeight{ weightV0 + weightV1 + weightV2 };

			weightV0 /= totalWeight;
			weightV1 /= totalWeight;
			weightV2 /= totalWeight;

	
			//week 8 slide 16 step 1
			const float zBufferValue =
				1.f /
				(
					(1 / v0.position.z * weightV0) +
					(1 / v1.position.z * weightV1) +
					(1 / v2.position.z * weightV2)
					);


			//TODO: frustum here?
			if (zBufferValue < 0 || zBufferValue > 1) continue;
			//if (zBufferValue < m_Camera.nearPlane || zBufferValue > m_Camera.farPlane) continue;
			//week 8 slide 16 step 2
			if (zBufferValue < m_pDepthBufferPixels[px * m_Height + py]) //depth read
			{
				//depth write
				m_pDepthBufferPixels[px * m_Height + py] = zBufferValue;

				//slide 16 week 8 step 3.1
				const float interpolatedW = 
					1.f /
					(
						(1.f / v0.position.w) * weightV0 +
						(1.f / v1.position.w) * weightV1 +
						(1.f / v2.position.w) * weightV2
					);


				//finalColor = v0.color * weightV0 + v1.color * weightV1 + v2.color * weightV2;

				//slide 16 week 8 step 3.2
				const Vector2 interpolatedUV =
					(
						((v0.uv / v0.position.z) * weightV0) +
						((v1.uv / v1.position.z) * weightV1) +
						((v2.uv / v2.position.z) * weightV2)
						)
					* interpolatedW;

				switch (m_RenderMode)
				{
				case dae::Renderer::RenderMode::FinalColor:
					finalColor = m_pTexture->Sample(interpolatedUV);
					break;

				case dae::Renderer::RenderMode::DepthBuffer:
					const float depth{ Remap(zBufferValue, 0.985f, 1.f) };
					finalColor = { depth, depth, depth };
					break;
				//TODO CPP QUESTION: Why does this break with a default?
				}


				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}
}

void dae::Renderer::WeekOneRasterizationOnly()
{
	//Triangle in NDC space
	const std::vector<Vertex> vertices_ndc{
		Vertex{Vector3(0.f, 0.5f, 1.f), colors::White},
		Vertex{Vector3(0.5f, -0.5f, 1.f), colors::White},
		Vertex{Vector3(-0.5f, -0.5f, 1.f), colors::White}
	};

	//Turn NDC space into Screen space (also called Raster Space)
	std::vector<Vertex> vertices_ss{};
	for (const Vertex& vertex : vertices_ndc)
	{
		const float posX = ((vertex.position.x + 1) / 2) * m_Width;
		const float posY = ((1 - vertex.position.y) / 2) * m_Height;
		const Vector3 newPos = Vector3{ posX, posY, vertex.position.z }; //saving vertex.pos.z in case i need it later?
		vertices_ss.push_back(Vertex{ newPos, vertex.color });
	}

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//define pixelPos with 0.5 offset
			const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };

			//checking if pixel is in triangle
			bool pixelIsInTriangle{ true };

			//sideA check
			const Vector2 sideA = Vector2{ vertices_ss[1].position.x - vertices_ss[0].position.x,
										   vertices_ss[1].position.y - vertices_ss[0].position.y };

			const Vector2 v0ToPixel = Vector2{ currentPixel.x - vertices_ss[0].position.x,
											   currentPixel.y - vertices_ss[0].position.y };

			if (Vector2::Cross(sideA, v0ToPixel) < 0)
			{
				pixelIsInTriangle = false;
			}

			//sideB check
			const Vector2 sideB = Vector2{ vertices_ss[2].position.x - vertices_ss[1].position.x,
										   vertices_ss[2].position.y - vertices_ss[1].position.y };

			const Vector2 v1ToPixel = Vector2{ currentPixel.x - vertices_ss[1].position.x,
													currentPixel.y - vertices_ss[1].position.y };

			if (Vector2::Cross(sideB, v1ToPixel) < 0)
			{
				pixelIsInTriangle = false;
			}

			//sideC check
			const Vector2 sideC = Vector2{ vertices_ss[0].position.x - vertices_ss[2].position.x,
										   vertices_ss[0].position.y - vertices_ss[2].position.y };

			const Vector2 v2ToPixel = Vector2{ currentPixel.x - vertices_ss[2].position.x,
													currentPixel.y - vertices_ss[2].position.y };

			if (Vector2::Cross(sideC, v2ToPixel) < 0)
			{
				pixelIsInTriangle = false;
			}

			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;

			ColorRGB finalColor{ 0,0,0 };
			if (pixelIsInTriangle)
			{
				finalColor = ColorRGB{ 1, 1, 1 };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

}

void dae::Renderer::WeekOneProjectionStage()
{
	//Triangle in NDC space
	const std::vector<Vertex> vertices_world{
		Vertex{Vector3(0.f, 2.f, 0.f), colors::White},
		Vertex{Vector3(1.f, 0.f, 0.f), colors::White},
		Vertex{Vector3(-1.f,0.f, 0.f), colors::White}
	};

	//Turn NDC space into Screen space (also called Raster Space)
	std::vector<Vertex> vertices_ss{};

	VertexTransformationFunction(vertices_world, vertices_ss);

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//define pixelPos with 0.5 offset
			const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };

			//checking if pixel is in triangle
			bool pixelIsInTriangle{ true };

			//sideA Check
			const Vector2 sideA = Vector2{ vertices_ss[1].position.x - vertices_ss[0].position.x,
										   vertices_ss[1].position.y - vertices_ss[0].position.y };

			const Vector2 v0ToPixel = Vector2{ currentPixel.x - vertices_ss[0].position.x,
											   currentPixel.y - vertices_ss[0].position.y };

			if (Vector2::Cross(sideA, v0ToPixel) < 0)
			{
				pixelIsInTriangle = false;
			}

			//sideB Check
			const Vector2 sideB = Vector2{ vertices_ss[2].position.x - vertices_ss[1].position.x,
										   vertices_ss[2].position.y - vertices_ss[1].position.y };

			const Vector2 v1ToPixel = Vector2{ currentPixel.x - vertices_ss[1].position.x,
													currentPixel.y - vertices_ss[1].position.y };

			if (Vector2::Cross(sideB, v1ToPixel) < 0)
			{
				pixelIsInTriangle = false;
			}

			//sideC check
			const Vector2 sideC = Vector2{ vertices_ss[0].position.x - vertices_ss[2].position.x,
										   vertices_ss[0].position.y - vertices_ss[2].position.y };

			const Vector2 v2ToPixel = Vector2{ currentPixel.x - vertices_ss[2].position.x,
													currentPixel.y - vertices_ss[2].position.y };

			if (Vector2::Cross(sideC, v2ToPixel) < 0)
			{
				pixelIsInTriangle = false;
			}

			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;

			ColorRGB finalColor{ 0,0,0 };
			if (pixelIsInTriangle)
			{
				finalColor = ColorRGB{ 1, 1, 1 };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::WeekOneBaryCentricCoordinates()
{
	//Triangle in NDC space
	const std::vector<Vertex> vertices_world{
		Vertex{Vector3(0.f, 4.f, 2.f),{1, 0, 0}},
		Vertex{Vector3(3.f, -2.f, 2.f),{0, 1, 0}},
		Vertex{Vector3(-3.f,-2.f, 2.f), {0, 0, 1}}
	};

	//Turn NDC space into Screen space (also called Raster Space)
	std::vector<Vertex> vertices_ss{};

	VertexTransformationFunction(vertices_world, vertices_ss);


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//define pixelPos with 0.5 offset
			const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };

			//checking if pixel is in triangle && initialize weights
			bool pixelIsInTriangle{ true };


			//init weight values, slide 26 for reference
			float weightV0{}, weightV1{}, weightV2{};

			//sideA Check
			const Vector2 sideA = Vector2{ vertices_ss[1].position.x - vertices_ss[0].position.x,
										   vertices_ss[1].position.y - vertices_ss[0].position.y };

			const Vector2 v0ToPixel = Vector2{ currentPixel.x - vertices_ss[0].position.x,
											   currentPixel.y - vertices_ss[0].position.y };
			
			float crossResultSideA{ Vector2::Cross(sideA, v0ToPixel) };
			if (crossResultSideA < 0)
			{
				pixelIsInTriangle = false;
			}
			else weightV0 = crossResultSideA;
			

			//sideB Check
			const Vector2 sideB = Vector2{ vertices_ss[2].position.x - vertices_ss[1].position.x,
										   vertices_ss[2].position.y - vertices_ss[1].position.y };

			const Vector2 v1ToPixel = Vector2{ currentPixel.x - vertices_ss[1].position.x,
													currentPixel.y - vertices_ss[1].position.y };

			float crossResultSideB{ Vector2::Cross(sideB, v1ToPixel) };
			if (crossResultSideB < 0)
			{
				pixelIsInTriangle = false;
			}
			else weightV1 = crossResultSideB;
			

			//sideC check
			const Vector2 sideC = Vector2{ vertices_ss[0].position.x - vertices_ss[2].position.x,
										   vertices_ss[0].position.y - vertices_ss[2].position.y };

			const Vector2 v2ToPixel = Vector2{ currentPixel.x - vertices_ss[2].position.x,
													currentPixel.y - vertices_ss[2].position.y };

			float crossResultSideC{ Vector2::Cross(sideC, v2ToPixel) };
			if (crossResultSideC < 0)
			{
				pixelIsInTriangle = false;
			}
			else weightV2 = crossResultSideC;


			//calc total weights; (slide 26 reference)

			const float totalWeight{ weightV0 + weightV1 + weightV2 };

			weightV0 /= totalWeight;
			weightV1 /= totalWeight;
			weightV2 /= totalWeight;
			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;

			ColorRGB finalColor{ 0,0,0 };
			if (pixelIsInTriangle)
			{
				//shuffled weights to match pdf example
				finalColor = vertices_ss[0].color * weightV1 
						   + vertices_ss[1].color * weightV2
						   + vertices_ss[2].color * weightV0;
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::WeekOneDepthBuffer()
{
	//Triangle in NDC space
	const std::vector<Vertex> vertices_world{
		//triangle 0
		Vertex{Vector3(0.f, 2.f, 0.f), {1, 0, 0}},
		Vertex{Vector3(1.5f, -1.f, 0.f), {1, 0, 0}},
		Vertex{Vector3(-1.5f, -1.f, 0.f), {1, 0, 0}},

		//triangle 1
		Vertex{Vector3(0.f, 4.f, 2.f), {1, 0, 0}},
		Vertex{Vector3(3.f, -2.f, 2.f), {0, 1, 0}},
		Vertex{Vector3(-3.f, -2.f, 2.f), {0, 0, 1}}
	};

	//Turn NDC space into Screen space (also called Raster Space)
	std::vector<Vertex> vertices_ss{};

	VertexTransformationFunction(vertices_world, vertices_ss);
	//slide 40
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, 100);

	//slide 29
	ColorRGB clearColor{ 100, 100, 100 };

	//TODO: ASK TEACHER!!! why does reverse work, what is a better way?
	//putting all values //using 32 bit storage split up per 8 bits being: 0x alpha, red, green, blue
	//https://wiki.libsdl.org/SDL2/SDL_FillRect

	Uint32 clearColorUint = 0xFF000000 | (Uint32)clearColor.r | (Uint32)clearColor.g << 8 | (Uint32)clearColor.b << 16;
	SDL_FillRect(m_pBackBuffer, NULL, clearColorUint);

	ColorRGB finalColor{ clearColor * 255.f};//rescale to 1


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//reworking bool method of pixelIsInTriangle based on tip of fellow student.
			for (int currTriVertIt{}; (currTriVertIt + 2) < vertices_world.size(); currTriVertIt += 3.f) //the +2 is there to check that the loop does not go past the vector
			{
				//define all vertices of current 
				Vertex v0 = vertices_ss[currTriVertIt];
				Vertex v1 = vertices_ss[currTriVertIt + 1];
				Vertex v2 = vertices_ss[currTriVertIt + 2];

				//define pixelPos with 0.5 offset
				const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };


				//sideA Check
				const Vector2 sideA = Vector2{ v1.position.x - v0.position.x,
											   v1.position.y - v0.position.y };

				const Vector2 v0ToPixel = Vector2{ currentPixel.x - v0.position.x,
												   currentPixel.y - v0.position.y };

				float crossResultSideA{ Vector2::Cross(sideA, v0ToPixel) };
				float weightV0{ crossResultSideA };


				//old system, leaving it only here as a reference, deleting it where it was used later.
				//now I check at the end if all is larger than 0 to not have to pass a bool around!
				//if (crossResultSideA < 0)
				//{
				//	pixelIsInTriangle = false;
				//}
				//else weightV0 = crossResultSideA;



				//sideB Check
				const Vector2 sideB = Vector2{ v2.position.x - v1.position.x,
											   v2.position.y - v1.position.y };

				const Vector2 v1ToPixel = Vector2{ currentPixel.x - v1.position.x,
														currentPixel.y - v1.position.y };

				float crossResultSideB{ Vector2::Cross(sideB, v1ToPixel) };
				float weightV1{ crossResultSideB };


				//sideC check
				const Vector2 sideC = Vector2{ v0.position.x - v2.position.x,
											   v0.position.y - v2.position.y };

				const Vector2 v2ToPixel = Vector2{ currentPixel.x - v2.position.x,
														currentPixel.y - v2.position.y };

				float crossResultSideC{ Vector2::Cross(sideC, v2ToPixel) };
				float weightV2{ crossResultSideC };


				//check if pixel is in triangle
				if (crossResultSideA > 0 && crossResultSideB > 0 && crossResultSideC > 0)
				{
					//all crosses are positive so pixel is in current triangle
				//calc total weights; (slide 26 reference)
					const float totalWeight{ weightV0 + weightV1 + weightV2 };

					weightV0 /= totalWeight;
					weightV1 /= totalWeight;
					weightV2 /= totalWeight;

					//slide 27-29 interpolate between depth values using barycentric weights
					const float interpolatedDepth = v0.position.z * weightV0 + v1.position.z * weightV1 + v2.position.z * weightV2;

					//if depth is smaller than what is stored in buffer, overwrite (slide 30)
					if (interpolatedDepth < m_pDepthBufferPixels[px * m_Height + py]) //Depth test (slide 29)
					{
						m_pDepthBufferPixels[px * m_Height + py] = interpolatedDepth; //Depth write (slide 29)

						finalColor = v0.color * weightV0 + v1.color * weightV1 + v2.color * weightV2;
					}
				}
			}
			
			//redundant?
			if (m_pDepthBufferPixels[px * m_Height + py] == FLT_MAX)
			{
				finalColor = ColorRGB{ clearColor / 255.f };//tip from fellow student to do this
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::WeekOneBBX()
{
	//Triangle in NDC space
	const std::vector<Vertex> vertices_world{
		//triangle 0
		Vertex{Vector3(0.f, 2.f, 0.f), {1, 0, 0}},
		Vertex{Vector3(1.5f, -1.f, 0.f), {1, 0, 0}},
		Vertex{Vector3(-1.5f, -1.f, 0.f), {1, 0, 0}},

		//triangle 1
		Vertex{Vector3(0.f, 4.f, 2.f), {1, 0, 0}},
		Vertex{Vector3(3.f, -2.f, 2.f), {0, 1, 0}},
		Vertex{Vector3(-3.f, -2.f, 2.f), {0, 0, 1}}
	};

	//Turn NDC space into Screen space (also called Raster Space)
	std::vector<Vertex> vertices_ss{};

	VertexTransformationFunction(vertices_world, vertices_ss);
	//slide 40
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, 100);

	//slide 29
	ColorRGB clearColor{ 100, 100, 100 };

	//TODO: ASK TEACHER!!! why does reverse work, what is a better way?
	//putting all values //using 32 bit storage split up per 8 bits being: 0x alpha, red, green, blue
	//https://wiki.libsdl.org/SDL2/SDL_FillRect

	Uint32 clearColorUint = 0xFF000000 | (Uint32)clearColor.r | (Uint32)clearColor.g << 8 | (Uint32)clearColor.b << 16;
	SDL_FillRect(m_pBackBuffer, NULL, clearColorUint);

	ColorRGB finalColor{ clearColor * 255.f };//rescale to 1


	for (int currTriVertIt{}; (currTriVertIt + 2) < vertices_world.size(); currTriVertIt += 3.f) //the +2 is there to check that the loop does not go past the vector
	{
		//define all vertices of current 
		Vertex v0 = vertices_ss[currTriVertIt];
		Vertex v1 = vertices_ss[currTriVertIt + 1];
		Vertex v2 = vertices_ss[currTriVertIt + 2];


		//=== build bbx === slide 32 
		//1. find top left and right bottom
			//1.1 vertex 0 and 1
		Vector2 topLeft{ std::min(v0.position.x, v1.position.x), std::min(v0.position.y, v1.position.y) };
		Vector2 bottomRight{ std::max(v0.position.x, v1.position.x), std::max(v0.position.y, v1.position.y) };
		//1.2 vertex 0&1 and 2
		topLeft = Vector2{ std::min(topLeft.x, v2.position.x), std::min(topLeft.y, v2.position.y) };
		bottomRight = Vector2{ std::max(bottomRight.x, v2.position.x), std::max(bottomRight.y, v2.position.y) };


		//TODO: Ask about slide 32, what do types matter besides having to be cast?
		//2. check if it doesn't exceed screen // offset of 1 according to slide 32
			//2.1 x check
		topLeft.x = Clamp((int)topLeft.x, 0, m_Width - 1);
		bottomRight.x = Clamp((int)bottomRight.x, 0, m_Width - 1);

		//2.2 y check
		topLeft.y = Clamp((int)topLeft.y, 0, m_Height - 1);
		bottomRight.y = Clamp((int)bottomRight.y, 0, m_Height - 1);

		//RENDER LOGIC
		for (int px{static_cast<int>(topLeft.x)}; px < bottomRight.x; ++px)
		{
			for (int py{ static_cast<int>(topLeft.y) }; py < bottomRight.y; ++py)
			{	
				//define pixelPos with 0.5 offset
				const Vector2 currentPixel = Vector2{ static_cast<float>(px) + 0.5f,  static_cast<float>(py) + 0.5f };


				//sideA Check
				const Vector2 sideA = Vector2{ v1.position.x - v0.position.x,
											   v1.position.y - v0.position.y };

				const Vector2 v0ToPixel = Vector2{ currentPixel.x - v0.position.x,
												   currentPixel.y - v0.position.y };
				float crossResultSideA{ Vector2::Cross(sideA, v0ToPixel) };
				float weightV0{ crossResultSideA };


				//old system, leaving it only here as a reference, deleting it where it was used later.
				//now I check at the end if all is larger than 0 to not have to pass a bool around!
				//if (crossResultSideA < 0)
				//{
				//	pixelIsInTriangle = false;
				//}
				//else weightV0 = crossResultSideA;



				//sideB Check
				const Vector2 sideB = Vector2{ v2.position.x - v1.position.x,
											   v2.position.y - v1.position.y };

				const Vector2 v1ToPixel = Vector2{ currentPixel.x - v1.position.x,
														currentPixel.y - v1.position.y };
				float crossResultSideB{ Vector2::Cross(sideB, v1ToPixel) };
				float weightV1{ crossResultSideB };


				//sideC check
				const Vector2 sideC = Vector2{ v0.position.x - v2.position.x,
											   v0.position.y - v2.position.y };

				const Vector2 v2ToPixel = Vector2{ currentPixel.x - v2.position.x,
														currentPixel.y - v2.position.y };
				float crossResultSideC{ Vector2::Cross(sideC, v2ToPixel) };
				float weightV2{ crossResultSideC };


				//check if pixel is in triangle
				if (crossResultSideA > 0 && crossResultSideB > 0 && crossResultSideC > 0)
				{
					//all crosses are positive so pixel is in current triangle
				//calc total weights; (slide 26 reference)
					const float totalWeight{ weightV0 + weightV1 + weightV2 };

					weightV0 /= totalWeight;
					weightV1 /= totalWeight;
					weightV2 /= totalWeight;

					//slide 27-29 interpolate between depth values using barycentric weights
					const float interpolatedDepth = v0.position.z * weightV0 + v1.position.z * weightV1 + v2.position.z * weightV2;

					//if depth is smaller than what is stored in buffer, overwrite (slide 30)
					if (interpolatedDepth < m_pDepthBufferPixels[px * m_Height + py]) //Depth test (slide 29)
					{
						m_pDepthBufferPixels[px * m_Height + py] = interpolatedDepth; //Depth write (slide 29)

						finalColor = v0.color * weightV0 + v1.color * weightV1 + v2.color * weightV2;
						
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void dae::Renderer::WeekTwo()
{
	//Define Triangle _ Vertices in world space
	std::vector<Mesh> meshes_world{
		Mesh{
			{
			Vertex{Vector3(-3.f, 3.f, -2.f), colors::White, Vector2(0,0)},
			Vertex{Vector3(0.f, 3.f, -2.f), colors::White, Vector2(.5f,0)},
			Vertex{Vector3(3.f, 3.f, -2.f), colors::White, Vector2(1,0)},
			Vertex{Vector3(-3.f, 0.f, -2.f), colors::White, Vector2(0,.5f)},
			Vertex{Vector3(0.f, 0.f, -2.f), colors::White, Vector2(.5f, .5f)},
			Vertex{Vector3(3.f, 0.f, -2.f), colors::White, Vector2(1, .5f)},
			Vertex{Vector3(-3.f, -3.f, -2.f), colors::White, Vector2(0, 1)},
			Vertex{Vector3(0.f, -3.f, -2.f), colors::White, Vector2(.5f, 1)},
			Vertex{Vector3(3.f, -3.f, -2.f), colors::White, Vector2(1,1)}
			},{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},
			PrimitiveTopology::TriangleStrip
		}
	};


	std::vector<Mesh> meshes_ss{};
	//convert to screen space
	MeshTransformationFunction(meshes_world, meshes_ss);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, 100);

	ColorRGB clearColor = ColorRGB{ 100,100,100 };
	Uint32 clearColorUint = 0xFF000000 | (Uint32)clearColor.r | (Uint32)clearColor.g << 8 | (Uint32)clearColor.b << 16;
	SDL_FillRect(m_pBackBuffer, NULL, clearColorUint);

	//RENDER LOGIC

	for (auto& mesh : meshes_ss)
	{
		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList) 
		{
			for (int i{}; i < mesh.indices.size() / 3; ++i)
			{
				const Vertex v0 = mesh.vertices[mesh.indices[i * 3]];
				const Vertex v1 = mesh.vertices[mesh.indices[i * 3 + 1]];
				const Vertex v2 = mesh.vertices[mesh.indices[i * 3 + 2]];

				RenderTriWithCurrTexturePtr(v0, v1, v2);
			}
		}
		else
		{
			for (int i{}; i < mesh.indices.size() - 2; ++i)
			{
				//slide 9
				if (i % 2 != 0)
				{
					const Vertex v0 = mesh.vertices[mesh.indices[i]];
					const Vertex v1 = mesh.vertices[mesh.indices[i + 2]];
					const Vertex v2 = mesh.vertices[mesh.indices[i + 1]];
					RenderTriWithCurrTexturePtr(v0, v1, v2);
				}
				else
				{
					const Vertex v0 = mesh.vertices[mesh.indices[i]];
					const Vertex v1 = mesh.vertices[mesh.indices[i + 1]];
					const Vertex v2 = mesh.vertices[mesh.indices[i + 2]];
					RenderTriWithCurrTexturePtr(v0, v1, v2);
				}
			}
		}
	}
}

void dae::Renderer::WeekThree()
{
	//Define Triangle _ Vertices in world space
	std::vector<Mesh> meshes_ss{
		Mesh{
			{
			Vertex{Vector3(-3.f, 3.f, -2.f), colors::White, Vector2(0,0)},
			Vertex{Vector3(0.f, 3.f, -2.f), colors::White, Vector2(.5f,0)},
			Vertex{Vector3(3.f, 3.f, -2.f), colors::White, Vector2(1,0)},
			Vertex{Vector3(-3.f, 0.f, -2.f), colors::White, Vector2(0,.5f)},
			Vertex{Vector3(0.f, 0.f, -2.f), colors::White, Vector2(.5f, .5f)},
			Vertex{Vector3(3.f, 0.f, -2.f), colors::White, Vector2(1, .5f)},
			Vertex{Vector3(-3.f, -3.f, -2.f), colors::White, Vector2(0, 1)},
			Vertex{Vector3(0.f, -3.f, -2.f), colors::White, Vector2(.5f, 1)},
			Vertex{Vector3(3.f, -3.f, -2.f), colors::White, Vector2(1,1)}
			},{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},
			PrimitiveTopology::TriangleList
		}
	};


	//convert to screen space //week 2
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, 100);

	ColorRGB clearColor = ColorRGB{ 100,100,100 };
	Uint32 clearColorUint = 0xFF000000 | (Uint32)clearColor.r | (Uint32)clearColor.g << 8 | (Uint32)clearColor.b << 16;
	SDL_FillRect(m_pBackBuffer, NULL, clearColorUint);

	//RENDER LOGIC

	for (auto& mesh : meshes_ss)
	{
		VertexTransformationFunctionImproved(mesh.vertices, mesh.vertices_out, mesh.worldMatrix);
		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
		{
			for (int i{}; i < mesh.indices.size() / 3; ++i)
			{
				Vertex_Out v0 = mesh.vertices_out[mesh.indices[i * 3]];
				Vertex_Out v1 = mesh.vertices_out[mesh.indices[i * 3 + 1]];
				Vertex_Out v2 = mesh.vertices_out[mesh.indices[i * 3 + 2]];

				//check if within boundaries: //todo: is it normal to only check x and y?
				//fustum culling
				if (isOutsideFrustum(v0)) continue;
				if (isOutsideFrustum(v1)) continue;
				if (isOutsideFrustum(v2)) continue;
				
				//old [-1,1]
				/*if ((v0.position.x < -1 || v0.position.x > 1) || (v0.position.y < -1 || v0.position.y > 1)) continue;
				else if ((v1.position.x < -1 || v1.position.x > 1) || (v1.position.y < -1 || v1.position.y > 1)) continue;
				else if ((v2.position.x < -1 || v2.position.x > 1) || (v2.position.y < -1 || v2.position.y > 1)) continue;*/
				//todo: do i check z? should be 0 - 1, slide 11

				//slide 2 week 8, NDC to raster space
				
				//NDC to raster space
				VertexNDCToRaster(v0);
				VertexNDCToRaster(v1);
				VertexNDCToRaster(v2);

				RenderTriWeekThree(v0, v1, v2);
			}
		}
		else
		{
			for (int i{}; i < mesh.indices.size() - 2; ++i)
			{
				//slide 9
				if (i % 2 != 0)
				{
					Vertex_Out v0 = mesh.vertices_out[mesh.indices[i * 3]];
					Vertex_Out v1 = mesh.vertices_out[mesh.indices[i * 3 + 2]];
					Vertex_Out v2 = mesh.vertices_out[mesh.indices[i * 3 + 1]];

					if (isOutsideFrustum(v0)) continue;
					if (isOutsideFrustum(v1)) continue;
					if (isOutsideFrustum(v2)) continue;




					RenderTriWeekThree(v0, v1, v2);
				}
				else
				{
					Vertex_Out v0 = mesh.vertices_out[mesh.indices[i * 3]];
					Vertex_Out v1 = mesh.vertices_out[mesh.indices[i * 3 + 1]];
					Vertex_Out v2 = mesh.vertices_out[mesh.indices[i * 3 + 2]];

					if (isOutsideFrustum(v0)) continue;
					if (isOutsideFrustum(v1)) continue;
					if (isOutsideFrustum(v2)) continue;


					RenderTriWeekThree(v0, v1, v2);
				}
			}
		}
	}


}

void dae::Renderer::VertexNDCToRaster(Vertex_Out& vertex)
{
	vertex.position.x = (vertex.position.x + 1) / 2.f * m_Width;
	vertex.position.y = (1 - vertex.position.y) / 2.f * m_Height;

}

void dae::Renderer::InitMesh()
{
	//Parse object to m_Mesh
	Utils::ParseOBJ("Resources/tuktuk.obj", m_Mesh.vertices, m_Mesh.indices); //W3

	const Vector3 position{ Vector3{0.f, 0.f, 50.f} };
	const Vector3 rotation{ };
	const Vector3 scale{ Vector3{ 1.f, 1.f, 1.f } };
	m_Mesh.worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);
	m_Mesh.primitiveTopology = PrimitiveTopology::TriangleList;
}

bool dae::Renderer::isOutsideFrustum(const Vertex_Out& vertex) const
{
	return 

		//x.
		((	   vertex.position.x < -1
			|| vertex.position.x > 1)
			//y.
			|| (vertex.position.y < -1
			|| vertex.position.y > 1));
		////x.
		//((vertex.position.x < m_Camera.nearPlane 
		//   || vertex.position.x > m_Camera.farPlane) 
		////y.
		//   || (vertex.position.y < m_Camera.nearPlane 
		//   || vertex.position.y > m_Camera.farPlane));
}

bool dae::Renderer::isInFrustum(const Vertex_Out& vertex) const
{
	return !isOutsideFrustum(vertex);
}

void dae::Renderer::ChangeRenderMode()
{	
	switch (m_RenderMode)
	{
	case dae::Renderer::RenderMode::FinalColor:
		m_RenderMode = RenderMode::DepthBuffer;
		break;
	case dae::Renderer::RenderMode::DepthBuffer:
		m_RenderMode = RenderMode::FinalColor;
		break;
	}

}

float dae::Renderer::Remap(float valueToRemap, float min, float max) const
{
	const float clampedValue{ Clamp(valueToRemap, min, max) };
	return ((clampedValue - min) / (max - min));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}


