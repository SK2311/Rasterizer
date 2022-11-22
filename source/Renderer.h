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

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		Texture* m_pTexture;

		void Render_W1_Gradient();
		void Render_W1_Part1();
		void Render_W1_Part2();
		void Render_W1_Part3();
		void Render_W1_Part4();
		void Render_W1_Part5();

		void Render_W2_TriangleStrip();
		void Render_W2_TriangleList();
		void Render_W2_Textures();
		void Render_W2_DepthInterpolation();

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const; //W1 Version
		void VertexTransformationFunction(std::vector<Mesh>& mesh_In) const; //W2 Version

		bool IsPointInTriangle(const std::vector<Vertex>& screenTriangleCoordinates, int pixelX, int pixelY);
		bool IsPointInTriangle(const std::vector<Vertex>& screenTriangleCoordinates, Vector2 point);
		bool IsPointInTriangle(const std::vector<Vector3>& screenTriangleCoordinates, int pixelX, int pixelY);
		bool IsPointInTriangle(const std::vector<Vertex_Out>& screenTriangleCoordinates, Vector2 point);

		float GetTriangleEdge(const Vector2& a, const Vector2& b, const Vector2& c);
		float GetTriangleEdge(const Vector3& a, const Vector3& b, const Vector3& c);
	};
}
