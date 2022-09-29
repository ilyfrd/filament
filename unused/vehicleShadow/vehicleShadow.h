#pragma once

#include <memory>
#include <utils/Entity.h>

namespace filament
{
    class Camera;
    class MaterialInstance;
    class Texture;
    class RenderTarget;
}

namespace gltfio
{
    class FilamentAsset;
}

namespace l3dkit
{
    class OffScreenRT;
}

namespace l3dkit{

class VehicleShadow
{
public:
    VehicleShadow();
    ~VehicleShadow();

    void show();
    void hide();

    void beginCaptureOffScreenRT();
    void endCaptureOffScreenRT();

    utils::Entity getGroundEntity() {return mGroundEntity;}

private:
    void optimise();

    utils::Entity mGroundEntity;
    utils::Entity mCameraEntity;
    filament::Camera* mCamera = nullptr;
    filament::MaterialInstance* mGroundShadowMatIns = nullptr;
    filament::Texture* mCapturedColorTexture = nullptr;
    std::shared_ptr<l3dkit::OffScreenRT> mVehiclePicture;
    bool mCaptured = false;
};

VehicleShadow* getGroundShadow();

} // namespace l3dkit


