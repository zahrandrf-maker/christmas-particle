#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#ifdef NOMETAFILE
#undef NOMETAFILE
#endif
#ifdef NOGDI
#undef NOGDI
#endif
#include <windows.h>
#include <wingdi.h>
#include <commdlg.h>
#include <objidl.h>
#include <gdiplus.h>
#include "BRChristmasParticles.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace ffglex;
using namespace Gdiplus;

namespace {
enum Param:FFUInt32{
 P_START,P_PAUSE,P_RESUME,P_STOP,P_RESET,P_BURST,P_START_TIME,P_STOP_TIME,
 P_MODE,P_TYPE,P_LOAD_IMAGE,P_IMAGE_PATH,
 P_AMOUNT,P_BURST_AMOUNT,P_SIZE,P_SIZE_RANDOM,P_SPEED,P_SPEED_RANDOM,
 P_DIRECTION,P_GRAVITY,P_TURBULENCE,P_ROTATION,
 P_POS_X,P_POS_Y,P_OPACITY,P_HUE,P_SAT,P_BRI,P_GLOW,P_TRAIL,
 P_DOF,P_FOCUS,P_DOF_BLUR,P_QUALITY,P_SEED,P_CREDIT,P_COUNT};

static CFFGLPluginInfo PluginInfo(PluginFactory<BRChristmasParticles>,"BRCP",
 "BR Christmas Particles",2,1,0,100,FF_SOURCE,
 "Christmas particle generator by BELAJAR RESOLUME","Effect by Belajar Resolume");

std::wstring utf8ToWide(const std::string& s){
 if(s.empty())return L"";int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);
 if(n<=0)return std::wstring(s.begin(),s.end());std::wstring r(size_t(n),L'\0');
 MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,r.data(),n);r.pop_back();return r;}
std::string wideToUtf8(const std::wstring& s){
 if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,nullptr,0,nullptr,nullptr);
 std::string r(size_t(n),'\0');WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,r.data(),n,nullptr,nullptr);r.pop_back();return r;}

const char* vertexShader=R"GLSL(#version 410 core
layout(location=0)in vec4 vPosition;layout(location=1)in vec2 vUV;out vec2 uv;
void main(){gl_Position=vPosition;uv=vUV;})GLSL";

const char* fragmentShader=R"GLSL(#version 410 core
uniform sampler2D customTex;uniform vec2 resolution;uniform float elapsed,particleMode,particleType;
uniform float amount,burstAmount,particleSize,sizeRandom,particleSpeed,speedRandom,directionMode;
uniform float gravity,turbulence,rotationSpeed,centerX,centerY,opacityValue;
uniform vec3 particleColor;uniform float glowAmount,trailAmount,dofEnabled,focusLayer,dofBlur;
uniform float quality,randomSeed,startProgress,stopProgress,burstAge,hasCustom;
in vec2 uv;out vec4 fragColor;

