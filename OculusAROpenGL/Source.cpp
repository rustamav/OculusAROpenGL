#define GLEW_STATIC
#include "GL/glew.h"
#include <ovrvision.h>        //Ovrvision SDK
#include <fstream>
// Uncomment your platform
#define OVR_OS_WIN32
//#define OVR_OS_MAC
//#define OVR_OS_LINUX
#include "OVR_CAPI_GL.h"
#include "Kernel/OVR_Math.h"
#include "SDL.h"
#include "SDL_syswm.h"

using namespace OVR;
int processer_quality = OVR::OV_PSQT_HIGH;
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	int x = SDL_WINDOWPOS_CENTERED;
	int y = SDL_WINDOWPOS_CENTERED;
	OVR::Vector3f translate = OVR::Vector3f(0.0f, 0.0f, 0.0f);

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	bool debug = false;

	ovr_Initialize();

	ovrHmd hmd = ovrHmd_Create(0);

	OVR::Ovrvision* g_pOvrvision;

	if (hmd == NULL)
	{
		hmd = ovrHmd_CreateDebug(ovrHmd_DK1);

		debug = true;
	}

	
	//g_pOvrvision = new OVR::Ovrvision();
	//if (hmd->Type == ovrHmd_DK2) {
		//Rift DK2
	//	g_pOvrvision->Open(0, OVR::OV_CAMVGA_FULL);  //Open
	//}
	//else {
	//	//Rift DK1
	//	g_pOvrvision->Open(0, OVR::OV_CAMVGA_FULL, OVR::OV_HMD_OCULUS_DK1);  //Open
	//}
	

	// Get ovrvision image
	//unsigned char pImageBuf[100000]; //TO_DORustam: WTF
	//int pSize = 0;
	//g_pOvrvision->PreStoreCamData();	//renderer
	//g_pOvrvision->GetCamImageMJPEG(pImageBuf, &pSize, OV_CAMEYE_LEFT);
	//unsigned char* p = g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_LEFT, (OvPSQuality)processer_quality);

	//std::ofstream outfile("new.jpeg", std::ofstream::binary);
	//outfile.write((const char*)pImageBuf, pSize);

	//unsigned char* p2 = g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_RIGHT, (OvPSQuality)processer_quality);
	//outfile.close();
//	delete pImageBuf;
	

	if (debug == false && hmd->HmdCaps & ovrHmdCap_ExtendDesktop)
	{
		x = hmd->WindowsPos.x;
		y = hmd->WindowsPos.y;
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	int w = hmd->Resolution.w;
	int h = hmd->Resolution.h;

	SDL_Window *window = SDL_CreateWindow("Oculus Rift SDL2 OpenGL Demo", x, y, w, h, flags);

	SDL_GLContext context = SDL_GL_CreateContext(window);

	glewExperimental = GL_TRUE;

	glewInit();

	Sizei recommendedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmd->DefaultEyeFov[0], 1.0f);
	Sizei recommendedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmd->DefaultEyeFov[1], 1.0f);
	Sizei renderTargetSize;
	renderTargetSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
	renderTargetSize.h = max(recommendedTex0Size.h, recommendedTex1Size.h);

	GLuint frameBuffer;
	glGenFramebuffers(1, &frameBuffer);

	GLuint texture;
	glGenTextures(1, &texture);

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderTargetSize.w, renderTargetSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

	GLuint renderBuffer;
	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, renderTargetSize.w, renderTargetSize.h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		glDeleteFramebuffers(1, &frameBuffer);
		glDeleteTextures(1, &texture);
		glDeleteRenderbuffers(1, &renderBuffer);

		SDL_GL_DeleteContext(context);

		SDL_DestroyWindow(window);

		ovrHmd_Destroy(hmd);

		ovr_Shutdown();

		SDL_Quit();

		return 0;
	}

	ovrFovPort eyeFov[2] = { hmd->DefaultEyeFov[0], hmd->DefaultEyeFov[1] };

	ovrRecti eyeRenderViewport[2];
	eyeRenderViewport[0].Pos = Vector2i(0, 0);
	eyeRenderViewport[0].Size = Sizei(renderTargetSize.w / 2, renderTargetSize.h);
	eyeRenderViewport[1].Pos = Vector2i((renderTargetSize.w + 1) / 2, 0);
	eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;

	ovrGLTexture eyeTexture[2];
	eyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
	eyeTexture[0].OGL.Header.TextureSize = renderTargetSize;
	eyeTexture[0].OGL.Header.RenderViewport = eyeRenderViewport[0];
	eyeTexture[0].OGL.TexId = texture;

	eyeTexture[1] = eyeTexture[0];
	eyeTexture[1].OGL.Header.RenderViewport = eyeRenderViewport[1];

	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);

	SDL_GetWindowWMInfo(window, &info);

	ovrGLConfig cfg;
	cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	cfg.OGL.Header.RTSize = Sizei(hmd->Resolution.w, hmd->Resolution.h);
	cfg.OGL.Header.Multisample = 1;
