#define _USE_MATH_DEFINES
#include "Raytracer.h"
#include "Scene.h"
#include "Cameras/Camera.h"
#include <thread>
#include "Integrators/Integrator.h"
#include "ComponentFactories.h"
#include <iostream>
#include "../IO/SceneLoader.h"
#include "../IO/pfm.h"

using namespace DirectX::SimpleMath;

Raytracer::Raytracer(void) {}

bool Raytracer::Initialize(int _width, int _height, std::string _integrator,
                           int _spp, int _tileSize, int _threads,
                           const char *scene) {
  std::cout << "Initializing renderer..." << std::endl;
  /* for best performance set FTZ and DAZ flags in MXCSR control and status
   * register */
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

  m_IsShutDown = false;
  m_SPP = _spp;
  m_TileSize = _tileSize;
  m_ThreadCount = _threads;
  m_Width = _width;
  m_Height = _height;

#ifndef HEADLESS
  m_Pixels = new sf::Uint8[m_Width * m_Height * 4]{0};
#endif

  m_RawPixels = new Color[m_Width * m_Height * 4]{};

  std::cout << "Loading scene..." << std::endl;
  Camera *cam = nullptr;
  std::vector<BaseObject *> objects;

  if (!LoadScene(scene, objects, &cam)) {
    return false;
  }

  m_pCamera.reset(cam);

  m_pScene = std::make_unique<Scene>(m_pCamera.get(), objects);
  m_pIntegrator.reset(IntegratorFactory(_integrator, m_pScene.get(), m_pCamera.get(), m_Width, m_Height));

  return true;
}

void Raytracer::Wait() {
  for (int i = 0; i < m_ThreadCount; i++) {
    m_Threads[i].join();
  }

  m_IsShutDown = true;
}

void Raytracer::Shutdown(void) {
  if (!m_IsShutDown) {
    m_IsShutDown = true;
    Wait();
  }

#ifndef HEADLESS
  if (m_Pixels) {
    delete[] m_Pixels;
    m_Pixels = nullptr;
  }
#endif

  if (m_RawPixels) {
    delete[] m_RawPixels;
    m_RawPixels = nullptr;
  }
}

void Raytracer::SetFOV(float _fov) { m_FOV = _fov; }

void Raytracer::RenderPart(int _x, int _y, int _width, int _height, int _spp) {
  assert(_x + _width <= m_Width);
  assert(_y + _height <= m_Height);

  std::random_device d;
  std::default_random_engine rnd(d());

  std::uniform_real_distribution<float> pixel_dist(-0.5, 0.5);

  for (int i = 0; i < _spp; i++) {
    for (int x = _x; x < _x + _width; x++) {
      for (int y = _y; y < _y + _height; y++) {
        int pixelIndex = (x + m_Width * y) * 4;
        
        Color rayColor = m_pIntegrator->Sample(x + pixel_dist(rnd), y + pixel_dist(rnd), m_Width, m_Height, rnd);

        Color *pixelAddress = m_RawPixels + x + m_Width * y;
        *pixelAddress += (rayColor - *pixelAddress) / float(i + 1);

#ifndef HEADLESS
        Color current = *pixelAddress;
        current.Saturate();

        current.x = pow(current.x, 1.0f / 2.2f);
        current.y = pow(current.y, 1.0f / 2.2f);
        current.z = pow(current.z, 1.0f / 2.2f);

        sf::Color newCol((sf::Uint8)(current.R() * 255),
                         (sf::Uint8)(current.G() * 255),
                         (sf::Uint8)(current.B() * 255), 255);
        memcpy(m_Pixels + pixelIndex, &newCol, 4);
#endif
      }
    }

    if (m_IsShutDown) {
      return;
    }
  }
}