float h(float p){p=fract(p*.1031);p*=p+33.33;p*=p+p;return fract(p);}
vec2 h2(float p){return vec2(h(p+1.7),h(p+9.2));}
float sdSegment(vec2 p,vec2 a,vec2 b){vec2 pa=p-a,ba=b-a;float t=clamp(dot(pa,ba)/dot(ba,ba),0.,1.);return length(pa-ba*t);}
float sdBox(vec2 p,vec2 b){vec2 d=abs(p)-b;return length(max(d,0.))+min(max(d.x,d.y),0.);}
float sdStar(vec2 p){float a=atan(p.y,p.x),r=length(p);float edge=mix(.42,1.,.5+.5*cos(5.*a));return r-edge;}
float sdSnow(vec2 p){float d=99.;for(int i=0;i<3;i++){float a=float(i)*1.0472;vec2 x=vec2(cos(a),sin(a)),y=vec2(-x.y,x.x);d=min(d,sdSegment(p,-x*.95,x*.95));d=min(d,sdSegment(p,x*.3,x*.68+y*.25));d=min(d,sdSegment(p,x*.3,x*.68-y*.25));d=min(d,sdSegment(p,-x*.3,-x*.68+y*.25));d=min(d,sdSegment(p,-x*.3,-x*.68-y*.25));}return d-.065;}
float sdBell(vec2 p){float dome=length((p-vec2(0.,.15))*vec2(1.,.8))-.64;float cut=-(p.y+.25);float body=max(dome,cut);float rim=sdBox(p-vec2(0.,-.3),vec2(.72,.09));float clapper=length(p-vec2(0.,-.53))-.13;return min(min(body,rim),clapper);}
float sdGift(vec2 p){float box=sdBox(p,vec2(.72,.58));float ribbon=min(sdBox(p,vec2(.11,.7)),sdBox(p,vec2(.8,.1)));float bow=min(length(p-vec2(-.2,.72))-.22,length(p-vec2(.2,.72))-.22);return min(box,min(ribbon,bow));}
float sdCandy(vec2 p){float stem=sdSegment(p,vec2(-.18,-.9),vec2(-.18,.35))-.18;float hook=abs(length(p-vec2(.1,.35))-.28)-.18;return min(stem,max(hook,p.y-.64));}
vec2 dir(float m){if(m<.5)return vec2(0.,-1.);if(m<1.5)return vec2(0.,1.);if(m<2.5)return vec2(-1.,0.);if(m<3.5)return vec2(1.,0.);if(m<4.5)return normalize(vec2(-.6,-1.));if(m<5.5)return normalize(vec2(.6,-1.));return vec2(0.,0.);}
float travel(vec2 q,float m){if(m<.5)return 1.-q.y;if(m<1.5)return q.y;if(m<2.5)return 1.-q.x;if(m<3.5)return q.x;if(m<4.5)return (2.-q.x-q.y)*.5;if(m<5.5)return (1.-q.y+q.x)*.5;return .5;}

vec2 rot(vec2 p,float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c)*p;}
float shapeAlpha(vec2 p,float id,float type,float blur){
 float angle=elapsed*rotationSpeed*mix(-3.,3.,h(id+3.))+id;p=rot(p,angle);
 float chosen=type;if(type>6.5)chosen=floor(h(id+17.)*6.);
 float d;if(chosen<.5)d=sdStar(p);else if(chosen<1.5)d=sdSnow(p);else if(chosen<2.5)d=length(p)-.78;else if(chosen<3.5)d=sdBell(p);else if(chosen<4.5)d=sdCandy(p);else if(chosen<5.5)d=sdGift(p);else d=length(p)-.85;
 float edge=.025+blur*.28;float a=1.-smoothstep(-edge,edge,d);
 if(chosen>5.5&&chosen<6.5&&hasCustom>.5){vec2 tuv=p*.5+.5;if(all(greaterThanEqual(tuv,vec2(0)))&&all(lessThanEqual(tuv,vec2(1))))a=texture(customTex,vec2(tuv.x,1.-tuv.y)).a;else a=0.;}
 float halo=(1.-smoothstep(.55,1.7+blur*2.,length(p)))*(glowAmount*.35+blur*.28);return clamp(a+halo,0.,1.);}

