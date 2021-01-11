// Local Includes
#include "spritevolumeapp.h"
#include "pointspritevolume.h"

// External Includes
#include <nap/core.h>
#include <nap/logger.h>
#include <perspcameracomponent.h>
#include <orthocameracomponent.h>
#include <renderablemeshcomponent.h>
#include <utility/fileutils.h>
#include <inputrouter.h>
#include <planemesh.h>
#include <cmath>

namespace nap 
{    
    bool SpriteVolumeApp::init(utility::ErrorState& error)
    {
		// Retrieve services
		mRenderService	= getCore().getService<nap::RenderService>();
		mSceneService	= getCore().getService<nap::SceneService>();
		mInputService	= getCore().getService<nap::InputService>();
		mGuiService		= getCore().getService<nap::IMGuiService>();

		// Fetch the resource manager
        mResourceManager = getCore().getResourceManager();

		// Fetch the scene
		mScene = mResourceManager->findObject<Scene>("Scene");

		// Convert our path and load resources from file
        auto abspath = utility::getAbsolutePath(mFilename);
        nap::Logger::info("Loading: %s", abspath.c_str());
        if (!mResourceManager->loadFile(mFilename, error))
            return false;

		// Get the render window
		mRenderWindow = mResourceManager->findObject<nap::RenderWindow>("Window");
		if (!error.check(mRenderWindow != nullptr, "unable to find render window with name: %s", "Window"))
			return false;

		// Get the scene that contains our entities and components
		mScene = mResourceManager->findObject<Scene>("Scene");
		if (!error.check(mScene != nullptr, "unable to find scene with name: %s", "Scene"))
			return false;

		mDefaultInputRouterEntity = mScene->findEntity("defaultInputRouterEntity");
		mPerspectiveCameraEntity = mScene->findEntity("cameraEntity");
		mOrthoCameraEntity = mScene->findEntity("orthoCameraEntity");
		mWorldEntity = mScene->findEntity("worldEntity");
		mBackgroundEntity = mScene->findEntity("backgroundEntity");

		MaterialInstance& materialInstance = mBackgroundEntity->getComponent<RenderableMeshComponentInstance>().getMaterialInstance();
		mTimeUniform = materialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformFloatInstance>("t");

		// All done!
        return true;
    }


    // Called when the window is going to render
    void SpriteVolumeApp::render()
    {
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording the render commands for the main render window
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin render pass
			mRenderWindow->beginRendering();

			// Background rendering
			std::vector<RenderableComponentInstance*> renderableComponents;
			OrthoCameraComponentInstance& orthoCam = mOrthoCameraEntity->getComponent<OrthoCameraComponentInstance>();

			mTimeUniform->setValue(mTime);
			renderableComponents.emplace_back(&mBackgroundEntity->getComponent<RenderableMeshComponentInstance>());
			mRenderService->renderObjects(*mRenderWindow, orthoCam, renderableComponents);

			// PointTextureVolume rendering
			PerspCameraComponentInstance& cam = mPerspectiveCameraEntity->getComponent<PerspCameraComponentInstance>();

			renderableComponents.clear();
			renderableComponents.emplace_back(&mWorldEntity->getComponent<PointSpriteVolumeInstance>());

			// Render the world with the right camera directly to screen
			mRenderService->renderObjects(*mRenderWindow, cam, renderableComponents);

			// Render GUI elements
			mGuiService->draw();

			// Stop render pass
			mRenderWindow->endRendering();

			// End recording
			mRenderService->endRecording();
		}

		// Proceed to next frame
		mRenderService->endFrame();
    }


    void SpriteVolumeApp::windowMessageReceived(WindowEventPtr windowEvent)
    {
		mRenderService->addEvent(std::move(windowEvent));
    }


    void SpriteVolumeApp::inputMessageReceived(InputEventPtr inputEvent)
    {
		// If we pressed escape, quit the loop
		if (inputEvent->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
		{
			nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(inputEvent.get());
			if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
				quit();

			if (press_event->mKey == nap::EKeyCode::KEY_f)
				mRenderWindow->toggleFullscreen();
		}
		mInputService->addEvent(std::move(inputEvent));
    }


    int SpriteVolumeApp::shutdown()
    {
		return 0;
    }


    void SpriteVolumeApp::update(double deltaTime)
    {
		mTime += deltaTime;

		float radius = 2.0f;
		float t = mTime * 0.5f;

		nap::TransformComponentInstance& trans = mPerspectiveCameraEntity->getComponent<nap::TransformComponentInstance>();
		trans.setTranslate(glm::vec3(sin(t) * radius, cos(t) * radius, 0));

		// Use a default input router to forward input events (recursively) to all input components in the scene
		// This is explicit because we don't know what entity should handle the events from a specific window.
		nap::DefaultInputRouter input_router(true);
		mInputService->processWindowEvents(*mRenderWindow, input_router, { &mScene->getRootEntity() });

		updateGui();
    }

	void SpriteVolumeApp::updateGui()
	{
		// Get component that copies meshes onto target mesh
		PointSpriteVolumeInstance& volume = mWorldEntity->getComponent<PointSpriteVolumeInstance>();
		PerspCameraComponentInstance& cam = mPerspectiveCameraEntity->getComponent<PerspCameraComponentInstance>();
		const glm::vec3& camPos = cam.getEntityInstance()->getComponent<TransformComponentInstance>().getTranslate();

		int volumeSize = volume.mVolumeSize;
		int numParticles = volumeSize*volumeSize*volumeSize;

		// Draw some gui elements
		ImGui::Begin("Controls");
		ImGui::Text(utility::stringFormat("Framerate: %.02f", getCore().getFramerate()).c_str());
		ImGui::Text("Volume ID: %s", volume.mID.c_str());
		ImGui::Text("Cam: (%.02f, %.02f, %.02f)", camPos.x, camPos.y, camPos.z);
		ImGui::Text("NumParticles: %d", numParticles);
		ImGui::SliderFloat("PointSize", &volume.mPointSize, 0.f, 512.f, "%.02f");
		ImGui::SliderFloat("VolumeScale", &volume.mVolumeScale, 0.f, 1.0f, "%.02f");
		ImGui::SliderFloat("TimeScale", &volume.mTimeScale, 0.f, 1.0f, "%.02f");
		ImGui::SliderFloat("VariationScale", &volume.mVariationScale, 0.0f, 2.0f, "%.02f");
		ImGui::End();
	}
}