void Raytracer::EmptyQueue(int threadIndex) {
  TileInfo toRender;
  int tileIndex;
  while (!m_IsShutDown) {
    {
      std::lock_guard<std::mutex> lock(m_TileMutex);
      if (m_TilesToRender.size() == 0) {
        break;
      }

      toRender = m_TilesToRender.back();
      m_TilesInProgress++;
      m_TilesToRender.pop_back();
      tileIndex = m_TilesToRender.size();
      if (tileIndex == 0)
        m_IsRendering = false;
    }

    RenderPart(toRender.X, toRender.Y, toRender.Width, toRender.Height,
               toRender.SPP);
    m_TilesInProgress--;
  }
}

void Raytracer::Render(int frameIndex) {
  m_IsRendering = true;
  m_pScene->SetTime(frameIndex);
  memset(m_RawPixels, 0, m_Height * m_Width * sizeof(Color));

  m_pIntegrator->Reset();

#ifndef HEADLESS
  memset(m_Pixels, 0, m_Height * m_Width * sizeof(sf::Uint8) * 4);
#endif

#ifdef MULTI_THREADED
  std::cout << "Start rendering..." << std::endl;
  // Spawn rendering threads
  for (int x = 0; x < m_Width; x += m_TileSize) {
    int width = std::min(m_TileSize, (m_Width - x));
    for (int y = 0; y < m_Height; y += m_TileSize) {
      int height = std::min(m_TileSize, (m_Height - y));
      m_TilesToRender.push_back({x, y, width, height, m_SPP - 5});
    }
  }

  // Preview render
  for (int x = 0; x < m_Width; x += m_TileSize * 2) {
    int width = std::min(m_TileSize * 2, (m_Width - x));
    for (int y = 0; y < m_Height; y += m_TileSize * 2) {
      int height = std::min(m_TileSize * 2, (m_Height - y));
      m_TilesToRender.push_back({x, y, width, height, 5});
    }
  }

  std::cout << "Rendering " << m_TilesToRender.size() << " tiles ("
            << m_TileSize << ") on " << m_ThreadCount << " threads."
            << std::endl;

  m_TilesInProgress = 0;

  {
    m_Threads.reserve(m_ThreadCount);
    std::lock_guard<std::mutex> lock(m_TileMutex);
    for (int i = 0; i < m_ThreadCount; i++) {
      m_Threads.push_back(std::thread(&Raytracer::EmptyQueue, this, i));
    }
  }

#else
  RenderPart(0, 0, m_Width, m_Height, m_SPP);
#endif
}

void Raytracer::SaveImages(std::string basename) {

  m_pIntegrator->Finalize(m_SPP);

  stbi_write_hdr((basename + ".hdr").c_str(),
    m_Width, m_Height, 4, (float *)GetRawPixels());

  auto outputs = m_pIntegrator->getOutputs();

  for (auto& output : outputs) {
    Color* data = output.Data.get();
    switch (output.format) {
      case OutputFormat::HDR:
        stbi_write_hdr((basename + "-" + output.name + ".hdr").c_str(),
          m_Width, m_Height, 4, (float*)data);
        break;
      case OutputFormat::PNG:
        stbi_write_png((basename + "-" + output.name + ".png").c_str(),
          m_Width, m_Height, 4, data, 0);
        break;
      case OutputFormat::PFM:
        float* data3 = (float*)malloc(m_Width * m_Height * 3 * sizeof(float));
        for (int x = 0; x < m_Width; x++) {
          for (int y = 0; y < m_Height; y++) {
            int i = x + m_Width * (m_Height - y - 1);
            int j = x + m_Width * y;
            Color col = data[j];
            data3[i * 3 + 0] = col.R();
            data3[i * 3 + 1] = col.G();
            data3[i * 3 + 2] = col.B();
          }
        }
        
        write_pfm_file3((basename + "-" + output.name + ".pfm").c_str(), data3, m_Width, m_Height);
        free(data3);
        break;
    }
  }

}

#ifndef HEADLESS
sf::Uint8 *Raytracer::GetPixels(void) const { return m_Pixels; }
#endif

Raytracer::~Raytracer(void) {}
