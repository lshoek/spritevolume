// Local Includes
#include "pointspritevolume.h"

// External Includes
#include <entity.h>
#include <renderservice.h>
#include <nap/core.h>
#include <nap/logger.h>
#include <mesh.h>
#include <renderablemeshcomponent.h>
#include <mathutils.h>
#include <indexbuffer.h>
#include <renderglobals.h>

RTTI_BEGIN_CLASS(nap::PointSpriteVolume)
	RTTI_PROPERTY("MaterialInstance", &nap::PointSpriteVolume::mMaterialInstanceResource, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("CameraTransform", &nap::PointSpriteVolume::mCameraTransform, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("PointSize", &nap::PointSpriteVolume::mPointSize, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("VolumeSize", &nap::PointSpriteVolume::mVolumeSize, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("SpriteSheetDims", &nap::PointSpriteVolume::mSpriteSheetDims, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PointSpriteVolumeInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	PointSpriteVolumeInstance::PointSpriteVolumeInstance(EntityInstance& entity, Component& resource) :
		RenderableComponentInstance(entity, resource),
		mParticleMesh(std::make_unique<ParticleMesh>(*entity.getCore())),
		mRenderService(entity.getCore()->getService<RenderService>())
	{ }

	bool PointSpriteVolumeInstance::init(nap::utility::ErrorState& errorState)
	{
		PointSpriteVolume* resource = getComponent<PointSpriteVolume>();
		mTransform = getEntityInstance()->findComponent<TransformComponentInstance>();
		
		if (!errorState.check(mTransform != nullptr, "%s: unable to find transform component", resource->mID.c_str())) {
			return false;
		}
		if (!mMaterialInstance.init(*mRenderService, resource->mMaterialInstanceResource, errorState)) {
			return false;
		}
		mMaterialInstance.setDepthMode(nap::EDepthMode::ReadWrite);

		mProjectionUniform = mMaterialInstance.getOrCreateUniform(uniform::mvpStruct)->getOrCreateUniform<UniformMat4Instance>(uniform::projectionMatrix);
		mViewUniform = mMaterialInstance.getOrCreateUniform(uniform::mvpStruct)->getOrCreateUniform<UniformMat4Instance>(uniform::viewMatrix);
		mModelUniform = mMaterialInstance.getOrCreateUniform(uniform::mvpStruct)->getOrCreateUniform<UniformMat4Instance>(uniform::modelMatrix);

		mSpriteSheetDimsUniform = mMaterialInstance.getOrCreateUniform("SpriteSheetUBO")->getOrCreateUniform<UniformVec2Instance>("sheetDims");
		mCameraPositionUniform = mMaterialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformVec3Instance>("cameraPosition");
		mPointSizeUniform = mMaterialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformFloatInstance>("pointSize");
		mVolumeScaleUniform = mMaterialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformFloatInstance>("volumeScale");
		mTimeUniform = mMaterialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformFloatInstance>("t");
		mTimeScaleUniform = mMaterialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformFloatInstance>("timeScale");
		mVariationScaleUniform = mMaterialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformFloatInstance>("variationScale");
		mVolumeSizeUniform = mMaterialInstance.getOrCreateUniform("UBO")->getOrCreateUniform<UniformIntInstance>("volumeSize");

		if (!errorState.check(mParticleMesh->init(errorState), "Unable to create particle mesh")) {
			return false;
		}

		// We can repeat a single point sprite mVolumeSize^3 times using instancing.
		// All locations are calculated procedurally using noise functions.
		MeshInstance& meshInstance = mParticleMesh->getMeshInstance();
		meshInstance.setNumVertices(1);

		Vec3VertexAttribute& positionAttrib = meshInstance.getOrCreateAttribute<glm::vec3>(vertexid::position);
		//Vec4VertexAttribute& colorAttrib = meshInstance.getOrCreateAttribute<glm::vec4>(VertexAttributeIDs::GetColorName(0));

		mPointSize = resource->mPointSize;
		mVolumeScale = resource->mVolumeScale;
		mTimeScale = resource->mTimeScale;
		mVariationScale = resource->mVariationScale;
		mVolumeSize = resource->mVolumeSize;
		mSpriteSheetDims = resource->mSpriteSheetDims;

		int volumeSize = resource->mVolumeSize;
		mNumParticles = volumeSize*volumeSize*volumeSize;
		Logger::info("%d particles", mNumParticles);

		mTime = math::random(0.f, 120.0f); // start time

		//std::vector<glm::vec4> colors; colors.reserve(count);
		//for (unsigned int i = 0; i < mNumParticles; i++) {
		//	colors.push_back(glm::vec4(1.f, 1.f, 1.f, 1.f));

		uint32_t index[] = { 0 };
		MeshShape& shape = meshInstance.createShape(); 
		shape.setIndices(&index[0], 1);

		glm::vec3 vertex[] = { glm::vec3(0.f, 0.f, 0.f) };
		positionAttrib.addData(&vertex[0], 1);
		//colorAttrib.addData(&colors[0], count);

		nap::RenderService* renderService = getEntityInstance()->getCore()->getService<RenderService>();
		mRenderableMesh = renderService->createRenderableMesh(*mParticleMesh, mMaterialInstance, errorState);
		if (!errorState.check(mRenderableMesh.isValid(), "%s, mesh: %s is invalid", resource->mID.c_str(), mParticleMesh->mID.c_str())) {
			return false;
		}

		MeshInstance& renderableMeshInstance = mRenderableMesh.getMesh().getMeshInstance();
		renderableMeshInstance.update(errorState);

		Logger::info("%s initialized", resource->mID.c_str());
		return true;
	}


	void PointSpriteVolumeInstance::update(double deltaTime)
	{
		mTime += deltaTime;
	}

	static void renderMeshInstanced(RenderService& renderService, RenderService::Pipeline& pipeline, RenderableMesh& renderableMesh, VkCommandBuffer commandBuffer, uint32_t instanceCount)
	{
		// Push uniforms and get descriptors for material
		VkDescriptorSet descriptor_set = renderableMesh.getMaterialInstance().update();

		// Bind descriptor set for next draw call
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.mLayout, 0, 1, &descriptor_set, 0, nullptr);
		
		// Bind vertex buffers
		const std::vector<VkBuffer>& vertexBuffers = renderableMesh.getVertexBuffers();
		const std::vector<VkDeviceSize>& vertexBufferOffsets = renderableMesh.getVertexBufferOffsets();
		vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), vertexBufferOffsets.data());

		// Get mesh to draw
		GPUMesh& gpuMesh = renderableMesh.getMesh().getMeshInstance().getGPUMesh();

		// Draw sprites on point instances
		const IndexBuffer& index_buffer = gpuMesh.getIndexBuffer(0);
		vkCmdBindIndexBuffer(commandBuffer, index_buffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, index_buffer.getCount(), instanceCount, 0, 0, 0);
	}

	void PointSpriteVolumeInstance::onDraw(IRenderTarget& renderTarget, VkCommandBuffer commandBuffer, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
	{
		const glm::mat4x4& modelMatrix = mTransform->getGlobalTransform();
		glm::vec3 camPos = math::extractPosition(mCameraTransform->getGlobalTransform());

		mModelUniform->setValue(modelMatrix);
		mViewUniform->setValue(viewMatrix);
		mProjectionUniform->setValue(projectionMatrix);
		mSpriteSheetDimsUniform->setValue(mSpriteSheetDims);
		mCameraPositionUniform->setValue(camPos);
		mPointSizeUniform->setValue(mPointSize);
		mVolumeScaleUniform->setValue(mVolumeScale);
		mTimeScaleUniform->setValue(mTimeScale);
		mVariationScaleUniform->setValue(mVariationScale);
		mTimeUniform->setValue((float)mTime);
		mVolumeSizeUniform->setValue(mVolumeSize);

		// Get render-pipeline for mesh / material
		utility::ErrorState error_state;
		RenderService::Pipeline pipeline = mRenderService->getOrCreatePipeline(renderTarget, mRenderableMesh.getMesh(), mRenderableMesh.getMaterialInstance(), error_state);

		// Bind pipeline and render mesh
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.mPipeline);
		renderMeshInstanced(*mRenderService, pipeline, mRenderableMesh, commandBuffer, mNumParticles);
	}

	nap::MaterialInstance& PointSpriteVolumeInstance::getMaterial()
	{
		return mMaterialInstance;
	}

	int PointSpriteVolumeInstance::getNumParticles()
	{
		return mNumParticles;
	}
}
