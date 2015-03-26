#ifndef _WINDOWHANDLER__
#define _WINDOWHANDLER__
#include "SDL.h"
#include "SDL_syswm.h"
#include "HMD.cpp"
class WindowHandler {
private:
	int x;
	int y;
	SDL_GLContext context;
	SDL_SysWMinfo info;
	SDL_Window *window;

public:
	void init(){
		SDL_Init(SDL_INIT_VIDEO);
		x = SDL_WINDOWPOS_CENTERED;
		y = SDL_WINDOWPOS_CENTERED;
	}
	void createWindow(HMD* hmd, Uint32 flags) {
		window = SDL_CreateWindow("Oculus Rift SDL2 OpenGL Demo", getX(), getY(), hmd->getResolution().w, hmd->getResolution().h, flags);
	}
	void createContext(){
		context = SDL_GL_CreateContext(window);
	}
	void createWindowInfo(){

		SDL_VERSION(&info.version);

		SDL_GetWindowWMInfo(window, &info);
	}
	SDL_SysWMinfo getInfo(){
		return info;
	}
	SDL_GLContext getContext(){
		return context;
	}
	void quit(){
		SDL_Quit();
	}
	SDL_Window* getWindowP(){
		return window;
	}
	int getX(){
		return x;
	}
	int getY(){
		return y;
	}
	void setX(int xPos){
		x = xPos;
	}
	void setY(int yPos){
		y = yPos;
	}
	void cleanup(){
		SDL_GL_DeleteContext(getContext());

		SDL_DestroyWindow(window);
	}
};
#endif