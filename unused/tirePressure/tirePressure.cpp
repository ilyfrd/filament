#include "tirePressure.h"
#include "billboard.h"
#include "L3dBridge.h"
#include "filamentContext.h"
#include "vehicle.h"
#include "materialFactory.h"

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

BatteryCharging::BatteryCharging(
    std::shared_ptr<l3dkit::FilamentContext> filamentContext,
    std::shared_ptr<l3dkit::ModelLoader> modelLoader,
    std::shared_ptr<l3dkit::MaterialFactory> materialFactory,
    std::shared_ptr<l3dkit::Vehicle> vehicle)
{
    mFilamentContext = filamentContext;
    mModelLoader = modelLoader;
    mVehicle = vehicle;

    
    auto createOffScreenRenderTarget = [this](OffScreenRenderTarget* offScreenRenderTarget)
    {
        offScreenRenderTarget->view = this->mFilamentContext->mEngine->createView();
        offScreenRenderTarget->scene = this->mFilamentContext->mEngine->createScene();
        offScreenRenderTarget->view->setScene(offScreenRenderTarget->scene);
        offScreenRenderTarget->view->setPostProcessingEnabled(false);
        offScreenRenderTarget->view->setCamera(this->mFilamentContext->mCamera);
    };
    createOffScreenRenderTarget(&mVehiclePicture);
    createOffScreenRenderTarget(&mTirePicture);

    auto& rcm = mFilamentContext->mEngine->getRenderableManager();

    std::vector<std::string> billBoardNames = {"aaa"};
    mBillboard = std::make_shared<Billboard>(filamentContext, modelLoader, billBoardNames);
    mBillboardMaterialIns = materialFactory->createMaterialInstance("plane.filamat");
    mBillboard->setMaterialInstance(mBillboardMaterialIns);
    mBillboard->showAll();

    mTireMaterialIns = materialFactory->createMaterialInstance("tire.filamat");

    mVehicle->getAsset()->getEntitiesByPrefix("Tire_", wheels, 4);

    mLightEntity = EntityManager::get().create();
    LightManager::Builder(LightManager::Type::DIRECTIONAL)
        .intensity(50000, filament::LightManager::EFFICIENCY_LED)
        .color(Color::toLinear<ACCURATE>(sRGBColor(1.0f, 1.0f, 1.0f)))
        .castShadows(false)
        .build(*mFilamentContext->mEngine, mLightEntity);
    mTirePicture.scene->addEntity(mLightEntity);

    auto batteryChargingInstance = mVehicle->getAsset();
    auto entities = batteryChargingInstance->getEntities();
    auto entityCount = batteryChargingInstance->getEntityCount();
    for(int i = 0; i < entityCount; ++i)
    {
        auto entity = entities[i];
        auto primitiveCount = rcm.getPrimitiveCount(rcm.getInstance(entity));

        auto item = std::find(std::begin(wheels), std::end(wheels), entity);
        if(item != std::end(wheels))
        {
            mTirePicture.scene->addEntity(entity);

            for(int j = 0; j < primitiveCount; ++j)
            {
                rcm.setMaterialInstanceAt(rcm.getInstance(entity), j, mTireMaterialIns);
            }

            rcm.setPriority(rcm.getInstance(entity), 4);
        }
        else
        {
            mVehiclePicture.scene->addEntity(entity);

            rcm.setPriority(rcm.getInstance(entity), 3);
        }
    }

    mVehiclePicture.scene->setIndirectLight(mFilamentContext->mIndirectLight);
    mTirePicture.scene->setIndirectLight(mFilamentContext->mIndirectLight);

    auto skybox = filament::Skybox::Builder().color({0, 0, 0, 1}).build(*(mFilamentContext->mEngine));
    mVehiclePicture.scene->setSkybox(skybox);
    mTirePicture.scene->setSkybox(skybox);

    mFilamentContext->addOffScreenView(mTirePicture.view);
    mFilamentContext->addOffScreenView(mVehiclePicture.view);

}