#if defined(OVR_OS_WIN32)
	if (!(hmd->HmdCaps & ovrHmdCap_ExtendDesktop))
		ovrHmd_AttachToWindow(hmd, info.info.win.window, NULL, NULL);

	cfg.OGL.Window = info.info.win.window;
	cfg.OGL.DC = NULL;
#elif defined(OVR_OS_LINUX)
	cfg.OGL.Disp = info.info.x11.display;
	cfg.OGL.Win = info.info.x11.window;
#endif

	ovrEyeRenderDesc eyeRenderDesc[2];

	ovrHmd_ConfigureRendering(hmd, &cfg.Config, ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive, eyeFov, eyeRenderDesc);

	ovrHmd_SetEnabledCaps(hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

	ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);

	const GLchar *vertexShaderSource[] = {
		"#version 150\n"
		"uniform mat4 MVPMatrix;\n"
		"in vec3 position;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = MVPMatrix * vec4(position, 1.0);\n"
		"}"
	};

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	const GLchar *fragmentShaderSource[] = {
		"#version 150\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"    outputColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
		"}"
	};

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glUseProgram(program);

	GLuint MVPMatrixLocation = glGetUniformLocation(program, "MVPMatrix");
	GLuint positionLocation = glGetAttribLocation(program, "position");

	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	GLfloat vertices[] = {
		-1.0f, 1.0f, -2.0f,
		-1.0f, -1.0f, -2.0f,
		1.0f, 1.0f, -2.0f,

		1.0f, -1.0f, -2.0f,
		-1.0f, -1.0f, -2.0f,
		1.0f, 1.0f, -2.0f
		
	};

	GLuint positionBuffer;
	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionLocation);

	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClearDepth(1.0f);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	bool running = true;

	while (running == true)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDLK_UP:
				translate += OVR::Vector3f(0.0f, 0.1f, 0.0f);
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				ovrHmd_DismissHSWDisplay(hmd);

				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					running = false;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}

		ovrHmd_BeginFrame(hmd, 0);

		ovrVector3f hmdToEyeViewOffset[2] = { eyeRenderDesc[0].HmdToEyeViewOffset, eyeRenderDesc[1].HmdToEyeViewOffset };

		ovrPosef eyeRenderPose[2];

		ovrHmd_GetEyePoses(hmd, 0, hmdToEyeViewOffset, eyeRenderPose, NULL);

		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			ovrEyeType eye = hmd->EyeRenderOrder[eyeIndex];

			Matrix4f MVPMatrix = Matrix4f(ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.01f, 10000.0f, true)) * Matrix4f(Quatf(eyeRenderPose[eye].Orientation).Inverted()) * Matrix4f::Translation(-Vector3f(eyeRenderPose[eye].Position));

			glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, &MVPMatrix.Transposed().M[0][0]);

			glViewport(eyeRenderViewport[eye].Pos.x, eyeRenderViewport[eye].Pos.y, eyeRenderViewport[eye].Size.w, eyeRenderViewport[eye].Size.h);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		glBindVertexArray(0);

		ovrHmd_EndFrame(hmd, eyeRenderPose, &eyeTexture[0].Texture);

		glBindVertexArray(vertexArray);
	}

	glDeleteVertexArrays(1, &vertexArray);
	glDeleteBuffers(1, &positionBuffer);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteProgram(program);

	glDeleteFramebuffers(1, &frameBuffer);
	glDeleteTextures(1, &texture);
	glDeleteRenderbuffers(1, &renderBuffer);

	SDL_GL_DeleteContext(context);

	SDL_DestroyWindow(window);

	ovrHmd_Destroy(hmd);

	ovr_Shutdown();

	SDL_Quit();

	return 0;
}