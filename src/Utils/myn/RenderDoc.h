#include <string>
class RENDERDOC_API_1_4_1;

namespace myn
{
class RenderDoc
{
public:

	static bool load(const std::string &appName);

	static void captureNextFrame();

	static void potentiallyStartCapture();

	static void potentiallyEndCapture();

	static void showOverlay();

	static void hideOverlay();

	static void toggleOverlay();

private:

	static RENDERDOC_API_1_4_1 *api;
	static bool shouldCaptureFrame;
};
}// namespace myn