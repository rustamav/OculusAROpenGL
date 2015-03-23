#define GLEW_STATIC
#include "FaceRecognition.h"
#include "GL/glew.h"
#include <ovrvision.h>        //Ovrvision SDK
#include <string>
#include <vector>
#include <fstream>
// Uncomment your platform
#define OVR_OS_WIN32
//#define OVR_OS_MAC
//#define OVR_OS_LINUX


#include "OVR_CAPI_GL.h"
#include "Kernel/OVR_Math.h"
#include "SDL.h"
#include "SDL_syswm.h"




GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path);
void cleanup();
void generateTexture(GLuint texture, unsigned char* data);

using namespace OVR;


SDL_Window *window;

SDL_GLContext context;


GLuint vertexArray;
GLuint positionBuffer;
GLuint uvbuffer;
GLuint program;
GLuint frameBuffer;
GLuint texture;
GLuint renderBuffer;

GLuint textureCam;

unsigned char* data[2];
unsigned char jpgimage[100000];
ovrFrameTiming ovrTiming;

class HMD {
private:
	ovrHmd hmd;
	Sizei renderTargetSize;
	ovrRecti eyeRenderViewport[2];
	ovrGLTexture eyeTexture[2];
	ovrFovPort eyeFov[2];
	ovrEyeRenderDesc eyeRenderDesc[2];
public:
	HMD(){}
	~HMD(){
		printf("~~~ HMD is deleted");
	}
	HMD(bool& isDebug, Uint32& flags){
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
	void createEyeTextures(){
		// ovrFovPort eyeFov[2] = { hmd->getDevice()->DefaultEyeFov[0], hmd->getDevice()->DefaultEyeFov[1] }; //TODO:Rustam Why this work and other does not

		eyeFov[0] =  hmd->DefaultEyeFov[0];
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
class CAMERA{
private:
	OVR::Ovrvision* g_pOvrvision;
	int processer_quality;
public:
	CAMERA(HMD* hmd, bool isDebug) {
		g_pOvrvision = new OVR::Ovrvision();
		int processer_quality = OVR::OV_PSQT_HIGH;
		if (isDebug){
		g_pOvrvision->Open(0, OVR::OV_CAMVGA_FULL);  //Open
		}
		else if (hmd->getDevice()->Type == ovrHmd_DK2) {
			//Rift DK2
			g_pOvrvision->Open(0, OVR::OV_CAMVGA_FULL);  //Open
		}
		else {
			//Rift DK1
			g_pOvrvision->Open(0, OVR::OV_CAMVGA_FULL, OVR::OV_HMD_OCULUS_DK1);  //Open
		}
	}
	OVR::Ovrvision* getDevice(){
		return g_pOvrvision;
		
	}
	void preStoreCamData() {
		g_pOvrvision->PreStoreCamData();	//renderer
	}
	unsigned char* getCamImageLeft() {
		return g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_LEFT, (OvPSQuality)processer_quality);
	}
	unsigned char* getCamImageRight() {
		return g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_RIGHT, (OvPSQuality)processer_quality);
	}
	
	

};
HMD* hmd;
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	int x = SDL_WINDOWPOS_CENTERED;
	int y = SDL_WINDOWPOS_CENTERED;
	
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	bool debug = false;

	ovr_Initialize();
	hmd = new HMD(debug, flags);

	CAMERA* camera = new CAMERA(hmd, debug);
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
	if (debug == false && hmd->getDevice()->HmdCaps & ovrHmdCap_ExtendDesktop)
	{
		x = hmd->getDevice()->WindowsPos.x;
		y = hmd->getDevice()->WindowsPos.y;
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	window = SDL_CreateWindow("Oculus Rift SDL2 OpenGL Demo", x, y, hmd->getResolution().w, hmd->getResolution().h, flags);

	context = SDL_GL_CreateContext(window);

	glewExperimental = GL_TRUE;

	glewInit();
	
	glGenFramebuffers(1, &frameBuffer);

	glGenTextures(1, &texture);

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hmd->getRenderTargetSize().w, hmd->getRenderTargetSize().h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, hmd->getRenderTargetSize().w, hmd->getRenderTargetSize().h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		glDeleteFramebuffers(1, &frameBuffer);
		glDeleteTextures(1, &texture);
		glDeleteRenderbuffers(1, &renderBuffer);

		SDL_GL_DeleteContext(context);

		SDL_DestroyWindow(window);

		ovrHmd_Destroy(hmd->getDevice());

		ovr_Shutdown();

		SDL_Quit();

		return 0;
	}


