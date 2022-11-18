//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

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

	//Render_W1_Gradient();
	//Render_W1_Part1();
	//Render_W1_Part2();
	//Render_W1_Part3();
	//Render_W1_Part4();
	Render_W1_Part5();

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

float dae::Renderer::GetTriangleEdge(const Vector2& a, const Vector2& b, const Vector2& c)
{
	return Vector2::Cross(b - a, c - a);
}

float dae::Renderer::GetTriangleEdge(const Vector3& a, const Vector3& b, const Vector3& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
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

bool dae::Renderer::IsPointInTriangle(const std::vector<Vertex>& screenTriangleCoordinates, Vector2 point)
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
