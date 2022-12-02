//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"
#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
	, m_Vertices{}
	, m_Indices{}
	, m_RotationMatrix{}
	, m_Shadingmode{ ShadingMode::Combined }
	, m_IsNormalMapEnabled{true}
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,5.0f,-30.f }, (float)m_Width / (float)m_Height);

	m_pTexture = Texture::LoadFromFile("Resources/tuktuk.png");

	/*Mesh tuktuk{};

	bool loadedMesh{ Utils::ParseOBJ("Resources/tuktuk.obj", m_Vertices, m_Indices) };

	tuktuk.vertices = m_Vertices;
	tuktuk.indices = m_Indices;

	tuktuk.primitiveTopology = PrimitiveTopology::TriangleList;

	m_Meshes.push_back(tuktuk);*/

	Mesh vehicle{};

	Utils::ParseOBJ("Resources/vehicle.obj", m_Vertices, m_Indices);

	vehicle.vertices = m_Vertices;
	vehicle.indices = m_Indices;
	vehicle.primitiveTopology = PrimitiveTopology::TriangleList;

	//Matrices for mesh worldMatrix
	Matrix scaleMatrix{ Matrix::CreateScale({1,1,1}) };
	Matrix rotateMatrix{ Matrix::CreateRotationY(90.f * TO_RADIANS) };
	Matrix translateMatrix{ Matrix::CreateTranslation({0,0,50}) };

	vehicle.worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

	m_Meshes.push_back(vehicle);

	m_pVehicleDiffuse = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pVehicleNormal = Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pVehicleGlossy = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pVehicleSpecular = Texture::LoadFromFile("Resources/vehicle_specular.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;

	if (m_pTexture)
		delete m_pTexture;

	if (m_pVehicleDiffuse)
		delete m_pVehicleDiffuse;
	if (m_pVehicleNormal)
		delete m_pVehicleNormal;
	if (m_pVehicleGlossy)
		delete m_pVehicleGlossy;
	if (m_pVehicleSpecular)
		delete m_pVehicleSpecular;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	const float rotationSpeed{ 25.f };
	m_RotationMatrix = Matrix::CreateRotationY((rotationSpeed * pTimer->GetElapsed()) * TO_RADIANS);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Render_W1_Gradient();
	//Render_W1_Part1();
	//Render_W1_Part2();
	//Render_W1_Part3();
	//Render_W1_Part4();
	//Render_W1_Part5();

	//Render_W2_TriangleList();
	//Render_W2_TriangleStrip();
	//Render_W2_Textures();
	//Render_W2_DepthInterpolation();

	//Render_W3_Projection();
	//Render_W3_Tuktuk();

	Render_W3_Vehicle();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::Render_W1_Gradient()
{
	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;

			ColorRGB finalColor{ gradient, gradient, gradient };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::Render_W1_Part1()
{
	//Rasterization stage

	std::vector<Vector3> vertices_ndc{
		{0.0f, 0.5f, 1.0f},	//vertex 0
		{0.5f, -0.5f, 1.0f},	//vertex 1
		{-0.5f, -0.5f, 1.0f}	//vertex 2
	};

	std::vector<Vector3> verticesScreenSpace{};
	for (auto& vector : vertices_ndc)
	{
		Vector3 newVector{};
		newVector.x = ((vector.x + 1) / 2) * m_Width;
		newVector.y = ((1 - vector.y) / 2) * m_Height;
		newVector.z = vector.z;

		verticesScreenSpace.push_back(newVector);
	}

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			finalColor = { 0.f,0.f,0.f };

			if (IsPointInTriangle(verticesScreenSpace, px, py))
			{
				finalColor = { 1.f,1.f,1.f };
			}

			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::Render_W1_Part2()
{
	//Projection stage
	std::vector<Vertex> vertices_world{
		{{0.0f, 2.0f, 0.0f}},	//vertex 0
		{{1.0f, 0.0f, 0.0f} },	//vertex 1
		{{-1.0f, 0.0f, 0.0f}}	//vertex 2
	};

	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	VertexTransformationFunction(vertices_world, vertices_raster);

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			finalColor = { 0.f,0.f,0.f };

			if (IsPointInTriangle(vertices_raster, px, py))
			{
				finalColor = { 1,1,1 };
			}
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::Render_W1_Part3()
{
	//Barycentric coordinates

	//Traingle in world space
	std::vector<Vertex> vertices_world
	{
		{{0.0f, 4.0f, 2.0f}, {1, 0, 0}},
		{{3.0f, -2.0f, 2.0f}, {0, 1, 0}},
		{{-3.0f, -2.0f, 2.0f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_raster);

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			finalColor = { 0.f,0.f,0.f };

			Vector2 pixel{ (float)px, (float)py };

			Vector2 v0{ vertices_raster[0].position.x, vertices_raster[0].position.y };
			Vector2 v1{ vertices_raster[1].position.x, vertices_raster[1].position.y };
			Vector2 v2{ vertices_raster[2].position.x, vertices_raster[2].position.y };

			float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

			float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
			float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
			float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

			Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };

			if (IsPointInTriangle(vertices_raster, pInsideTriangle))
			{
				finalColor = { vertices_raster[0].color * w0 + vertices_raster[1].color * w1 + vertices_raster[2].color * w2 };
			}

			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::Render_W1_Part4()
{
	//Depth buffer

	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{{0.0f, 2.0f, 0.0f}, {1,0,0}},
		{{1.5f, -1.0f, 0.0f}, {1,0,0}},
		{{-1.5f, -1.0f, 0.0f}, {1,0,0}},

		//Triangle 1
		{{0.0f, 4.0f, 2.0f}, {1,0,0}},
		{{3.0f, -2.0f, 2.0f}, {0,1,0}},
		{{-3.0f, -2.0f, 2.0f}, {0,0,1}}
	};

	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_raster);

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//RENDER LOGIC
	for (int i = 0; i < vertices_raster.size(); i += 3)
	{
		std::vector<Vertex> triangle{
				vertices_raster[i],
				vertices_raster[i + 1],
				vertices_raster[i + 2]
		};

		const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
		const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
		const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

		float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

		for (int py{}; py < m_Height; ++py)
		{
			for (int px{}; px < m_Width; ++px)
			{
				//finalColor = { 0.f,0.f,0.f };

				const Vector2 pixel{ (float)px, (float)py };

				const float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
				const float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
				const float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

				const Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };


				if (IsPointInTriangle(triangle, pInsideTriangle))
				{
					const float depthV0{ triangle[0].position.z };
					const float depthV1{ triangle[1].position.z };
					const float depthV2{ triangle[2].position.z };

					const float z = depthV0 * w0 + depthV1 * w1 + depthV2 * w2;
					const int pixelZIndex = py * m_Width + px;

					if (z < m_pDepthBufferPixels[pixelZIndex])
					{
						m_pDepthBufferPixels[pixelZIndex] = z;
						finalColor = { triangle[0].color * w0 + triangle[1].color * w1 + triangle[2].color * w2 };

						//Update Color in Buffer
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

void dae::Renderer::Render_W1_Part5()
{
	//Boundingbox optimization

	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{{0.0f, 2.0f, 5.0f}, {1,0,0}},
		{{1.5f, -1.0f, 0.0f}, {1,0,0}},
		{{-1.5f, -1.0f, 0.0f}, {1,0,0}},

		//Triangle 1
		{{0.0f, 4.0f, 2.0f}, {1,0,0}},
		{{3.0f, -2.0f, 2.0f}, {0,1,0}},
		{{-3.0f, -2.0f, 2.0f}, {0,0,1}}
	};

	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_raster);

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//RENDER LOGIC
	for (int i = 0; i < vertices_raster.size(); i += 3)
	{
		std::vector<Vertex> triangle{
				vertices_raster[i],
				vertices_raster[i + 1],
				vertices_raster[i + 2]
		};

		const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
		const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
		const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

		float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

		int smallestX{ (int)v0.x };
		smallestX = std::min(smallestX, (int)v1.x);
		smallestX = std::min(smallestX, (int)v2.x);

		int smallestY{ (int)v0.y };
		smallestY = std::min(smallestY, (int)v1.y);
		smallestY = std::min(smallestY, (int)v2.y);

		Vector2 smallestPoint{ (float)smallestX, (float)smallestY };

		if (smallestPoint.x < 0)
		{
			smallestPoint.x = 0;
		}
		else if (smallestPoint.x > m_Width)
		{
			smallestPoint.x = m_Width;
		}

		if (smallestPoint.y < 0)
		{
			smallestPoint.y = 0;
		}
		else if (smallestPoint.y > m_Height)
		{
			smallestPoint.y = m_Height;
		}

		int biggestX{ (int)v0.x };
		biggestX = std::max(biggestX, (int)v1.x);
		biggestX = std::max(biggestX, (int)v2.x);

		int biggestY{ (int)v0.x };
		biggestY = std::max(biggestY, (int)v1.y);
		biggestY = std::max(biggestY, (int)v2.y);

		Vector2 biggestPoint{ (float)biggestX, (float)biggestY };
		if (biggestPoint.x < 0)
		{
			biggestPoint.x = 0;
		}
		else if (biggestPoint.x > m_Width)
		{
			biggestPoint.x = m_Width;
		}

		if (biggestPoint.y < 0)
		{
			biggestPoint.y = 0;
		}
		else if (biggestPoint.y > m_Height)
		{
			biggestPoint.y = m_Height;
		}


		for (int py{ (int)smallestPoint.y }; py < (int)biggestPoint.y; ++py)
		{
			for (int px{ (int)smallestPoint.x }; px < (int)biggestPoint.x; ++px)
			{
				//finalColor = { 0.f,0.f,0.f };

				const Vector2 pixel{ (float)px, (float)py };

				const float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
				const float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
				const float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

				const Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };


				if (IsPointInTriangle(triangle, pInsideTriangle))
				{
					const float depthV0{ triangle[0].position.z };
					const float depthV1{ triangle[1].position.z };
					const float depthV2{ triangle[2].position.z };

					const float z = depthV0 * w0 + depthV1 * w1 + depthV2 * w2;
					const int pixelZIndex = py * m_Width + px;

					if (z < m_pDepthBufferPixels[pixelZIndex])
					{
						const float area{ w0 + w1 + w2 };

						m_pDepthBufferPixels[pixelZIndex] = z;
						finalColor = { triangle[0].color * w0 + triangle[1].color * w1 + triangle[2].color * w2 };

						//Update Color in Buffer
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

void dae::Renderer::Render_W2_TriangleStrip()
{
	//TriangleStrip

	std::vector<Mesh> meshes_World
	{
		Mesh{
			{
				Vertex{{-3,3,-2}},
				Vertex{{0,3,-2}},
				Vertex{{3,3,-2}},
				Vertex{{-3,0,-2}},
				Vertex{{0,0,-2}},
				Vertex{{3,0,-2}},
				Vertex{{-3,-3,-2}},
				Vertex{{0,-3,-2}},
				Vertex{{3,-3,-2}}
			},
		{
			3,0,4,1,5,2,
			2,6,
			6,3,7,4,8,5
		},
		PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_World);

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//RENDER LOGIC
	for (Mesh& mesh : meshes_World)
	{
		//mesh.indices
		for (int i = 0; i < mesh.indices.size() - 2; ++i)
		{
			std::vector<Vertex_Out> triangle{
					mesh.vertices_out[mesh.indices[i]],
					mesh.vertices_out[mesh.indices[i + 1]],
					mesh.vertices_out[mesh.indices[i + 2]]
			};

			if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				if (mesh.indices[i] == mesh.indices[i + 1] ||
					mesh.indices[i] == mesh.indices[i + 2] ||
					mesh.indices[i + 1] == mesh.indices[i + 2])
				{
					continue;
				}

				if (i % 2 != 0)
				{
					//Odd triangle
					auto tempIdx1 = triangle[1];
					triangle[1] = triangle[2];
					triangle[2] = tempIdx1;
				}
			}

			Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
			Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
			Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

			float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

			int smallestX{ (int)v0.x };
			smallestX = std::min(smallestX, (int)v1.x);
			smallestX = std::min(smallestX, (int)v2.x);

			int smallestY{ (int)v0.y };
			smallestY = std::min(smallestY, (int)v1.y);
			smallestY = std::min(smallestY, (int)v2.y);

			if (smallestX < 0)
			{
				smallestX = 0;
			}
			else if (smallestX > m_Width - 1)
			{
				smallestX = m_Width - 1;
			}

			if (smallestY < 0)
			{
				smallestY = 0;
			}
			else if (smallestY > m_Height - 1)
			{
				smallestY = m_Height - 1;
			}

			int biggestX{ (int)v0.x };
			biggestX = std::max(biggestX, (int)v1.x);
			biggestX = std::max(biggestX, (int)v2.x);

			int biggestY{ (int)v0.y };
			biggestY = std::max(biggestY, (int)v1.y);
			biggestY = std::max(biggestY, (int)v2.y);

			if (biggestX < 0)
			{
				biggestX = 0;
			}
			else if (biggestX > m_Width - 1)
			{
				biggestX = m_Width - 1;
			}

			if (biggestY < 0)
			{
				biggestY = 0;
			}
			else if (biggestY > m_Height - 1)
			{
				biggestY = m_Height - 1;
			}


			for (int py{ smallestY }; py < biggestY; ++py)
			{
				for (int px{ smallestX }; px < biggestX; ++px)
				{
					/*m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,255,0,0);

					continue;*/

					//finalColor = { 0.f,0.f,0.f };

					const Vector2 pixel{ (float)px, (float)py };

					const float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
					const float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
					const float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

					const Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };


					if (IsPointInTriangle(triangle, pInsideTriangle))
					{
						const float depthV0{ triangle[0].position.z };
						const float depthV1{ triangle[1].position.z };
						const float depthV2{ triangle[2].position.z };

						const float z = depthV0 * w0 + depthV1 * w1 + depthV2 * w2;
						const int pixelZIndex = py * m_Width + px;

						if (z < m_pDepthBufferPixels[pixelZIndex])
						{
							const float area{ w0 + w1 + w2 };

							m_pDepthBufferPixels[pixelZIndex] = z;
							finalColor = { triangle[0].color * w0 + triangle[1].color * w1 + triangle[2].color * w2 };

							//Update Color in Buffer
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
}

void dae::Renderer::Render_W2_TriangleList()
{
	//TriangleList

	std::vector<Mesh> meshes_World
	{
		Mesh{
			{
				Vertex{{-3,3,-2}},
				Vertex{{0,3,-2}},
				Vertex{{3,3,-2}},
				Vertex{{-3,0,-2}},
				Vertex{{0,0,-2}},
				Vertex{{3,0,-2}},
				Vertex{{-3,-3,-2}},
				Vertex{{0,-3,-2}},
				Vertex{{3,-3,-2}}
			},
		{
			3, 0, 1,		1, 4, 3,		4, 1, 2,
			2, 5, 4,		6, 3, 4,		4, 7, 6,
			7, 4, 5,		5, 8, 7
		},
		PrimitiveTopology::TriangleList
		}
	};

	VertexTransformationFunction(meshes_World);

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//RENDER LOGIC
	for (Mesh& mesh : meshes_World)
	{
		//mesh.indices
		for (int i = 0; i < mesh.indices.size(); i += 3)
		{
			std::vector<Vertex_Out> triangle{
					mesh.vertices_out[mesh.indices[i]],
					mesh.vertices_out[mesh.indices[i + 1]],
					mesh.vertices_out[mesh.indices[i + 2]]
			};

			const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
			const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
			const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

			float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

			int smallestX{ (int)v0.x };
			smallestX = std::min(smallestX, (int)v1.x);
			smallestX = std::min(smallestX, (int)v2.x);

			int smallestY{ (int)v0.y };
			smallestY = std::min(smallestY, (int)v1.y);
			smallestY = std::min(smallestY, (int)v2.y);

			if (smallestX < 0)
			{
				smallestX = 0;
			}
			else if (smallestX > m_Width - 1)
			{
				smallestX = m_Width - 1;
			}

			if (smallestY < 0)
			{
				smallestY = 0;
			}
			else if (smallestY > m_Height - 1)
			{
				smallestY = m_Height - 1;
			}

			int biggestX{ (int)v0.x };
			biggestX = std::max(biggestX, (int)v1.x);
			biggestX = std::max(biggestX, (int)v2.x);

			int biggestY{ (int)v0.y };
			biggestY = std::max(biggestY, (int)v1.y);
			biggestY = std::max(biggestY, (int)v2.y);

			if (biggestX < 0)
			{
				biggestX = 0;
			}
			else if (biggestX > m_Width - 1)
			{
				biggestX = m_Width - 1;
			}

			if (biggestY < 0)
			{
				biggestY = 0;
			}
			else if (biggestY > m_Height - 1)
			{
				biggestY = m_Height - 1;
			}


			for (int py{ smallestY }; py < biggestY; ++py)
			{
				for (int px{ smallestX }; px < biggestX; ++px)
				{
					/*m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,255,0,0);

					continue;*/

					//finalColor = { 0.f,0.f,0.f };

					const Vector2 pixel{ (float)px, (float)py };

					const float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
					const float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
					const float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

					const Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };


					if (IsPointInTriangle(triangle, pInsideTriangle))
					{
						const float depthV0{ triangle[0].position.z };
						const float depthV1{ triangle[1].position.z };
						const float depthV2{ triangle[2].position.z };

						const float z = depthV0 * w0 + depthV1 * w1 + depthV2 * w2;
						const int pixelZIndex = py * m_Width + px;

						if (z < m_pDepthBufferPixels[pixelZIndex])
						{
							const float area{ w0 + w1 + w2 };

							m_pDepthBufferPixels[pixelZIndex] = z;
							finalColor = { triangle[0].color * w0 + triangle[1].color * w1 + triangle[2].color * w2 };

							//Update Color in Buffer
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
}

void dae::Renderer::Render_W2_Textures()
{
	//Textures

	std::vector<Mesh> meshes_World
	{
		Mesh{
			{
				Vertex{{-3,3,-2}, colors::White, {0.0f,0.0f}},
				Vertex{{0,3,-2}, colors::White, {0.5f, 0.0f}},
				Vertex{{3,3,-2}, colors::White, {1.0f,0.0f}},
				Vertex{{-3,0,-2}, colors::White, {0.0f, 0.5f}},
				Vertex{{0,0,-2}, colors::White, {0.5f,0.5f}},
				Vertex{{3,0,-2}, colors::White, {1.0f, 0.5f}},
				Vertex{{-3,-3,-2}, colors::White, {0.0f, 1.0f}},
				Vertex{{0,-3,-2}, colors::White, {0.5f, 1.0f}},
				Vertex{{3,-3,-2}, colors::White, {1.0f,1.0f}}
			},
		{
			3,0,4,1,5,2,
			2,6,
			6,3,7,4,8,5
		},
		PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_World);

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	Texture* texture = Texture::LoadFromFile("Resources/uv_grid_2.png");

	//RENDER LOGIC
	for (Mesh& mesh : meshes_World)
	{
		//mesh.indices
		for (int i = 0; i < mesh.indices.size() - 2; ++i)
		{
			std::vector<Vertex_Out> triangle{
					mesh.vertices_out[mesh.indices[i]],
					mesh.vertices_out[mesh.indices[i + 1]],
					mesh.vertices_out[mesh.indices[i + 2]]
			};

			if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				if (mesh.indices[i] == mesh.indices[i + 1] ||
					mesh.indices[i] == mesh.indices[i + 2] ||
					mesh.indices[i + 1] == mesh.indices[i + 2])
				{
					continue;
				}

				if (i % 2 != 0)
				{
					//Odd triangle
					auto tempIdx1 = triangle[1];
					triangle[1] = triangle[2];
					triangle[2] = tempIdx1;
				}
			}

			Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
			Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
			Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

			float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

			int smallestX{ (int)v0.x };
			smallestX = std::min(smallestX, (int)v1.x);
			smallestX = std::min(smallestX, (int)v2.x);

			int smallestY{ (int)v0.y };
			smallestY = std::min(smallestY, (int)v1.y);
			smallestY = std::min(smallestY, (int)v2.y);

			if (smallestX < 0)
			{
				smallestX = 0;
			}
			else if (smallestX > m_Width - 1)
			{
				smallestX = m_Width - 1;
			}

			if (smallestY < 0)
			{
				smallestY = 0;
			}
			else if (smallestY > m_Height - 1)
			{
				smallestY = m_Height - 1;
			}

			int biggestX{ (int)v0.x };
			biggestX = std::max(biggestX, (int)v1.x);
			biggestX = std::max(biggestX, (int)v2.x);

			int biggestY{ (int)v0.y };
			biggestY = std::max(biggestY, (int)v1.y);
			biggestY = std::max(biggestY, (int)v2.y);

			if (biggestX < 0)
			{
				biggestX = 0;
			}
			else if (biggestX > m_Width - 1)
			{
				biggestX = m_Width - 1;
			}

			if (biggestY < 0)
			{
				biggestY = 0;
			}
			else if (biggestY > m_Height - 1)
			{
				biggestY = m_Height - 1;
			}


			for (int py{ smallestY }; py < biggestY; ++py)
			{
				for (int px{ smallestX }; px < biggestX; ++px)
				{
					/*m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,255,0,0);

					continue;*/

					//finalColor = { 0.f,0.f,0.f };

					const Vector2 pixel{ (float)px, (float)py };

					const float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
					const float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
					const float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

					const Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };


					if (IsPointInTriangle(triangle, pInsideTriangle))
					{
						const float depthV0{ triangle[0].position.z };
						const float depthV1{ triangle[1].position.z };
						const float depthV2{ triangle[2].position.z };

						const float z = depthV0 * w0 + depthV1 * w1 + depthV2 * w2;
						const int pixelZIndex = py * m_Width + px;

						if (z < m_pDepthBufferPixels[pixelZIndex])
						{
							const float area{ w0 + w1 + w2 };

							m_pDepthBufferPixels[pixelZIndex] = z;
							//finalColor = { triangle[0].color * w0 + triangle[1].color * w1 + triangle[2].color * w2 };
							Vector2 interpolatedUv = triangle[0].uv * w0 + triangle[1].uv * w1 + triangle[2].uv * w2;

							finalColor = texture->Sample(interpolatedUv);

							//Update Color in Buffer
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
}

void dae::Renderer::Render_W2_DepthInterpolation()
{
	//Depth Interpolation

	std::vector<Mesh> meshes_World
	{
		Mesh{
			{
				Vertex{{-3,3,-2}, colors::White, {0.0f,0.0f}},
				Vertex{{0,3,-2}, colors::White, {0.5f, 0.0f}},
				Vertex{{3,3,-2}, colors::White, {1.0f,0.0f}},
				Vertex{{-3,0,-2}, colors::White, {0.0f, 0.5f}},
				Vertex{{0,0,-2}, colors::White, {0.5f,0.5f}},
				Vertex{{3,0,-2}, colors::White, {1.0f, 0.5f}},
				Vertex{{-3,-3,-2}, colors::White, {0.0f, 1.0f}},
				Vertex{{0,-3,-2}, colors::White, {0.5f, 1.0f}},
				Vertex{{3,-3,-2}, colors::White, {1.0f,1.0f}}
			},
		{
			3,0,4,1,5,2,
			2,6,
			6,3,7,4,8,5
		},
		PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_World);

	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//RENDER LOGIC
	for (Mesh& mesh : meshes_World)
	{
		//mesh.indices
		for (int i = 0; i < mesh.indices.size() - 2; ++i)
		{
			std::vector<Vertex_Out> triangle{
					mesh.vertices_out[mesh.indices[i]],
					mesh.vertices_out[mesh.indices[i + 1]],
					mesh.vertices_out[mesh.indices[i + 2]]
			};

			if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				if (mesh.indices[i] == mesh.indices[i + 1] ||
					mesh.indices[i] == mesh.indices[i + 2] ||
					mesh.indices[i + 1] == mesh.indices[i + 2])
				{
					continue;
				}

				if (i % 2 != 0)
				{
					//Odd triangle
					auto tempIdx1 = triangle[1];
					triangle[1] = triangle[2];
					triangle[2] = tempIdx1;
				}
			}

			Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
			Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
			Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

			float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

			int smallestX = static_cast<int>(std::min(v0.x, std::min(v1.x, v2.x)));
			int smallestY = static_cast<int>(std::min(v0.y, std::min(v1.y, v2.y)));

			smallestX = Clamp(smallestX, 0, m_Width);
			smallestY = Clamp(smallestY, 0, m_Height);

			int biggestX = static_cast<int>(std::max(v0.x, std::max(v1.x, v2.x)));
			int biggestY = static_cast<int>(std::max(v0.y, std::max(v1.y, v2.y)));

			biggestX = Clamp(biggestX, 0, m_Width);
			biggestY = Clamp(biggestY, 0, m_Height);


			for (int py{ smallestY }; py < biggestY; ++py)
			{
				for (int px{ smallestX }; px < biggestX; ++px)
				{

					const Vector2 pixel{ (float)px, (float)py };

					const float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
					const float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
					const float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

					const Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };


					if (IsPointInTriangle(triangle, pInsideTriangle))
					{
						const float depthV0{ triangle[0].position.z };
						const float depthV1{ triangle[1].position.z };
						const float depthV2{ triangle[2].position.z };

						const float z = depthV0 * w0 + depthV1 * w1 + depthV2 * w2;

						const int pixelZIndex = py * m_Width + px;

						if (z < m_pDepthBufferPixels[pixelZIndex])
						{
							const float area{ w0 + w1 + w2 };

							const float v0Calculation{ (1 / depthV0) * w0 };
							const float v1Calculation{ (1 / depthV1) * w1 };
							const float v2Calculation{ (1 / depthV2) * w2 };
							const float zInterpolated{ 1 / (v0Calculation + v1Calculation + v2Calculation) };

							m_pDepthBufferPixels[pixelZIndex] = z;
							//finalColor = { triangle[0].color * w0 + triangle[1].color * w1 + triangle[2].color * w2 };
							//Vector2 interpolatedUv = triangle[0].uv * w0 + triangle[1].uv * w1 + triangle[2].uv * w2;
							auto uv0 = (triangle[0].uv / depthV0) * w0;
							auto uv1 = (triangle[1].uv / depthV1) * w1;
							auto uv2 = (triangle[2].uv / depthV2) * w2;
							Vector2 interpolatedUv{ uv0 + uv1 + uv2 };

							interpolatedUv *= zInterpolated;

							finalColor = m_pTexture->Sample(interpolatedUv);

							//Update Color in Buffer
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
}

void dae::Renderer::Render_W3_Projection()
{
	//Projection matrix + depth buffer

	std::vector<Mesh> meshes_World
	{
		Mesh{
			{
				Vertex{{-3,3,-2}, colors::White, {0.0f,0.0f}},
				Vertex{{0,3,-2}, colors::White, {0.5f, 0.0f}},
				Vertex{{3,3,-2}, colors::White, {1.0f,0.0f}},
				Vertex{{-3,0,-2}, colors::White, {0.0f, 0.5f}},
				Vertex{{0,0,-2}, colors::White, {0.5f,0.5f}},
				Vertex{{3,0,-2}, colors::White, {1.0f, 0.5f}},
				Vertex{{-3,-3,-2}, colors::White, {0.0f, 1.0f}},
				Vertex{{0,-3,-2}, colors::White, {0.5f, 1.0f}},
				Vertex{{3,-3,-2}, colors::White, {1.0f,1.0f}}
			},
		{
			3,0,4,1,5,2,
			2,6,
			6,3,7,4,8,5
		},
		PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_World);


	ColorRGB finalColor{ 0.f, 0.f, 0.f };

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//RENDER LOGIC
	for (Mesh& mesh : meshes_World)
	{
		for (int i = 0; i < mesh.indices.size() - 2; ++i)
		{
			std::vector<Vertex_Out> triangle{
					mesh.vertices_out[mesh.indices[i]],
					mesh.vertices_out[mesh.indices[i + 1]],
					mesh.vertices_out[mesh.indices[i + 2]]
			};

			if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				if (mesh.indices[i] == mesh.indices[i + 1] ||
					mesh.indices[i] == mesh.indices[i + 2] ||
					mesh.indices[i + 1] == mesh.indices[i + 2])
				{
					continue;
				}

				if (i % 2 != 0)
				{
					//Odd triangle
					auto tempIdx1 = triangle[1];
					triangle[1] = triangle[2];
					triangle[2] = tempIdx1;
				}
			}

			//get the indices of the triangle
			Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
			Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
			Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

			//frustum check
			if (v0.x < -1.0f || v0.x > 1.0f ||
				v1.x < -1.0f || v1.x > 1.0f ||
				v2.x < -1.0f || v2.x > 1.0f)
			{
				continue;
			}
			if (v0.y < -1.0f || v0.y > 1.0f ||
				v1.y < -1.0f || v1.y > 1.0f ||
				v2.y < -1.0f || v2.y > 1.0f)
			{
				continue;
			}

			//convert xy coordinates into raster space
			for (auto& vertex : triangle)
			{
				vertex.position.x = ((vertex.position.x + 1) / 2) * (float)m_Width;
				vertex.position.y = ((1 - vertex.position.y) / 2) * (float)m_Height;
			}

			//get the raster space vertices of the triangle
			v0 = { triangle[0].position.x, triangle[0].position.y };
			v1 = { triangle[1].position.x, triangle[1].position.y };
			v2 = { triangle[2].position.x, triangle[2].position.y };

			float areaOfParallelogram{ Vector2::Cross((v1 - v0), (v2 - v0)) };

			int smallestX = static_cast<int>(std::min(v0.x, std::min(v1.x, v2.x)));
			int smallestY = static_cast<int>(std::min(v0.y, std::min(v1.y, v2.y)));

			smallestX = Clamp(smallestX, 0, m_Width);
			smallestY = Clamp(smallestY, 0, m_Height);

			int biggestX = static_cast<int>(std::max(v0.x, std::max(v1.x, v2.x)));
			int biggestY = static_cast<int>(std::max(v0.y, std::max(v1.y, v2.y)));

			biggestX = Clamp(biggestX, 0, m_Width);
			biggestY = Clamp(biggestY, 0, m_Height);


			for (int py{ smallestY }; py < biggestY; ++py)
			{
				for (int px{ smallestX }; px < biggestX; ++px)
				{

					const Vector2 pixel{ (float)px, (float)py };

					const float w0{ Vector2::Cross((v2 - v1), (pixel - v1)) / areaOfParallelogram };
					const float w1{ Vector2::Cross((v0 - v2), (pixel - v2)) / areaOfParallelogram };
					const float w2{ Vector2::Cross((v1 - v0), (pixel - v0)) / areaOfParallelogram };

					const Vector2 pInsideTriangle{ w0 * v0 + w1 * v1 + w2 * v2 };

					if (IsPointInTriangle(triangle, pInsideTriangle))
					{
						const float depthV0{ triangle[0].position.z };
						const float depthV1{ triangle[1].position.z };
						const float depthV2{ triangle[2].position.z };

						const float z = depthV0 * w0 + depthV1 * w1 + depthV2 * w2;

						const int pixelZIndex = py * m_Width + px;

						const float v0Calculation{ (1 / depthV0) * w0 };
						const float v1Calculation{ (1 / depthV1) * w1 };
						const float v2Calculation{ (1 / depthV2) * w2 };
						const float zInterpolated{ 1 / (v0Calculation + v1Calculation + v2Calculation) };

						//is the interpolated value in the range [0,1]
						if (zInterpolated > 0 && zInterpolated < 1)
						{
							//is this value closer than the stored value
							if (z < m_pDepthBufferPixels[pixelZIndex])
							{
								m_pDepthBufferPixels[pixelZIndex] = z;

								const float depthV0W{ triangle[0].position.w };
								const float depthV1W{ triangle[1].position.w };
								const float depthV2W{ triangle[2].position.w };

								const float v0CalculationW{ (1 / depthV0W) * w0 };
								const float v1CalculationW{ (1 / depthV1W) * w1 };
								const float v2CalculationW{ (1 / depthV2W) * w2 };
								const float wInterpolated{ 1 / (v0CalculationW + v1CalculationW + v2CalculationW) };

								const float w = depthV0W * w0 + depthV1W * w1 + depthV2W * w2;

								auto uv0 = (triangle[0].uv / depthV0W) * w0;
								auto uv1 = (triangle[1].uv / depthV1W) * w1;
								auto uv2 = (triangle[2].uv / depthV2W) * w2;
								Vector2 interpolatedUv{ uv0 + uv1 + uv2 };

								interpolatedUv *= wInterpolated;

								finalColor = m_pTexture->Sample(interpolatedUv);
								//Update Color in Buffer
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
	}
}

void dae::Renderer::Render_W3_Tuktuk()
{
	//Projection matrix + depth buffer

	VertexTransformationFunction(m_Meshes);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	for (Mesh& mesh : m_Meshes)
	{
		int size = 0;
		std::vector<Vertex_Out> transformedVertices{ mesh.vertices_out };

		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
		{
			size = (int)m_Indices.size();
		}
		else if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			size = (int)m_Indices.size() - 2;
		}

		for (int i = 0; i < size;)
		{
			int evenIndex{ 0 };
			if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				evenIndex = i % 2;
			}
			int index0{ (int)m_Indices[i] };
			int index1{ (int)m_Indices[i + 1 + evenIndex] };
			int index2{ (int)m_Indices[i + 2 - evenIndex] };

			//Increase i based on primitiveTopology
			if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
			{
				i += 3;
			}
			else if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				++i;
			}

			//Calculate the points of the triangle
			Vector4 v0{ transformedVertices[index0].position };
			Vector4 v1{ transformedVertices[index1].position };
			Vector4 v2{ transformedVertices[index2].position };

			//Frustum Culling
			if (v0.x < -1.0f || v0.x > 1.0f || v0.y < -1.0f || v0.y > 1.0f || v0.z < 0.0f || v0.z > 1.0f ||
				v1.x < -1.0f || v1.x > 1.0f || v1.y < -1.0f || v1.y > 1.0f || v1.z < 0.0f || v1.z > 1.0f ||
				v2.x < -1.0f || v2.x > 1.0f || v2.y < -1.0f || v2.y > 1.0f || v2.z < 0.0f || v2.z > 1.0f)
			{
				continue;
			}


			//Pre-calculate value for the depth buffer -> depth buffer will not be linear anymore
			float v0InvDepth{ 1 / v0.w };
			float v1InvDepth{ 1 / v1.w };
			float v2InvDepth{ 1 / v2.w };

			//Convert from NDC to raster space
			//Go from [-1,1] range to [0,1] range, taking screen size into acount
			v0.x = ((v0.x + 1) / 2.0f) * m_Width;
			v0.y = ((1 - v0.y) / 2.0f) * m_Height;

			v1.x = ((v1.x + 1) / 2.0f) * m_Width;
			v1.y = ((1 - v1.y) / 2.0f) * m_Height;

			v2.x = ((v2.x + 1) / 2.0f) * m_Width;
			v2.y = ((1 - v2.y) / 2.0f) * m_Height;


			//Calculate the bounding box
			float xMin = std::min(std::min(v0.x, v1.x), v2.x);
			float xMax = std::max(std::max(v0.x, v1.x), v2.x);

			float yMin = std::min(std::min(v0.y, v1.y), v2.y);
			float yMax = std::max(std::max(v0.y, v1.y), v2.y);

			//Use the min and max values of the bounding box to loop over the pixels
			for (int py{ (int)yMin }; py < yMax; ++py)
			{
				for (int px{ (int)xMin }; px < xMax; ++px)
				{
					ColorRGB finalColor{ 0.f, 0.f, 0.f };

					//Current pixel
					Vector2 pixel{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };

					//Check if the current pixel overlaps the triangle formed by the vertices
					//2D cross product gives a float, based on sign we know if the point is inside the triangle
					Vector2 edge0{ {v1.x - v0.x}, {v1.y - v0.y} };
					Vector2 pointToEdge0{ Vector2{v0.x, v0.y }, pixel };
					float cross0{ Vector2::Cross(edge0, pointToEdge0) };

					Vector2 edge1{ {v2.x - v1.x}, {v2.y - v1.y} };
					Vector2 pointToEdge1{ Vector2{v1.x, v1.y }, pixel };
					float cross1{ Vector2::Cross(edge1, pointToEdge1) };

					Vector2 edge2{ {v0.x - v2.x}, {v0.y - v2.y} };
					Vector2 pointToEdge2{ Vector2{v2.x, v2.y }, pixel };
					float cross2{ Vector2::Cross(edge2, pointToEdge2) };


					if (cross0 > 0.0f && cross1 > 0.0f && cross2 > 0.0f)
					{
						//Calculate the barycentric coordinates
						//2D cross product of V1V0 and V2V0
						float areaOfparallelogram{ Vector2::Cross(edge0, edge1) };

						//Calculate the weights
						float w0{ Vector2::Cross(edge1, pointToEdge1) / areaOfparallelogram };
						float w1{ Vector2::Cross(edge2, pointToEdge2) / areaOfparallelogram };
						float w2{ Vector2::Cross(edge0, pointToEdge0) / areaOfparallelogram };


						if (w0 > 0.0f && w1 > 0.0f && w2 > 0.0f ||
							w0 < 0.0f && w1 < 0.0f && w2 < 0.0f)
						{
							//Do the depth buffer test
							float zBuffer0{ (1.0f / v0.z) * w0 };
							float zBuffer1{ (1.0f / v1.z) * w1 };
							float zBuffer2{ (1.0f / v2.z) * w2 };

							float zBuffer{ zBuffer0 + zBuffer1 + zBuffer2 };
							float invZBuffer{ 1.0f / zBuffer };

							if (invZBuffer < 0.0f || invZBuffer > 1.0f)
							{
								continue;
							}

							if (invZBuffer < m_pDepthBufferPixels[px + (py * m_Width)])
							{
								//Write value of invZbuffer to the depthBuffer
								m_pDepthBufferPixels[px + (py * m_Width)] = invZBuffer;

								//Interpolated the depth value
								float wBuffer0{ (1.0f / v0.w) * w0 };
								float wBuffer1{ (1.0f / v1.w) * w1 };
								float wBuffer2{ (1.0f / v2.w) * w2 };

								float wBuffer{ wBuffer0 + wBuffer1 + wBuffer2 };
								float wInterpolated{ 1.0f / wBuffer };

								//Interpolated uv
								Vector2 interpolatedUV{ transformedVertices[index0].uv * (w0 / v0.w) +
														transformedVertices[index1].uv * (w1 / v1.w) +
														transformedVertices[index2].uv * (w2 / v2.w) };
								interpolatedUV *= wInterpolated;

								//Render the pixel
								finalColor = m_pTexture->Sample(interpolatedUV);
								//finalColor = transformedVertices[index0].color * w0 + transformedVertices[index1].color * w1 + transformedVertices[index2].color * w2;

								//Update Color in Buffer
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
	}
}

void dae::Renderer::Render_W3_Vehicle()
{
	//Transform vertices into raster space (world -> camera -> NDC -> raster)
	VertexTransformationFunction(m_Meshes);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	for (Mesh& mesh : m_Meshes)
	{
		//Change how the for loop advances based on the primitive topology
		int size = 0;
		std::vector<Vertex_Out> transformedVertices{ mesh.vertices_out };

		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
		{
			size = (int)m_Indices.size();
		}
		else if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			size = (int)m_Indices.size() - 2;
		}

		for (int i = 0; i < size;)
		{
			int evenIndex{ 0 };
			if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				evenIndex = i % 2;
			}
			int index0{ (int)m_Indices[i] };
			int index1{ (int)m_Indices[i + 1 + evenIndex] };
			int index2{ (int)m_Indices[i + 2 - evenIndex] };

			//Increase i based on primitiveTopology
			if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
			{
				i += 3;
			}
			else if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				++i;
			}

			//Calculate the points of the triangle
			Vector4 v0{ transformedVertices[index0].position };
			Vector4 v1{ transformedVertices[index1].position };
			Vector4 v2{ transformedVertices[index2].position };

			//Frustum Culling
			if (v0.x < -1.0f || v0.x > 1.0f || v0.y < -1.0f || v0.y > 1.0f || v0.z < 0.0f || v0.z > 1.0f ||
				v1.x < -1.0f || v1.x > 1.0f || v1.y < -1.0f || v1.y > 1.0f || v1.z < 0.0f || v1.z > 1.0f ||
				v2.x < -1.0f || v2.x > 1.0f || v2.y < -1.0f || v2.y > 1.0f || v2.z < 0.0f || v2.z > 1.0f)
			{
				continue;
			}

			//Pre-calculate value for the depth buffer -> depth buffer will not be linear anymore
			float v0InvDepth{ 1 / v0.w };
			float v1InvDepth{ 1 / v1.w };
			float v2InvDepth{ 1 / v2.w };

			//Convert from NDC to raster space
			//Go from [-1,1] range to [0,1] range, taking screen size into acount
			v0.x = ((v0.x + 1) / 2.0f) * m_Width;
			v0.y = ((1 - v0.y) / 2.0f) * m_Height;

			v1.x = ((v1.x + 1) / 2.0f) * m_Width;
			v1.y = ((1 - v1.y) / 2.0f) * m_Height;

			v2.x = ((v2.x + 1) / 2.0f) * m_Width;
			v2.y = ((1 - v2.y) / 2.0f) * m_Height;


			//Calculate the bounding box
			float xMin = std::min(std::min(v0.x, v1.x), v2.x);
			float xMax = std::max(std::max(v0.x, v1.x), v2.x);

			float yMin = std::min(std::min(v0.y, v1.y), v2.y);
			float yMax = std::max(std::max(v0.y, v1.y), v2.y);

			//Use the min and max values of the bounding box to loop over the pixels
			for (int py{ (int)yMin }; py < yMax; ++py)
			{
				for (int px{ (int)xMin }; px < xMax; ++px)
				{
					ColorRGB finalColor{ 0.f, 0.f, 0.f };

					//Current pixel
					Vector2 pixel{ (float)px,(float)py };


					//Check if the current pixel overlaps the triangle formed by the vertices
					//2D cross product gives a float, based on sign we know if the point is inside the triangle
					Vector2 edge0{ {v1.x - v0.x}, {v1.y - v0.y} };
					Vector2 pointToEdge0{ Vector2{v0.x, v0.y }, pixel };
					float cross0{ Vector2::Cross(edge0, pointToEdge0) };

					Vector2 edge1{ {v2.x - v1.x}, {v2.y - v1.y} };
					Vector2 pointToEdge1{ Vector2{v1.x, v1.y }, pixel };
					float cross1{ Vector2::Cross(edge1, pointToEdge1) };

					Vector2 edge2{ {v0.x - v2.x}, {v0.y - v2.y} };
					Vector2 pointToEdge2{ Vector2{v2.x, v2.y }, pixel };
					float cross2{ Vector2::Cross(edge2, pointToEdge2) };

					if (cross0 > 0.0f && cross1 > 0.0f && cross2 > 0.0f)
					{
						//Calculate the barycentric coordinates
						//2D cross product of V1V0 and V2V0
						float areaOfparallelogram{ Vector2::Cross(edge0, edge1) };

						//Calculate the weights
						float w0{ Vector2::Cross(edge1, pointToEdge1) / areaOfparallelogram };
						float w1{ Vector2::Cross(edge2, pointToEdge2) / areaOfparallelogram };
						float w2{ Vector2::Cross(edge0, pointToEdge0) / areaOfparallelogram };

						if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
						{
							//Do the depth buffer test
							float zBuffer0{ (1.0f / v0.z) * w0 };
							float zBuffer1{ (1.0f / v1.z) * w1 };
							float zBuffer2{ (1.0f / v2.z) * w2 };

							float zBuffer{ zBuffer0 + zBuffer1 + zBuffer2 };
							float invZBuffer{ 1.0f / zBuffer };

							if (invZBuffer < 0.0f || invZBuffer > 1.0f)
							{
								break;
							}

							if (invZBuffer < m_pDepthBufferPixels[px + (py * m_Width)])
							{
								//Write value of invZbuffer to the depthBuffer
								m_pDepthBufferPixels[px + (py * m_Width)] = invZBuffer;

								//Interpolated the depth value
								float wInterpolated{ 1.0f / ((w0 / v0.w) + (w1 / v1.w) + (w2 / v2.w)) };

								//Interpolated colour
								ColorRGB interpolatedColour{ transformedVertices[index0].color * (w0 / v0.w) +
															transformedVertices[index1].color * (w1 / v1.w) +
															transformedVertices[index2].color * (w2 / v2.w) };
								interpolatedColour *= wInterpolated;


								//Interpolated uv
								Vector2 interpolatedUV{ transformedVertices[index0].uv * (w0 / v0.w) +
														transformedVertices[index1].uv * (w1 / v1.w) +
														transformedVertices[index2].uv * (w2 / v2.w) };
								interpolatedUV *= wInterpolated;

								/*interpolatedUV.x = Clamp(interpolatedUV.x, 0.f, 1.f);
								interpolatedUV.y = Clamp(interpolatedUV.y, 0.f, 1.f);*/


								//Interpolated normal
								Vector3 interpolatedNormal{ transformedVertices[index0].normal * (w0 / v0.w) +
															transformedVertices[index1].normal * (w1 / v1.w) +
															transformedVertices[index2].normal * (w2 / v2.w) };
								interpolatedNormal *= wInterpolated;
								//Normalize direction vectors!
								interpolatedNormal.Normalize();


								//Interpolated tangent
								Vector3 interpolatedTangent{ transformedVertices[index0].tangent * (w0 / v0.w) +
															transformedVertices[index1].tangent * (w1 / v1.w) +
															transformedVertices[index2].tangent * (w2 / v2.w) };
								interpolatedTangent *= wInterpolated;
								//Normalize direction vectors!
								interpolatedTangent.Normalize();


								//Interpolated viewDirection
								Vector3 interpolatedViewDirection{ transformedVertices[index0].viewDirection * (w0 / v0.w) +
																	transformedVertices[index1].viewDirection * (w1 / v1.w) +
																	transformedVertices[index2].viewDirection * (w2 / v2.w) };
								interpolatedViewDirection *= wInterpolated;
								//Normalize direction vectors!
								interpolatedViewDirection.Normalize();


								Vertex_Out pixelInfo{};
								pixelInfo.position = Vector4{ pixel.x, pixel.y, invZBuffer, wInterpolated };
								pixelInfo.uv = interpolatedUV;
								pixelInfo.normal = interpolatedNormal;
								pixelInfo.tangent = interpolatedTangent;
								pixelInfo.viewDirection = interpolatedViewDirection;

								//Render the pixel
								finalColor = RenderPixelInfo(pixelInfo);

								//Update Color in Buffer
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
	}
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	for (int index{}; index < vertices_in.size(); ++index)
	{
		Vertex vertexInWorldSpace = vertices_in[index];

		// Transform world to view (camera space)
		const Vector3 viewSpaceVertex = m_Camera.viewMatrix.TransformPoint(vertexInWorldSpace.position);

		// Position buffer
		Vector3 position{};

		// Perspective divide -> Projection
		position.x = (viewSpaceVertex.x / viewSpaceVertex.z) / (((float)m_Width / (float)m_Height) * m_Camera.fov);
		position.y = (viewSpaceVertex.y / viewSpaceVertex.z) / m_Camera.fov;
		position.z = viewSpaceVertex.z;

		// Projection -> raster
		position.x = ((position.x + 1) * (float)m_Width) / 2.f;
		position.y = ((1 - position.y) * (float)m_Height) / 2.f;

		Vertex rasterVertex{ position, vertices_in[index].color };
		vertices_out.emplace_back(rasterVertex);
	}
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& mesh_In) const
{
	Matrix worldViewProjectionMatrix{};


	for (Mesh& mesh : mesh_In)
	{
		worldViewProjectionMatrix *= mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;
		mesh.vertices_out.clear();

		for (auto& vertex : mesh.vertices)
		{
			//Transfrom vertex from model space to screen space (aka raster space)
			Vector4 position{ Vector4{vertex.position, 1} };
			Vector4 transformedVertex{ worldViewProjectionMatrix.TransformPoint(position) };

			//Get the viewDirection from the vertex position
			Vector3 viewDirection{ mesh.worldMatrix.TransformPoint(vertex.position) - m_Camera.origin };

			//Do the perspective divide with the w component from the Vector4 transfromedVertex
			transformedVertex.x /= transformedVertex.w;
			transformedVertex.y /= transformedVertex.w;
			transformedVertex.z /= transformedVertex.w;

			//Normal and tangent info from vertex
			Vector3 normal = mesh.worldMatrix.TransformVector(vertex.normal);
			normal.Normalize();
			Vector3 tangent = mesh.worldMatrix.TransformVector(vertex.tangent);
			tangent.Normalize();

			Vertex_Out outVertex{};
			outVertex.color = vertex.color;
			outVertex.normal = normal;
			outVertex.position = transformedVertex;
			outVertex.tangent = tangent;
			outVertex.uv = vertex.uv;
			outVertex.viewDirection = viewDirection;

			mesh.vertices_out.push_back(outVertex);
		}
	}
}

ColorRGB Renderer::RenderPixelInfo(const Vertex_Out& vertexOut)
{
	ColorRGB finalColour{};

	Vector3 lightDirection{ 0.577f,-0.577f,0.577f };
	float lightIntensity{ 7.0f };
	ColorRGB totalLight{ ColorRGB{1.0f,1.0f,1.0f} * lightIntensity };
	float shininess{ 25.0f };
	ColorRGB ambient{ 0.025f,0.025f,0.025f };

	//Diffuse map
	ColorRGB diffuse = m_pVehicleDiffuse->Sample(vertexOut.uv);

	//Normal map
	Vector3 biNormal{ Vector3::Cross(vertexOut.normal, vertexOut.tangent).Normalized() };
	Matrix tangentAxisSpace{ Matrix{vertexOut.tangent, biNormal, vertexOut.normal, {0,0,0}} };

	ColorRGB normalColour{ m_pVehicleNormal->Sample(vertexOut.uv) };
	Vector3 normal{ 2.0f * normalColour.r - 1.0f, 2.0f * normalColour.g - 1.0f, 2.0f * normalColour.b - 1.0f };
	normal = tangentAxisSpace.TransformVector(normal);
	normal.Normalize();

	//Glossy map
	ColorRGB gloss = m_pVehicleGlossy->Sample(vertexOut.uv);

	//Specular map
	ColorRGB specular = m_pVehicleSpecular->Sample(vertexOut.uv);

	//Calculate labert cosine
	//Make sure that the normal and the lightDirection point in the same direction (originally opposed to each other)
	float lambertCosine{ Vector3::Dot(normal,-lightDirection) };
	if (lambertCosine <= 0.0f)
	{
		finalColour = { 0.f,0.f,0.f };
		return finalColour;
	}

	switch (m_Shadingmode)
	{
	case dae::Renderer::ShadingMode::Combined:

	{
		ColorRGB phongExponent{ gloss * shininess };

		Vector3 reflect{ Vector3::Reflect(-lightDirection, normal)};
		float cosAlpha{ std::max(0.0f, Vector3::Dot(reflect, vertexOut.viewDirection)) };
		ColorRGB phong{ specular * std::powf(cosAlpha, phongExponent.r) };

		ColorRGB rho{ diffuse };
		ColorRGB diffuseColour{ rho / PI };

		finalColour = lambertCosine * totalLight * diffuseColour + (phong + ambient);
	}

	break;
	case dae::Renderer::ShadingMode::Diffuse:

	{
		ColorRGB rho{ diffuse };
		ColorRGB diffuseColour{ rho / PI };
		finalColour = totalLight * diffuseColour * lambertCosine;
	}

	break;
	case dae::Renderer::ShadingMode::Specular:

	{
		ColorRGB phongExponent{ gloss * shininess };

		Vector3 reflect{ Vector3::Reflect(-lightDirection, normal) };
		float cosAlpha{ std::max(0.0f, Vector3::Dot(reflect, vertexOut.viewDirection)) };
		ColorRGB phong{ specular * std::powf(cosAlpha, phongExponent.r) };

		finalColour = totalLight * phong * lambertCosine;
	}

	break;
	case dae::Renderer::ShadingMode::ObservedArea:

	{
		finalColour = { lambertCosine,lambertCosine,lambertCosine };
	}

	break;
	default:
		break;
	}

	return finalColour;
}

float Renderer::GetTriangleEdge(const Vector2& a, const Vector2& b, const Vector2& c)
{
	return Vector2::Cross(b - a, c - a);
}

float Renderer::GetTriangleEdge(const Vector3& a, const Vector3& b, const Vector3& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::ToggleRotation()
{
}

void Renderer::ToggleNormalMap()
{
	m_IsNormalMapEnabled = !m_IsNormalMapEnabled;
}

void Renderer::SwitchShadingMode()
{
	switch (m_Shadingmode)
	{
	case Renderer::ShadingMode::Combined:
		m_Shadingmode = ShadingMode::ObservedArea;
		break;
	case Renderer::ShadingMode::ObservedArea:
		m_Shadingmode = ShadingMode::Diffuse;
		break;
	case Renderer::ShadingMode::Diffuse:
		m_Shadingmode = ShadingMode::Specular;
		break;
	case Renderer::ShadingMode::Specular:
		m_Shadingmode = ShadingMode::Combined;
		break;
	default:
		break;
	}
}

bool Renderer::IsPointInTriangle(const std::vector<Vector3>& screenTriangleCoordinates, int pixelX, int pixelY)
{
	const Vector3 p{ (float)pixelX, (float)pixelY, 0.f };

	const Vector3 V0 = screenTriangleCoordinates[2];
	const Vector3 V1 = screenTriangleCoordinates[1];
	const Vector3 V2 = screenTriangleCoordinates[0];

	if (GetTriangleEdge(V0, V1, p) < 0.f)
		return false;

	if (GetTriangleEdge(V1, V2, p) < 0.f)
		return false;

	if (GetTriangleEdge(V2, V0, p) < 0.f)
		return false;

	return true;
}

bool Renderer::IsPointInTriangle(const std::vector<Vertex>& screenTriangleCoordinates, int pixelX, int pixelY)
{
	const Vector3 p{ (float)pixelX, (float)pixelY, 0 };

	const Vector3 V0 = screenTriangleCoordinates[2].position;
	const Vector3 V1 = screenTriangleCoordinates[1].position;
	const Vector3 V2 = screenTriangleCoordinates[0].position;

	if (GetTriangleEdge(V0, V1, p) < 0.f)
		return false;

	if (GetTriangleEdge(V1, V2, p) < 0.f)
		return false;

	if (GetTriangleEdge(V2, V0, p) < 0.f)
		return false;

	return true;
}

bool Renderer::IsPointInTriangle(const std::vector<Vertex>& screenTriangleCoordinates, Vector2 point)
{
	const Vector3 p{ point.x, point.y, 0 };

	const Vector3 V0 = screenTriangleCoordinates[2].position;
	const Vector3 V1 = screenTriangleCoordinates[1].position;
	const Vector3 V2 = screenTriangleCoordinates[0].position;

	if (GetTriangleEdge(V0, V1, p) < 0.f)
		return false;

	if (GetTriangleEdge(V1, V2, p) < 0.f)
		return false;

	if (GetTriangleEdge(V2, V0, p) < 0.f)
		return false;

	return true;
}

bool Renderer::IsPointInTriangle(const std::vector<Vertex_Out>& screenTriangleCoordinates, Vector2 point)
{
	const Vector3 p{ point.x, point.y, 0 };

	const Vector3 V0 = screenTriangleCoordinates[2].position;
	const Vector3 V1 = screenTriangleCoordinates[1].position;
	const Vector3 V2 = screenTriangleCoordinates[0].position;

	if (GetTriangleEdge(V0, V1, p) < 0.f)
		return false;

	if (GetTriangleEdge(V1, V2, p) < 0.f)
		return false;

	if (GetTriangleEdge(V2, V0, p) < 0.f)
		return false;

	return true;
}