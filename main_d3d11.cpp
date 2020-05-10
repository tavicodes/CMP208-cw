#include <platform/d3d11/system/platform_d3d11.h>
#include <time.h>
#include "scene_app.h"

unsigned int sceLibcHeapSize = 128*1024*1024;	// Sets up the heap area size as 128MiB.

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	srand(time(NULL()));
	// initialisation with 4:3 (vertical) aspect ratio
	gef::PlatformD3D11 platform(hInstance, 750, 1000, false, true);

	SceneApp myApp(platform);
	myApp.Run();

	return 0;
}