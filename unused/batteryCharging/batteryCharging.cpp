#include "batteryCharging.h"
#include "billboard.h"
#include "L3dBridge.h"
#include "filamentContext.h"
#include "vehicle.h"
#include "materialFactory.h"
#include "modelLoader.h"

#include <filament/Engine.h>
#include <filament/View.h>
#include <filament/Scene.h>
#include <filament/Viewport.h>
#include <filament/Skybox.h>
#include <filament/Texture.h>
#include <filament/Camera.h>
#include <filament/RenderTarget.h>
#include <filament/Material.h>
#include <filament/TextureSampler.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/LightManager.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/AssetLoader.h>
#include <utils/Path.h>
#include <utils/EntityManager.h>



#include <filament/Fence.h>

#include <vector>
#include <cmath>


using namespace l3dkit;
using namespace filament;
using namespace utils;
using namespace filament::math;


const float kAperture = 32.0f;
const float kShutterSpeed = 1.0f / 125.0f;
const float kSensitivity = 100.0f;

BatteryCharging::BatteryCharging(std::shared_ptr<l3dkit::Vehicle> vehicle)
{
    mVehicle = vehicle;
    
    auto createOffScreenRenderTarget = [](OffScreenRenderTarget* offScreenRenderTarget)
    {
        offScreenRenderTarget->view = getFilamentContext()->mEngine->createView();
        offScreenRenderTarget->scene = getFilamentContext()->mEngine->createScene();
        offScreenRenderTarget->view->setScene(offScreenRenderTarget->scene);
        offScreenRenderTarget->view->setPostProcessingEnabled(false);
        offScreenRenderTarget->view->setCamera(getFilamentContext()->mCamera);
    };
    createOffScreenRenderTarget(&mVehiclePicture);
    mVehiclePicture.scene->addEntities(mVehicle->getAsset()->getEntities(), mVehicle->getAsset()->getEntityCount());
    mVehiclePicture.scene->setIndirectLight(getFilamentContext()->mIndirectLight);
    getFilamentContext()->addOffScreenView(mVehiclePicture.view);

    auto& rcm = getFilamentContext()->mEngine->getRenderableManager();

    std::vector<std::string> billBoardNames = {"showScene"};
    mBillboard = std::make_shared<Billboard>(billBoardNames);
    mBillboardMaterialIns = MaterialFactory::get().createMaterialInstance("billboard.filamat");
    mBillboard->setMaterialInstance(mBillboardMaterialIns);
    mBillboard->setBillboardSize(10, 10);

}

BatteryCharging::~BatteryCharging()
{
}

void BatteryCharging::viewportChanged()
{
    auto reBuildRenderTarget = [](OffScreenRenderTarget* offScreenRenderTarget)
    {
        // todo 释放旧的texture

        offScreenRenderTarget->colorTexture = Texture::Builder()
            .width(getFilamentContext()->mvDrawable.mViewport.width)
            .height(getFilamentContext()->mvDrawable.mViewport.height)
            .levels(1)
            .usage(Texture::Usage::COLOR_ATTACHMENT | Texture::Usage::SAMPLEABLE)
            .format(Texture::InternalFormat::RGBA8)
            .build(*getFilamentContext()->mEngine);
        offScreenRenderTarget->depthTexture = Texture::Builder()
            .width(getFilamentContext()->mvDrawable.mViewport.width)
            .height(getFilamentContext()->mvDrawable.mViewport.height)
            .levels(1)
            .usage(Texture::Usage::DEPTH_ATTACHMENT | Texture::Usage::SAMPLEABLE)
            .format(Texture::InternalFormat::DEPTH32F)
            .build(*getFilamentContext()->mEngine);

        offScreenRenderTarget->renderTarget = RenderTarget::Builder()
            .texture(RenderTarget::AttachmentPoint::COLOR, offScreenRenderTarget->colorTexture)
            .texture(RenderTarget::AttachmentPoint::DEPTH, offScreenRenderTarget->depthTexture)
            .build(*getFilamentContext()->mEngine);

        offScreenRenderTarget->view->setRenderTarget(offScreenRenderTarget->renderTarget);
    };
    reBuildRenderTarget(&mVehiclePicture);

    TextureSampler sampler(TextureSampler::MinFilter::LINEAR, TextureSampler::MagFilter::LINEAR);
    mBillboardMaterialIns->setParameter("vehicleColorTexture", mVehiclePicture.colorTexture, sampler);
    mBillboardMaterialIns->setParameter("vehicleDepthTexture", mVehiclePicture.depthTexture, sampler);

    filament::Viewport vp = {0, 0, getFilamentContext()->mvDrawable.mViewport.width, getFilamentContext()->mvDrawable.mViewport.height};
    mVehiclePicture.view->setViewport(vp);

}

void BatteryCharging::tick(double timeStamp)
{
    mBillboard->tick(timeStamp);
}

void BatteryCharging::play(bool reverse)
{
    if(reverse == true)
    {
        mVehicle->addToScene();
        getFilamentContext()->removeOffScreenView(mVehiclePicture.view);
        mBillboard->hideAll();
    }
    else
    {
        mVehicle->removeFromScene();
        getFilamentContext()->addOffScreenView(mVehiclePicture.view);
        mBillboard->showAll();
    }
   
}




