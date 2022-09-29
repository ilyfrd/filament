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
    BatteryCharging(std::shared_ptr<l3dkit::Vehicle> vehicle);
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

    std::shared_ptr<l3dkit::Vehicle> mVehicle;
    std::shared_ptr<l3dkit::Billboard> mBillboard;
    OffScreenRenderTarget mVehiclePicture;
    filament::MaterialInstance* mBillboardMaterialIns = nullptr;
    utils::Entity mLightEntity;
    float mDuration = 2;

};

} // namespace l3dkit
