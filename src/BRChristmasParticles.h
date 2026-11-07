#pragma once
#include <FFGLSDK.h>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

class BRChristmasParticles final : public CFFGLPlugin {
public:
  BRChristmasParticles();
  ~BRChristmasParticles() override;
  FFResult InitGL(const FFGLViewportStruct*) override;
  FFResult ProcessOpenGL(ProcessOpenGLStruct*) override;
  FFResult DeInitGL() override;
  FFResult SetFloatParameter(unsigned int,float) override;
  float GetFloatParameter(unsigned int) override;
  FFResult SetTextParameter(unsigned int,const char*) override;
  char* GetTextParameter(unsigned int) override;
private:
  void addOption(unsigned int,unsigned int,const char*,float);
  void chooseCustomImage();
  void rebuildCustomTexture();
  void releaseCustomTexture();
  std::vector<float> values_;
  std::string customPath_;
  const std::string credit_="Effect by Belajar Resolume";
  std::atomic<bool> textureDirty_{false};
  unsigned long long gdiplusToken_=0;
  unsigned int customTexture_=0;
  unsigned int viewportWidth_=1920,viewportHeight_=1080;
  std::chrono::steady_clock::time_point lastFrame_;
  double elapsed_=0.0,stopStartedAt_=0.0,burstStartedAt_=-100.0;
  bool paused_=false,stopping_=false,stopped_=false;
  ffglex::FFGLShader shader_;
  ffglex::FFGLScreenQuad quad_;
};

