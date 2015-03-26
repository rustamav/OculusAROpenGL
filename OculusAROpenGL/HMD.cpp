#ifndef _HMD__
#define _HMD__
#define GLEW_STATIC
// Uncomment your platform
#define OVR_OS_WIN32
//#define OVR_OS_MAC
//#define OVR_OS_LINUX
#include "GL/glew.h"
#include "OVR_CAPI_GL.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include "Kernel/OVR_Math.h"

using namespace OVR;
class HMD {
private:
	ovrHmd hmd;
	Sizei renderTargetSize;
	ovrRecti eyeRenderViewport[2];
	ovrGLTexture eyeTexture[2];
	ovrFovPort eyeFov[2];
	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef eyeRenderPose[2];
	ovrFrameTiming timing;
public:
	HMD(){}
	~HMD(){
		printf("~~~ HMD is deleted");
	}
	HMD(bool& isDebug, Uint32& flags){
		ovr_Initialize();
		hmd = ovrHmd_Create(0);
		if (hmd == NULL)
		{
			hmd = ovrHmd_CreateDebug(ovrHmd_DK1);

			isDebug = true;
		}

		Sizei recommendedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmd->DefaultEyeFov[0], 1.0f);
		Sizei recommendedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmd->DefaultEyeFov[1], 1.0f);

		renderTargetSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
		renderTargetSize.h = max(recommendedTex0Size.h, recommendedTex1Size.h);

	}
	void beginFrame(){
		timing = ovrHmd_BeginFrame(getDevice(), 0);
	}
	void endFrame(){
		ovrHmd_EndFrame(getDevice(), getEyeRenderPose(), &getEyeTexture()[0].Texture);
	}
	void getEyePoses(){
		ovrVector3f hmdToEyeViewOffset[2] = {getEyeRenderDesc()[0].HmdToEyeViewOffset, getEyeRenderDesc()[1].HmdToEyeViewOffset };
		ovrHmd_GetEyePoses(getDevice(), 0, hmdToEyeViewOffset, eyeRenderPose, NULL);
	}
	ovrPosef* getEyeRenderPose(){
		return eyeRenderPose;
	}
	ovrFrameTiming getTiming(){
		return timing;
	}
	ovrHmd getDevice() {
		return hmd;
	}
	Sizei getRenderTargetSize() {
		return renderTargetSize;
	}
	Sizei getResolution() {
		return hmd->Resolution;
	}
	ovrRecti* getEyeRenderViewport(){
		return eyeRenderViewport;
	}
	ovrGLTexture* getEyeTexture(){
		return eyeTexture;
	}
	ovrFovPort* getEyeFov(){
		return eyeFov;
	}
	ovrEyeRenderDesc* getEyeRenderDesc(){
		return eyeRenderDesc;
	}
	void cleanup(){
		ovrHmd_Destroy(hmd);
		ovr_Shutdown();
	}
	void createEyeTextures(const GLuint& texture){
		// ovrFovPort eyeFov[2] = { hmd->getDevice()->DefaultEyeFov[0], hmd->getDevice()->DefaultEyeFov[1] }; //TODO:Rustam Why this work and other does not

		eyeFov[0] = hmd->DefaultEyeFov[0];
		eyeFov[1] = hmd->DefaultEyeFov[1];

		eyeRenderViewport[0].Pos = Vector2i(0, 0);
		eyeRenderViewport[0].Size = Sizei(getRenderTargetSize().w / 2, getRenderTargetSize().h);
		eyeRenderViewport[1].Pos = Vector2i((getRenderTargetSize().w + 1) / 2, 0);
		eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;


		eyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
		eyeTexture[0].OGL.Header.TextureSize = getRenderTargetSize();
		eyeTexture[0].OGL.Header.RenderViewport = eyeRenderViewport[0];
		eyeTexture[0].OGL.TexId = texture;

		eyeTexture[1] = eyeTexture[0];
		eyeTexture[1].OGL.Header.RenderViewport = eyeRenderViewport[1];
	}

	void configureWindow(SDL_SysWMinfo& info){
		ovrGLConfig cfg;
		cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
		cfg.OGL.Header.RTSize = Sizei(getResolution().w, getResolution().h);
		cfg.OGL.Header.Multisample = 1;
#if defined(OVR_OS_WIN32)
		if (!(getDevice()->HmdCaps & ovrHmdCap_ExtendDesktop))
			ovrHmd_AttachToWindow(getDevice(), info.info.win.window, NULL, NULL);

		cfg.OGL.Window = info.info.win.window;
		cfg.OGL.DC = NULL;
#elif defined(OVR_OS_LINUX)
		cfg.OGL.Disp = info.info.x11.display;
		cfg.OGL.Win = info.info.x11.window;
#endif



		ovrHmd_ConfigureRendering(getDevice(), &cfg.Config, ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive, getEyeFov(), eyeRenderDesc);

		ovrHmd_SetEnabledCaps(getDevice(), ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

		ovrHmd_ConfigureTracking(getDevice(), ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
	}
};


#endif