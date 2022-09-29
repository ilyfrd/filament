#pragma once

#include "animationBase.h"

#include <memory>
#include <filameshio/MeshReader.h>


// #include <utils/Entity.h>


namespace filament
{
    // class Engine;
    // class Material;
    class MaterialInstance;
    class Engine;
    class Texture;
    class RenderTarget;
    class View;
    class Scene;
    class Camera;
}

namespace gltfio
{
    class FilamentAsset;
    class FilamentInstance;
}

namespace l3dkit
{
    class FilamentContext;
    class Vehicle;
    class ModelLoader;
    class Billboard;
    class MaterialFactory;
}

namespace l3dkit{

class BatteryCharging: public AnimationBase
{
public:
    BatteryCharging(
        std::shared_ptr<l3dkit::FilamentContext> filamentContext,
        std::shared_ptr<l3dkit::ModelLoader> modelLoader,
        std::shared_ptr<l3dkit::MaterialFactory> materialFactory,
        std::shared_ptr<l3dkit::Vehicle> vehicle);
    ~BatteryCharging();

    virtual void tick(double timeStamp) override;
    virtual void play(bool reverse) override;

    void viewportChanged();

private:
    struct OffScreenRenderTarget
    {
        filament::Texture* colorTexture = nullptr;
        filament::Texture* depthTexture = nullptr;
        filament::RenderTarget* renderTarget = nullptr;
        filament::View* view = nullptr;
        filament::Scene* scene = nullptr;
    };

    std::shared_ptr<l3dkit::FilamentContext> mFilamentContext;
    std::shared_ptr<l3dkit::Vehicle> mVehicle;
    std::shared_ptr<l3dkit::ModelLoader> mModelLoader;
    std::shared_ptr<l3dkit::Billboard> mBillboard;
    filamesh::MeshReader::MaterialRegistry mMaterialInstances;
    filamesh::MeshReader::Mesh mMesh;   
    OffScreenRenderTarget mVehiclePicture;
    OffScreenRenderTarget mTirePicture;
    filament::MaterialInstance* mBillboardMaterialIns = nullptr;
    filament::MaterialInstance* mTireMaterialIns = nullptr;

    filament::MaterialInstance* mBatteryChargingMaterialIns = nullptr;

    gltfio::FilamentInstance* mBatteryChargingInstance = nullptr;
    float mDuration = 2;

    utils::Entity wheels[4] = {};
    utils::Entity mLightEntity;

};

} // namespace l3dkit