BatteryCharging::~BatteryCharging()
{
    std::vector<filament::MaterialInstance*> materialList(mMaterialInstances.numRegistered());
    mMaterialInstances.getRegisteredMaterials(materialList.data());
    for (auto material : materialList) 
    {
        mFilamentContext->mEngine->destroy(material);
    }
    mMaterialInstances.unregisterAll();  
}

void BatteryCharging::viewportChanged()
{
    auto reBuildRenderTarget = [this](OffScreenRenderTarget* offScreenRenderTarget)
    {
        // todo 释放旧的texture

        offScreenRenderTarget->colorTexture = Texture::Builder()
            .width(this->mFilamentContext->mvDrawable.mViewport.width)
            .height(this->mFilamentContext->mvDrawable.mViewport.height)
            .levels(1)
            .usage(Texture::Usage::COLOR_ATTACHMENT | Texture::Usage::SAMPLEABLE)
            .format(Texture::InternalFormat::RGBA8)
            .build(*this->mFilamentContext->mEngine);
        offScreenRenderTarget->depthTexture = Texture::Builder()
            .width(this->mFilamentContext->mvDrawable.mViewport.width)
            .height(this->mFilamentContext->mvDrawable.mViewport.height)
            .levels(1)
            .usage(Texture::Usage::DEPTH_ATTACHMENT | Texture::Usage::SAMPLEABLE)
            .format(Texture::InternalFormat::DEPTH32F)
            .build(*this->mFilamentContext->mEngine);

        offScreenRenderTarget->renderTarget = RenderTarget::Builder()
            .texture(RenderTarget::AttachmentPoint::COLOR, offScreenRenderTarget->colorTexture)
            .texture(RenderTarget::AttachmentPoint::DEPTH, offScreenRenderTarget->depthTexture)
            .build(*this->mFilamentContext->mEngine);

        offScreenRenderTarget->view->setRenderTarget(offScreenRenderTarget->renderTarget);
    };
    reBuildRenderTarget(&mTirePicture);
    reBuildRenderTarget(&mVehiclePicture);


    TextureSampler sampler(TextureSampler::MinFilter::LINEAR, TextureSampler::MagFilter::LINEAR);
    mBillboardMaterialIns->setParameter("vehicleColorTexture", mVehiclePicture.colorTexture, sampler);
    mBillboardMaterialIns->setParameter("vehicleDepthTexture", mVehiclePicture.depthTexture, sampler);
    mBillboardMaterialIns->setParameter("tireColorTexture", mTirePicture.colorTexture, sampler);
    mBillboardMaterialIns->setParameter("tireDepthTexture", mTirePicture.depthTexture, sampler);

    filament::Viewport vp = {0, 0, mFilamentContext->mvDrawable.mViewport.width, mFilamentContext->mvDrawable.mViewport.height};
    mTirePicture.view->setViewport(vp);
    mVehiclePicture.view->setViewport(vp);

    auto aspect = vp.height * 1.0 / vp.width;
    auto width = 1;
    mBillboard->setBillboardSize(width, aspect * width);
}

void BatteryCharging::tick(double timeStamp)
{
    mBillboard->tick(timeStamp);
    auto radio = fmodf(timeStamp, mDuration) / mDuration;
    radio = pow(radio, 3);
    mTireMaterialIns->setParameter("radio", radio);

    auto& lcm = mFilamentContext->mEngine->getLightManager();
    auto cameraPosition = mFilamentContext->mCamera->getPosition();
    lcm.setDirection(lcm.getInstance(mLightEntity), normalize(-cameraPosition));
}

void BatteryCharging::play(bool reverse)
{
    if(reverse == true)
    {
        mVehicle->addToScene();
        mFilamentContext->removeOffScreenView(mTirePicture.view);
        mFilamentContext->removeOffScreenView(mVehiclePicture.view);
        mBillboard->hideAll();
    }
    else
    {
        mVehicle->removeFromScene();
        mFilamentContext->addOffScreenView(mTirePicture.view);
        mFilamentContext->addOffScreenView(mVehiclePicture.view);
        mBillboard->showAll();
    }
   
}