float layer(vec2 scene,float li){
 float dep=mix(.65,1.2,li/2.),density=mix(3.,15.,amount)*dep;vec2 g=scene*density;
 float sp=mix(.025,.43,particleSpeed)*dep;vec2 dv=dir(directionMode);
 g-=(dv*elapsed*sp+vec2(0.,gravity*elapsed*.025))*density;vec2 base=floor(g),f=fract(g);float outA=0.;
 for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){vec2 o=vec2(x,y),idv=base+o;float id=idv.x*127.1+idv.y*311.7+li*73.+randomSeed*997.;vec2 pt=.12+h2(id)*.76;
  pt.x+=sin(elapsed*(.4+h(id+4.))+id)*turbulence*.3;float rv=mix(1.-sizeRandom*.7,1.+sizeRandom*.8,h(id+8.));
  float sr=mix(1.-speedRandom*.5,1.+speedRandom*.5,h(id+12.));pt-=dv*elapsed*sp*(sr-1.)*density;
  vec2 p=(f-(o+pt))/max(mix(.045,.3,particleSize)*rv*dep,.001);
  float fd=focusLayer>2.5?0.:abs(li-focusLayer)/2.;float blur=dofEnabled>.5?fd*dofBlur:0.;float core=shapeAlpha(p,id,particleType,blur);
  for(int k=1;k<=3;k++){float fade=1.-float(k)*.24;core=max(core,shapeAlpha(p+dv*float(k)*trailAmount*.42,id,particleType,blur)*trailAmount*fade);}outA=max(outA,core*mix(1.,.65,blur));}
 return outA;}

float burst(vec2 scene){if(burstAge<0.||burstAge>5.)return 0.;float aspect=resolution.x/max(resolution.y,1.);vec2 c=vec2(centerX*aspect,centerY),sumP=vec2(0);float outA=0.;
 for(int i=0;i<32;i++){float id=float(i)+randomSeed*101.;float active=step(float(i),mix(5.,32.,burstAmount));float a=6.283185*(float(i)/32.+h(id)*.08);vec2 v=vec2(cos(a),sin(a));float dist=burstAge*mix(.08,.65,particleSpeed)*mix(.7,1.3,h(id+2.));vec2 pos=c+v*dist+vec2(0.,-gravity*burstAge*burstAge*.06);vec2 p=(scene-pos)/max(mix(.018,.12,particleSize)*mix(.7,1.3,h(id+5.)),.001);outA=max(outA,shapeAlpha(p,id+900.,particleType,0.)*active*(1.-smoothstep(3.5,5.,burstAge)));}return outA;}

void main(){float aspect=resolution.x/max(resolution.y,1.);vec2 scene=vec2(uv.x*aspect,uv.y);float a=0.;
 if(particleMode<.5||particleMode>1.5){a=layer(scene,0.);if(quality>.25)a=max(a,layer(scene,1.));if(quality>.75)a=max(a,layer(scene,2.));float t=travel(uv,directionMode);if(directionMode>5.5)a*=startProgress*(1.-stopProgress);else{a*=1.-smoothstep(startProgress,startProgress+.035,t);a*=smoothstep(stopProgress-.035,stopProgress,t);}}
 if(particleMode>.5)a=max(a,burst(scene));a=clamp(a*opacityValue,0.,1.);fragColor=vec4(particleColor*a,a);})GLSL";
}

