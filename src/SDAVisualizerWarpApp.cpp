
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
//#include "cinder/gl/Texture.h"
//#include "cinder/ImageIo.h"
//#include "cinder/Rand.h"

#include "Warp.h"
// Settings
#include "SDASettings.h"
// Session
#include "SDASession.h"
// Log
#include "SDALog.h"
// Spout
#include "CiSpoutIn.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;
using namespace SophiaDigitalArt;

class SDAVisualizerWarpApp : public App {
public:
	static void prepare(Settings *settings);

	void setup() override;
	void cleanup() override;
	void update() override;
	void draw() override;

	void resize() override;

	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;

	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;

	void setUIVisibility(bool visible);
private:
	fs::path		mSettings;

	WarpList		mWarps;

	// Settings
	SDASettingsRef					mSDASettings;
	// Session
	SDASessionRef					mSDASession;
	// Log
	SDALogRef						mSDALog;
	// Spout
	SpoutIn	mSpoutIn;
	// fbo
	bool							mIsShutDown;
	Anim<float>						mRenderWindowTimer;
	void							positionRenderWindow();
	bool							mFadeInDelay;
};

void SDAVisualizerWarpApp::prepare(Settings *settings)
{
	settings->setWindowSize(1440, 900);
}

void SDAVisualizerWarpApp::setup()
{
	// Settings
	mSDASettings = SDASettings::create();
	// Session
	mSDASession = SDASession::create(mSDASettings);
	//mSDASettings->mCursorVisible = true;
	setUIVisibility(mSDASettings->mCursorVisible);
	mSDASession->getWindowsResolution();

	mFadeInDelay = true;
	// windows
	mIsShutDown = false;
	mRenderWindowTimer = 0.0f;
	timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });

	disableFrameRate();

	// initialize warps
	mSettings = getAssetPath("") / "warps.xml";
	if (fs::exists(mSettings)) {
		// load warp settings from file if one exists
		mWarps = Warp::readSettings(loadFile(mSettings));
	}
	else {
		// otherwise create a warp from scratch
		mWarps.push_back(WarpPerspectiveBilinear::create());
	}

}
void SDAVisualizerWarpApp::positionRenderWindow() {
	mSDASettings->mRenderPosXY = ivec2(mSDASettings->mRenderX, mSDASettings->mRenderY);//20141214 was 0
	setWindowPos(mSDASettings->mRenderX, mSDASettings->mRenderY);
	setWindowSize(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight);
}
void SDAVisualizerWarpApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void SDAVisualizerWarpApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		// save warp settings
		Warp::writeSettings(mWarps, writeFile(mSettings));
		// save settings
		mSDASettings->save();
		mSDASession->save();
		quit();
	}
}

void SDAVisualizerWarpApp::update()
{
	mSDASession->setFloatUniformValueByIndex(mSDASettings->IFPS, getAverageFps());
	mSDASession->update();
}

void SDAVisualizerWarpApp::draw()
{
	gl::clear(Color::black());
	if (mFadeInDelay) {
		mSDASettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mSDASession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mSDASettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	auto tex = mSpoutIn.receiveTexture();
	if (tex) {
		// Otherwise draw the texture and fill the screen
		for (auto &warp : mWarps) {
			warp->draw(tex, tex->getBounds());
		}
		// Show the user what it is receiving
		gl::ScopedBlendAlpha alpha;
		gl::enableAlphaBlending();
		gl::drawString("Receiving from: " + mSpoutIn.getSenderName(), vec2(toPixels(20), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
		gl::drawString("fps: " + std::to_string((int)getAverageFps()), vec2(getWindowWidth() - toPixels(100), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
		gl::drawString("RH click to select a sender", vec2(toPixels(20), getWindowHeight() - toPixels(40)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
	}
	else {
		gl::ScopedBlendAlpha alpha;
		gl::enableAlphaBlending();
		gl::drawString("No sender/texture detected", vec2(toPixels(20), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
	}
	getWindow()->setTitle(mSDASettings->sFps + " fps SDA");
}

void SDAVisualizerWarpApp::resize()
{
	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarps);
}

void SDAVisualizerWarpApp::mouseMove(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarps, event)) {
		// let your application perform its mouseMove handling here
	}
}

void SDAVisualizerWarpApp::mouseDown(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarps, event)) {
		// let your application perform its mouseDown handling here
		if (event.isRightDown()) {
			// Select a sender
			// SpoutPanel.exe must be in the executable path
			mSpoutIn.getSpoutReceiver().SelectSenderPanel(); // DirectX 11 by default
		}
	}
}

void SDAVisualizerWarpApp::mouseDrag(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDrag(mWarps, event)) {
		// let your application perform its mouseDrag handling here
	}
}

void SDAVisualizerWarpApp::mouseUp(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseUp(mWarps, event)) {
		// let your application perform its mouseUp handling here
	}
}

void SDAVisualizerWarpApp::keyDown(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarps, event)) {
		// warp editor did not handle the key, so handle it here
		switch (event.getCode()) {
		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_f:
			// toggle full screen
			setFullScreen(!isFullScreen());
			break;
		case KeyEvent::KEY_v:
			// toggle vertical sync
			gl::enableVerticalSync(!gl::isVerticalSyncEnabled());
			break;
		case KeyEvent::KEY_w:
			// toggle warp edit mode
			Warp::enableEditMode(!Warp::isEditModeEnabled());
			break;
		case KeyEvent::KEY_h:
			// mouse cursor and ui visibility
			mSDASettings->mCursorVisible = !mSDASettings->mCursorVisible;
			setUIVisibility(mSDASettings->mCursorVisible);
			break;

		}
	}
}

void SDAVisualizerWarpApp::keyUp(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyUp(mWarps, event)) {
		// let your application perform its keyUp handling here
	}
}

CINDER_APP(SDAVisualizerWarpApp, RendererGl(RendererGl::Options().msaa(8)), &SDAVisualizerWarpApp::prepare)
