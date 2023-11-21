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
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
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

	//WeekOneRasterizationOnly();
	//WeekOneProjectionStage();
	//WeekOneBaryCentricCoordinates();
	//WeekOneDepthBuffer();
	WeekOneBBX();


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

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}


