#pragma once
#include "particlemesh.h"

// External Includes
#include <renderablemesh.h>
#include <rendercomponent.h>
#include <transformcomponent.h>
#include <componentptr.h>
#include <uniforminstance.h>
#include <materialinstance.h>

namespace nap
{
	class PointSpriteVolumeInstance;

	class PointSpriteVolume : public RenderableComponent
	{
		RTTI_ENABLE(RenderableComponent)
		DECLARE_COMPONENT(PointSpriteVolume, PointSpriteVolumeInstance)

	public:
		MaterialInstanceResource mMaterialInstanceResource;
		ComponentPtr<TransformComponent> mCameraTransform;

		float mPointSize = 100.0f;
		float mVolumeScale = 1.0f;
		float mTimeScale = 0.0625f;
		float mVariationScale = 1.0f;
		unsigned int mVolumeSize = 16;
		glm::ivec2 mSpriteSheetDims = {4, 4};
	};



	class PointSpriteVolumeInstance : public RenderableComponentInstance
	{
		RTTI_ENABLE(RenderableComponentInstance)

	public:
		PointSpriteVolumeInstance(EntityInstance& entity, Component& resource);
		virtual bool init(nap::utility::ErrorState& errorState) override;
		virtual void update(double deltaTime) override;

	protected:
		/**
		* Draws a volume of point sprites 
		* @param viewMatrix the camera world space location
		* @param projectionMatrix the camera projection matrix
		*/
		virtual void onDraw(IRenderTarget& renderTarget, VkCommandBuffer commandBuffer, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) override;

	public:
		MaterialInstance& getMaterial();
		int getNumParticles();

		ComponentInstancePtr<TransformComponent> mCameraTransform = { this, &PointSpriteVolume::mCameraTransform };

		float mPointSize = 100.0f;
		float mVolumeScale = 1.0f;
		float mTimeScale = 0.0625f;
		float mVariationScale = 1.0f;
		unsigned int mVolumeSize = 16;

	private:
		TransformComponentInstance* mTransform = nullptr;

		std::unique_ptr<ParticleMesh> mParticleMesh;
		RenderableMesh mRenderableMesh;

		MaterialInstance mMaterialInstance;

		nap::UniformMat4Instance* mModelUniform = nullptr;
		nap::UniformMat4Instance* mViewUniform = nullptr;
		nap::UniformMat4Instance* mProjectionUniform = nullptr;
		nap::UniformVec2Instance* mSpriteSheetDimsUniform = nullptr;
		nap::UniformVec3Instance* mCameraPositionUniform = nullptr;
		nap::UniformFloatInstance* mPointSizeUniform = nullptr;
		nap::UniformFloatInstance* mVolumeScaleUniform = nullptr;
		nap::UniformFloatInstance* mTimeScaleUniform = nullptr;
		nap::UniformFloatInstance* mVariationScaleUniform = nullptr;
		nap::UniformFloatInstance* mTimeUniform = nullptr;
		nap::UniformIntInstance* mVolumeSizeUniform = nullptr;

		double mTime = 0.0;
		int mNumParticles = 0;

		glm::vec2 mSpriteSheetDims;

		RenderService* mRenderService = nullptr;
	};
}