BRChristmasParticles::BRChristmasParticles():values_(P_COUNT,0.f){
 SetMinInputs(0);SetMaxInputs(0);GdiplusStartupInput si;GdiplusStartup(&gdiplusToken_,&si,nullptr);
 values_[P_START_TIME]=.32f;values_[P_STOP_TIME]=.32f;values_[P_MODE]=0;values_[P_TYPE]=7;
 values_[P_AMOUNT]=.48f;values_[P_BURST_AMOUNT]=.65f;values_[P_SIZE]=.35f;values_[P_SIZE_RANDOM]=.55f;
 values_[P_SPEED]=.35f;values_[P_SPEED_RANDOM]=.4f;values_[P_DIRECTION]=0;values_[P_GRAVITY]=.18f;
 values_[P_TURBULENCE]=.3f;values_[P_ROTATION]=.3f;values_[P_POS_X]=.5f;values_[P_POS_Y]=.5f;
 values_[P_OPACITY]=.95f;values_[P_SAT]=.75f;values_[P_BRI]=1;values_[P_GLOW]=.28f;
 values_[P_DOF]=0;values_[P_FOCUS]=3;values_[P_DOF_BLUR]=.4f;values_[P_QUALITY]=.5f;values_[P_SEED]=.37f;
 SetParamInfof(P_START,"Start",FF_TYPE_EVENT);SetParamInfof(P_PAUSE,"Pause",FF_TYPE_EVENT);SetParamInfof(P_RESUME,"Resume",FF_TYPE_EVENT);SetParamInfof(P_STOP,"Stop Naturally",FF_TYPE_EVENT);SetParamInfof(P_RESET,"Reset",FF_TYPE_EVENT);SetParamInfof(P_BURST,"BURST",FF_TYPE_EVENT);
 SetParamInfof(P_START_TIME,"Start Fill Time",FF_TYPE_STANDARD);SetParamInfof(P_STOP_TIME,"Stop Clear Time",FF_TYPE_STANDARD);
 SetOptionParamInfo(P_MODE,"Particle Mode",3,0);addOption(P_MODE,0,"Continuous",0);addOption(P_MODE,1,"Burst",1);addOption(P_MODE,2,"Continuous + Burst",2);
 SetOptionParamInfo(P_TYPE,"Particle Type",8,6);const char* types[]={"Christmas Star","Snowflake","Christmas Ball","Bell","Candy Cane","Gift Box","Custom PNG","Mixed"};for(unsigned i=0;i<8;i++)addOption(P_TYPE,i,types[i],float(i));
 SetParamInfof(P_LOAD_IMAGE,"Load Custom PNG",FF_TYPE_EVENT);SetParamInfo(P_IMAGE_PATH,"Custom Image Path",FF_TYPE_TEXT,customPath_.c_str());
 SetParamInfof(P_AMOUNT,"Amount",FF_TYPE_STANDARD);SetParamInfof(P_BURST_AMOUNT,"Burst Amount",FF_TYPE_STANDARD);SetParamInfof(P_SIZE,"Size",FF_TYPE_STANDARD);SetParamInfof(P_SIZE_RANDOM,"Size Random",FF_TYPE_STANDARD);SetParamInfof(P_SPEED,"Speed",FF_TYPE_STANDARD);SetParamInfof(P_SPEED_RANDOM,"Speed Random",FF_TYPE_STANDARD);
 SetOptionParamInfo(P_DIRECTION,"Direction",7,0);const char* dirs[]={"Down","Up","Left","Right","Down Left","Down Right","Float"};for(unsigned i=0;i<7;i++)addOption(P_DIRECTION,i,dirs[i],float(i));
 SetParamInfof(P_GRAVITY,"Gravity",FF_TYPE_STANDARD);SetParamInfof(P_TURBULENCE,"Turbulence",FF_TYPE_STANDARD);SetParamInfof(P_ROTATION,"Rotation Speed",FF_TYPE_STANDARD);
 SetParamInfof(P_POS_X,"Burst Position X",FF_TYPE_XPOS);SetParamInfof(P_POS_Y,"Burst Position Y",FF_TYPE_YPOS);SetParamInfof(P_OPACITY,"Opacity",FF_TYPE_STANDARD);SetParamInfof(P_HUE,"Particle Color",FF_TYPE_HUE);SetParamInfof(P_SAT,"Color Saturation",FF_TYPE_SATURATION);SetParamInfof(P_BRI,"Color Brightness",FF_TYPE_BRIGHTNESS);SetParamInfof(P_GLOW,"Glow",FF_TYPE_STANDARD);SetParamInfof(P_TRAIL,"Trail",FF_TYPE_STANDARD);
 SetParamInfof(P_DOF,"Depth of Field",FF_TYPE_BOOLEAN);SetOptionParamInfo(P_FOCUS,"DOF Focus",4,3);addOption(P_FOCUS,0,"Background",0);addOption(P_FOCUS,1,"Middle",1);addOption(P_FOCUS,2,"Foreground",2);addOption(P_FOCUS,3,"All Sharp",3);SetParamInfof(P_DOF_BLUR,"DOF Blur",FF_TYPE_STANDARD);
 SetOptionParamInfo(P_QUALITY,"Quality",3,1);addOption(P_QUALITY,0,"Low",0);addOption(P_QUALITY,1,"Medium",.5f);addOption(P_QUALITY,2,"High",1);SetParamInfof(P_SEED,"Random Seed",FF_TYPE_STANDARD);SetParamInfo(P_CREDIT,"Effect by Belajar Resolume",FF_TYPE_TEXT,credit_.c_str());
 for(unsigned i=P_START;i<=P_STOP_TIME;i++)SetParamGroup(i,"Playback");for(unsigned i=P_MODE;i<=P_IMAGE_PATH;i++)SetParamGroup(i,"Particle Type");for(unsigned i=P_AMOUNT;i<=P_SPEED_RANDOM;i++)SetParamGroup(i,"Particle Setting");for(unsigned i=P_DIRECTION;i<=P_ROTATION;i++)SetParamGroup(i,"Motion");for(unsigned i=P_POS_X;i<=P_TRAIL;i++)SetParamGroup(i,"Appearance");for(unsigned i=P_DOF;i<=P_DOF_BLUR;i++)SetParamGroup(i,"Depth of Field");for(unsigned i=P_QUALITY;i<=P_SEED;i++)SetParamGroup(i,"Performance");SetParamGroup(P_CREDIT,"Credit");lastFrame_=std::chrono::steady_clock::now();}

