
#include "FaceRecognition.h"
//#include "GL/glew.h"

#include <string>
#include <vector>
#include <fstream>

#include "HMD.cpp"
#include "CAMERA.cpp"
#include "WindowHandler.cpp"

#include <thread>
#include <chrono>  





GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path);
void cleanup();
void generateTexture(GLuint texture, unsigned char* data);

using namespace OVR;



GLuint frameBuffer;
GLuint renderBuffer;
GLuint texture;

GLuint vertexArray;
GLuint positionBuffer;
GLuint uvbuffer;
GLuint program;



GLuint textureCam;

unsigned char* data[2];
unsigned char jpgimage[100000];
HMD* hmd;
CAMERA* camera;




WindowHandler windowHandler;
int main(int argc, char *argv[])
{
	
	windowHandler.init();
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	bool debug = false;

	hmd = new HMD(debug, flags);

	camera = new CAMERA(hmd, debug);
		
	if (debug == false && hmd->getDevice()->HmdCaps & ovrHmdCap_ExtendDesktop)
	{
		windowHandler.setX(hmd->getDevice()->WindowsPos.x);
		windowHandler.setY(hmd->getDevice()->WindowsPos.y);
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	

	windowHandler.createWindow(hmd, flags);

	windowHandler.createContext();

	
	
	//render = new RENDER(hmd, windowHandler.getWindowP(),windowHandler.getContext());
	glewExperimental = GL_TRUE;

	glewInit();

	glGenFramebuffers(1, &frameBuffer);

	glGenTextures(1, &texture);
	//glGenTextures(1, &textureCam);

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

		SDL_GL_DeleteContext(windowHandler.getContext());

		SDL_DestroyWindow(windowHandler.getWindowP());

		ovrHmd_Destroy(hmd->getDevice());

		ovr_Shutdown();

		SDL_Quit();

		printf("*** ERROR *** Could not create OpenGL buffers");
	}


	hmd->createEyeTextures(texture);

	hmd->configureWindow(windowHandler.getInfo());
	
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
	/*
	GLfloat uvvertices[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,

		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f
	};
	*/
	//TODO:Rustam WHY the HACK it appeared reversed after some magic
	GLfloat uvvertices[] = {
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f
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
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
				case SDLK_SPACE:
					camera->storeCamImages();
					break;
				case SDLK_r:
					camera->storeCamImage(0,"./CamData/recognize.jpeg");
					std::thread(faceRecognition, "./CamData/recognize.jpeg" ,1).detach();
					//std::thread(threadfunc, "hi").detach();
					break;
				default:
					break;
				}
			default:
				break;
			}
		}

		hmd->beginFrame();
		
		float curDelta = hmd->getTiming().DeltaSeconds;
		
		while (curDelta < 0.016666) 
		{
			Sleep(1);
			hmd->beginFrame();
			curDelta += hmd->getTiming().DeltaSeconds;
		}
		
		//printf("Time since last frame %2.8f\n", 1/curDelta);
		hmd->getEyePoses();
		

		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// ================= CREATE TEXTURE ===========
		// Create one OpenGL texture
		camera->getDevice()->PreStoreCamData();	//renderer
		data[OVR::OV_CAMEYE_LEFT] = camera->getCamImageLeft();
		data[OVR::OV_CAMEYE_RIGHT] = camera->getCamImageRight();
				
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			ovrEyeType eye = hmd->getDevice()->EyeRenderOrder[eyeIndex];
			//glGenTextures(1, &textureCam); //TODO:Rustam This was causing memory drain. WORKS WITHOUR glGenTextures???
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
			glActiveTexture(GL_TEXTURE0); //TODO:Rustam what are these lines doing?
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
		//hmd->endFrame();
		ovrHmd_EndFrame(hmd->getDevice(), hmd->getEyeRenderPose(), &hmd->getEyeTexture()[0].Texture);

		glBindVertexArray(vertexArray);
	}

	cleanup(); //TODO:Rustam Destructors are not called?
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

	
	windowHandler.cleanup();
	hmd->cleanup();
	//camera->cleanup();
	
	windowHandler.quit();
	
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

