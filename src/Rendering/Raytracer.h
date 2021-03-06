#pragma once
#include <thread>
#include "../SimpleMath.h"
#include <mutex>
#include <atomic>
#include <vector>
#include "../IO/stb_image_write.h"

#ifndef HEADLESS
#include <SFML/Graphics.hpp>
#endif

class Camera;
class Scene;
class Integrator;

#define MULTI_THREADED
#define BOUNCES 8

/********************************************
** Raytracer
** Base class of this renderer, fills an array
** of pixels based on the scenes content
*********************************************/
#include <iostream>
class Raytracer {
private:
  struct TileInfo {
    int X, Y, Width, Height, SPP;
  };

#ifndef HEADLESS
  sf::Uint8 *m_Pixels;
#endif

  DirectX::SimpleMath::Color *m_RawPixels;

  std::unique_ptr<Camera> m_pCamera;
  std::unique_ptr<Scene> m_pScene;
  std::unique_ptr<Integrator> m_pIntegrator;

  float m_FOV;
  int m_Width;
  int m_Height;
  int m_SPP;
  int m_TileSize;
  int m_ThreadCount;

  std::atomic<bool> m_IsShutDown;
  std::atomic<bool> m_IsRendering;
  std::atomic<int> m_TilesInProgress;

  std::vector<std::thread> m_Threads;
  std::vector<TileInfo> m_TilesToRender;

  std::mutex m_TileMutex;

  // Render a part of the image (for multy threading)
  void RenderPart(int _x, int _y, int _width, int _height, int _spp);
   
  void EmptyQueue(int threadIndex);

public:
  Raytracer(void);
  bool Initialize(int _width, int _height, std::string _integrator,
                  int _spp, int _tileSize, int _threads,
                  const char *scene);
  void Shutdown(void);
  void SetFOV(float _fov);

  bool FrameDone() {
	  return m_TilesInProgress == 0 && !m_IsRendering;
  }

  void Wait();

  void Render(int frameIndex);

  void SaveImages(std::string basename);

#ifndef HEADLESS
  sf::Uint8 *GetPixels(void) const;
#endif

  DirectX::SimpleMath::Color *GetRawPixels(void) const { return m_RawPixels; }

  ~Raytracer(void);
};