	hmd->createEyeTextures();
	

	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);

	SDL_GetWindowWMInfo(window, &info);

	hmd->configureWindow(info);
	
	program = LoadShaders("Vertex2.shader", "Fragment2.shader");
	glLinkProgram(program);
	glUseProgram(program);

	GLuint MVPMatrixLocation = glGetUniformLocation(program, "MVPMatrix");
	GLuint positionLocation = glGetAttribLocation(program, "position");
	GLuint uvlocation = glGetAttribLocation(program, "uvcoord");
	GLuint TextureID = glGetUniformLocation(program, "myTextureSampler");

	
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	GLfloat vertices[] = {
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f

	};
	
	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionLocation);

	//========== CREATE TEXTURE ===============
	GLfloat uvvertices[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,

		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f
	};
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvvertices), uvvertices, GL_STATIC_DRAW);
	glVertexAttribPointer(uvlocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(uvlocation);

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
			
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				ovrHmd_DismissHSWDisplay(hmd->getDevice());

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

		ovrTiming = ovrHmd_BeginFrame(hmd->getDevice(), 0);
		float curDelta = ovrTiming.DeltaSeconds;
		
		while (curDelta < 0.016) 
		{
			Sleep(1);
			ovrTiming = ovrHmd_BeginFrame(hmd->getDevice(), 0);
			curDelta += ovrTiming.DeltaSeconds;
		}
		
		printf("Time since last frame %2.8f\n", 1/curDelta);

		ovrVector3f hmdToEyeViewOffset[2] = { hmd->getEyeRenderDesc()[0].HmdToEyeViewOffset, hmd->getEyeRenderDesc()[1].HmdToEyeViewOffset };

		ovrPosef eyeRenderPose[2];

		ovrHmd_GetEyePoses(hmd->getDevice(), 0, hmdToEyeViewOffset, eyeRenderPose, NULL);

		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// ================= CREATE TEXTURE ===========
		// Create one OpenGL texture
		//Sleep(30);
		camera->getDevice()->PreStoreCamData();	//renderer
		//g_pOvrvision->GetCamImageMJPEG(pImageBuf, &pSize, OV_CAMEYE_LEFT);
		data[OVR::OV_CAMEYE_LEFT] = camera->getCamImageLeft();
		data[OVR::OV_CAMEYE_RIGHT] = camera->getCamImageRight();
		//int p = 3 * 640 * 480;
		
		//g_pOvrvision->GetCamImageMJPEG(jpgimage,&p ,OVR::OV_CAMEYE_LEFT);

		//FILE *pa = fopen("testJPEG", "wb");
		//FILE *pb = fopen("testRAW", "wb");
		//fwrite(jpgimage, 1, p, pa);
		//fwrite(data[0], 1, p, pb);
		//fclose(pa);
		//fclose(pb);
		//delete jpgimage;
		//printf("Prediction is : %d",faceRecognition("new.jpg",2));
	
		//GLuint textureCam = loadBMP_custom("test.bmp");
		
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			ovrEyeType eye = hmd->getDevice()->EyeRenderOrder[eyeIndex];
			glGenTextures(1, &textureCam);
			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, textureCam);

			// Give the image to OpenGL
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, data[eye]);

//			delete[] data[eye];

			// ... nice trilinear filtering.
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(GL_TEXTURE_2D);
			// ========= /CREATE TEXTURE ==============
			// Use our shader
			glUseProgram(program);
			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureCam);
			// Set our "myTextureSampler" sampler to user Texture Unit 0
			glUniform1i(TextureID, 0);
			// ================= CREATE TEXTURE ===========

			// MVPMatrix = Matrix4f(ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.01f, 10000.0f, true)) * Matrix4f(Quatf(eyeRenderPose[eye].Orientation).Inverted()) * Matrix4f::Translation(-Vector3f(eyeRenderPose[eye].Position));

			Matrix4f MVPMatrix = OVR::Matrix4f::Identity();

			glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, &MVPMatrix.Transposed().M[0][0]);

			glViewport(hmd->getEyeRenderViewport()[eye].Pos.x, hmd->getEyeRenderViewport()[eye].Pos.y, hmd->getEyeRenderViewport()[eye].Size.w, hmd->getEyeRenderViewport()[eye].Size.h);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		glBindVertexArray(0);

		ovrHmd_EndFrame(hmd->getDevice(), eyeRenderPose, &hmd->getEyeTexture()[0].Texture);

		glBindVertexArray(vertexArray);
	}

	cleanup(); //TODO:Rustam Destructors are not called?
	//system("pause");
	return 0;
}

void cleanup() {
	glDeleteVertexArrays(1, &vertexArray);
	glDeleteBuffers(1, &positionBuffer);
	glDeleteBuffers(1, &uvbuffer);

	//glDeleteShader(vertexShader);
	//glDeleteShader(fragmentShader);
	glDeleteProgram(program);

	glDeleteFramebuffers(1, &frameBuffer);
	glDeleteTextures(1, &texture);
	glDeleteTextures(1, &textureCam);
	glDeleteRenderbuffers(1, &renderBuffer);

	SDL_GL_DeleteContext(context);

	SDL_DestroyWindow(window);

	hmd->cleanup();
	
	SDL_Quit();
}
void generateTexture(GLuint texture, unsigned char *data)
{
	glGenTextures(1, &texture);
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, texture);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	//		delete[] data;

	// ... nice trilinear filtering.
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
} //TODO:Rustam pass texture by reference and it should work
GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()){
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()){
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}



	GLint Result = GL_FALSE;
	int InfoLogLength;



	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0){
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}