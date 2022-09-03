#ifdef WINOS
#include <windows.h>
#include <renderdoc/renderdoc_app.h>
#include <libloaderapi.h>
#endif

#include "RenderDoc.h"
#include "Log.h"

namespace myn
{

RENDERDOC_API_1_5_0* RenderDoc::api = nullptr;
bool RenderDoc::shouldCaptureFrame = false;

bool RenderDoc::load(const std::string &appName)
{
#ifdef WINOS
	//if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
	if (HMODULE mod = LoadLibrary("C:\\Program Files\\RenderDoc\\renderdoc.dll"))
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI =
			(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");

		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void**)&api);
		if (ret != 1) return false;

		api->SetCaptureFilePathTemplate(("renderdoc/" + appName).c_str());
		api->MaskOverlayBits(
			0,
			RENDERDOC_OverlayBits::eRENDERDOC_Overlay_Enabled
			| RENDERDOC_OverlayBits::eRENDERDOC_Overlay_CaptureList
			//| RENDERDOC_OverlayBits::eRENDERDOC_Overlay_FrameNumber
			);
		api->SetCaptureKeys(nullptr, 0);

		LOG("loaded renderdoc. captures will be saved in renderdoc/")

		return true;
	}
	return false;
#else
    LOG("RenderDoc is only supported on windows.")
    return false;
#endif
}

void RenderDoc::captureNextFrame()
{
#ifdef WINOS
	showOverlay();
	shouldCaptureFrame = true;
#endif
}

void RenderDoc::potentiallyStartCapture()
{
#ifdef WINOS
	if (api && shouldCaptureFrame)
	{
		api->StartFrameCapture(nullptr, nullptr);
		shouldCaptureFrame = false;
	}
#endif
}

void RenderDoc::potentiallyEndCapture()
{
#ifdef WINOS
	if (api && api->IsFrameCapturing())
	{
		api->EndFrameCapture(nullptr, nullptr);
		LOG("captured.")

		if (!api->ShowReplayUI()) {
			api->LaunchReplayUI(1, nullptr);
		}
	}
#endif
}

void RenderDoc::showOverlay()
{
#ifdef WINOS
	if (api)
	{
		api->MaskOverlayBits(
			~0,
			RENDERDOC_OverlayBits::eRENDERDOC_Overlay_Enabled |
			RENDERDOC_OverlayBits::eRENDERDOC_Overlay_CaptureList);
	}
#endif
}

void RenderDoc::hideOverlay()
{
#ifdef WINOS
	if (api)
	{
		api->MaskOverlayBits(0, 0);
	}
#endif
}

void RenderDoc::toggleOverlay()
{
#ifdef WINOS
	if (api)
	{
		if (api->GetOverlayBits() & RENDERDOC_OverlayBits::eRENDERDOC_Overlay_Enabled)
			hideOverlay();
		else
			showOverlay();
	}
#endif
}
}

