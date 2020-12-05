#pragma once
#include <nap/core.h>
#include <mesh.h>
#include <renderglobals.h>

namespace nap
{
	class ParticleMesh : public IMesh
	{
	public:
		ParticleMesh(Core& core) : mRenderService(core.getService<RenderService>()) {}

		bool init(utility::ErrorState& errorState)
		{
			assert(mRenderService != nullptr);
			mMeshInstance = std::make_unique<MeshInstance>(*mRenderService);

			mMeshInstance->setNumVertices(0);
			mMeshInstance->setUsage(EMeshDataUsage::DynamicWrite);
			mMeshInstance->setDrawMode(EDrawMode::Points);
			mMeshInstance->setCullMode(ECullMode::None);

			mMeshInstance->getOrCreateAttribute<glm::vec3>(vertexid::position);
			//mMeshInstance.getOrCreateAttribute<glm::vec4>(VertexAttributeIDs::GetColorName(0));

			return mMeshInstance->init(errorState);
		}
		virtual MeshInstance& getMeshInstance() override { return *mMeshInstance; }
		virtual const MeshInstance& getMeshInstance() const override { return *mMeshInstance; }

	private:
		std::unique_ptr<MeshInstance> mMeshInstance = nullptr;	///< The mesh instance to construct
		nap::RenderService* mRenderService = nullptr;			///< Handle to the render service
	};
}
