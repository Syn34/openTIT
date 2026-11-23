// openTIT - Loading screen thread (spinning icon shown during boot and loads).
#pragma once
namespace sly {
extern bool g_loadingScreenRequest;   // 0x00F9CDE2: show the loading screen next frame
void LoadingScreenStart(bool firstBoot);
void LoadingScreenStop();
}
