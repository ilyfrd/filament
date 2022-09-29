#include "vehicleShadow.h"

#include "filamentContext.h"
#include "modelLoader.h"
#include "materialFactory.h"
#include "offScreenRT.h"
#include "vehicle.h"

#include <filament/TransformManager.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/View.h>
#include <filament/Texture.h>
#include <gltfio/AssetLoader.h>
#include <utils/EntityManager.h>
#include <math/TMatHelpers.h>


using namespace filament;
using namespace filament::math;
using namespace l3dkit;


namespace l3dkit
{
    VehicleShadow* gGroundShadow = nullptr;
    VehicleShadow* getGroundShadow()
    {
        assert(gGroundShadow != nullptr);
        return gGroundShadow;
    }
}

VehicleShadow::VehicleShadow()
{
    mGroundEntity = getVehicle()->getAsset()->getFirstEntityByName("ShadowPlane02");
    mGroundShadowMatIns = MaterialFactory::get().createMaterialInstance("groundShadow2.filamat");

    auto width = getFilamentContext()->mView->getViewport().width;
    auto height = getFilamentContext()->mView->getViewport().height;
    mVehiclePicture = std::make_shared<OffScreenRT>(width, height);
    mVehiclePicture->mScene->addEntities(getVehicle()->getAsset()->getEntities(), getVehicle()->getAsset()->getEntityCount());
    mVehiclePicture->mScene->setIndirectLight(getFilamentContext()->mIndirectLight);
    mCameraEntity = utils::EntityManager::get().create();
    mCamera = getFilamentContext()->mEngine->createCamera(mCameraEntity);
    mCamera->setProjection(
        getFilamentContext()->mFovInDegrees, 
        width * 1.0 / height,
        getFilamentContext()->mNearPlane, 
        getFilamentContext()->mFarPlane, 
        Camera::Fov::HORIZONTAL);
    mCamera->lookAt(normalize(float3(0, 5, 3)), float3(0, 0, 0), float3(0, 1, 0)); 
    mVehiclePicture->mView->setCamera(mCamera);
    mVehiclePicture->mView->setViewport({0, 0, width, height});

    TextureSampler sampler(TextureSampler::MinFilter::LINEAR, TextureSampler::MagFilter::LINEAR);
    mGroundShadowMatIns->setParameter("vehicleColorTexture", mVehiclePicture->mColorTexture, sampler);
    auto projectionMaxtrix = mCamera->getProjectionMatrix();
    auto viewMatrix = mCamera->getViewMatrix();
    mGroundShadowMatIns->setParameter("worldToClipMatrix", mat4f(projectionMaxtrix * viewMatrix));
    auto& rcm = getFilamentContext()->mEngine->getRenderableManager();
    rcm.setMaterialInstanceAt(rcm.getInstance(mGroundEntity), 0, mGroundShadowMatIns);
  
    gGroundShadow = this;
}

VehicleShadow::~VehicleShadow()
{
    getFilamentContext()->mScene->remove(mGroundEntity);
    getFilamentContext()->mEngine->destroy(mGroundShadowMatIns);
    getFilamentContext()->mEngine->destroyCameraComponent(mCameraEntity);
    utils::EntityManager::get().destroy(mCameraEntity);

    getFilamentContext()->mEngine->destroy(mCapturedColorTexture);

    gGroundShadow = nullptr;
}

void VehicleShadow::show()
{
    auto& rcm = getFilamentContext()->mEngine->getRenderableManager();
    rcm.setLayerMask(rcm.getInstance(mGroundEntity), 0xff, 0xff);
}

void VehicleShadow::hide()
{
    auto& rcm = getFilamentContext()->mEngine->getRenderableManager();
    rcm.setLayerMask(rcm.getInstance(mGroundEntity), 0xff, 0x00);
}

void VehicleShadow::beginCaptureOffScreenRT()
{
    if(mCaptured) return;

    getFilamentContext()->addOffScreenView(mVehiclePicture->mView);
}

void VehicleShadow::endCaptureOffScreenRT()
{
    if(mCaptured) return;

    mCaptured = true;

    getFilamentContext()->removeOffScreenView(mVehiclePicture->mView);
    show();
    optimise();
}

void VehicleShadow::optimise()
{
    mCapturedColorTexture = mVehiclePicture->mColorTexture;
    mVehiclePicture->mKeepColorTexture = true;
    mVehiclePicture.reset();
}