BRChristmasParticles::~BRChristmasParticles(){if(gdiplusToken_)GdiplusShutdown(gdiplusToken_);}
void BRChristmasParticles::addOption(unsigned p,unsigned e,const char*n,float v){SetParamElementInfo(p,e,n,v);}

void BRChristmasParticles::chooseCustomImage(){wchar_t file[32768]=L"";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.lpstrFile=file;o.nMaxFile=32768;o.lpstrFilter=L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0PNG Files\0*.png\0All Files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;if(GetOpenFileNameW(&o)){customPath_=wideToUtf8(file);textureDirty_.store(true);RaiseParamEvent(P_IMAGE_PATH,FF_EVENT_FLAG_VALUE);}}
void BRChristmasParticles::releaseCustomTexture(){if(customTexture_){glDeleteTextures(1,&customTexture_);customTexture_=0;}}
void BRChristmasParticles::rebuildCustomTexture(){releaseCustomTexture();if(customPath_.empty())return;Bitmap b(utf8ToWide(customPath_).c_str());if(b.GetLastStatus()!=Ok||!b.GetWidth()||!b.GetHeight())return;Rect r(0,0,b.GetWidth(),b.GetHeight());BitmapData d{};if(b.LockBits(&r,ImageLockModeRead,PixelFormat32bppARGB,&d)!=Ok)return;glGenTextures(1,&customTexture_);glBindTexture(GL_TEXTURE_2D,customTexture_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,b.GetWidth(),b.GetHeight(),0,GL_BGRA,GL_UNSIGNED_BYTE,d.Scan0);b.UnlockBits(&d);glBindTexture(GL_TEXTURE_2D,0);}

FFResult BRChristmasParticles::InitGL(const FFGLViewportStruct*v){viewportWidth_=v->width;viewportHeight_=v->height;if(!shader_.Compile(vertexShader,fragmentShader)||!quad_.Initialise())return FF_FAIL;textureDirty_.store(false);rebuildCustomTexture();return CFFGLPlugin::InitGL(v);}
FFResult BRChristmasParticles::ProcessOpenGL(ProcessOpenGLStruct*){if(textureDirty_.exchange(false))rebuildCustomTexture();auto now=std::chrono::steady_clock::now();double dt=std::chrono::duration<double>(now-lastFrame_).count();lastFrame_=now;if(!paused_)elapsed_+=std::min(dt,.1);float sd=.25f+values_[P_START_TIME]*7.75f,ed=.25f+values_[P_STOP_TIME]*7.75f;float sp=stopped_?0:std::min(1.f,float(elapsed_/sd)),ep=0;if(stopping_){ep=std::min(1.f,float((elapsed_-stopStartedAt_)/ed));if(ep>=1){stopping_=false;stopped_=true;}}else if(stopped_)ep=1;float rgb[3];HSVtoRGB(values_[P_HUE],values_[P_SAT],values_[P_BRI],rgb[0],rgb[1],rgb[2]);ScopedShaderBinding b(shader_.GetGLID());auto u=[&](const char*n){return shader_.FindUniform(n);};glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,customTexture_);glUniform1i(u("customTex"),0);glUniform2f(u("resolution"),float(viewportWidth_),float(viewportHeight_));
#define U1(n,p) glUniform1f(u(n),values_[p])
 glUniform1f(u("elapsed"),float(elapsed_));U1("particleMode",P_MODE);U1("particleType",P_TYPE);U1("amount",P_AMOUNT);U1("burstAmount",P_BURST_AMOUNT);U1("particleSize",P_SIZE);U1("sizeRandom",P_SIZE_RANDOM);U1("particleSpeed",P_SPEED);U1("speedRandom",P_SPEED_RANDOM);U1("directionMode",P_DIRECTION);U1("gravity",P_GRAVITY);U1("turbulence",P_TURBULENCE);U1("rotationSpeed",P_ROTATION);U1("centerX",P_POS_X);glUniform1f(u("centerY"),1-values_[P_POS_Y]);U1("opacityValue",P_OPACITY);glUniform3f(u("particleColor"),rgb[0],rgb[1],rgb[2]);U1("glowAmount",P_GLOW);U1("trailAmount",P_TRAIL);U1("dofEnabled",P_DOF);U1("focusLayer",P_FOCUS);U1("dofBlur",P_DOF_BLUR);U1("quality",P_QUALITY);U1("randomSeed",P_SEED);glUniform1f(u("startProgress"),sp);glUniform1f(u("stopProgress"),ep);glUniform1f(u("burstAge"),float(elapsed_-burstStartedAt_));glUniform1f(u("hasCustom"),customTexture_?1.f:0.f);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);quad_.Draw();glDisable(GL_BLEND);glBindTexture(GL_TEXTURE_2D,0);return FF_SUCCESS;}
FFResult BRChristmasParticles::DeInitGL(){releaseCustomTexture();shader_.FreeGLResources();quad_.Release();return FF_SUCCESS;}
FFResult BRChristmasParticles::SetFloatParameter(unsigned i,float v){if(i>=P_COUNT)return FF_FAIL;values_[i]=v;if(i==P_LOAD_IMAGE&&v)chooseCustomImage();if(i==P_START&&v){elapsed_=0;paused_=stopping_=stopped_=false;burstStartedAt_=-100;lastFrame_=std::chrono::steady_clock::now();}if(i==P_PAUSE&&v)paused_=true;if(i==P_RESUME&&v&&!stopped_){paused_=false;lastFrame_=std::chrono::steady_clock::now();}if(i==P_STOP&&v&&!stopped_&&!stopping_){paused_=false;stopping_=true;stopStartedAt_=elapsed_;}if(i==P_RESET&&v){elapsed_=0;paused_=stopping_=stopped_=false;burstStartedAt_=-100;}if(i==P_BURST&&v){burstStartedAt_=elapsed_;}return FF_SUCCESS;}
float BRChristmasParticles::GetFloatParameter(unsigned i){return i<P_COUNT?values_[i]:0;}
FFResult BRChristmasParticles::SetTextParameter(unsigned i,const char*v){if(i==P_IMAGE_PATH){customPath_=v?v:"";textureDirty_.store(true);return FF_SUCCESS;}return i==P_CREDIT?FF_SUCCESS:FF_FAIL;}
char* BRChristmasParticles::GetTextParameter(unsigned i){static thread_local char b[32768]{};std::string v=i==P_IMAGE_PATH?customPath_:(i==P_CREDIT?credit_:"");size_t n=std::min(v.size(),sizeof(b)-1);memcpy(b,v.data(),n);b[n]=0;return b;}

