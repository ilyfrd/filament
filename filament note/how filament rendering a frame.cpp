
/*
 * 此流程未包含后处理,所做的工作是把场景中的每一个 geometry 渲染出来.
 */

FRenderer::renderJob(FView& view)
{
    view.prepare();

    // 准备 FView 的 mPerViewUniforms 数据.
    view.prepareCamera();

    // 准备 FView 的 mPerViewUniforms 数据.
    view.prepareViewport();

    /*
     * 把 FView 的 mPerViewUniforms 数据传到GPU上. 
     * PerViewUniforms 的 mUniforms 数据传到 PerViewUniforms 的 mUniformBufferHandle 对应的GPU内存上.
     * PerViewUniforms 的 mSamplers 数据传到 PerViewUniforms 的 mSamplerGroupHandle 对应的GPU内存上.
     */
    view.commitUniforms();

    /*
     * 绑定 UniformBuffer 和 Samplers.

     * 把 PerViewUniforms 的 mUniformBufferHandle 绑定到 UniformBindingPoints::PER_VIEW.
     * 把 PerViewUniforms 的 mSamplerGroupHandle 绑定到 SamplerBindingPoints::PER_VIEW.
     * 把 FView 的 mLightUbh 绑定到 UniformBindingPoints::LIGHTS.
     * 把 ShadowMapManager 的 mShadowUbh 绑定到 UniformBindingPoints::SHADOW.
     * 把 Froxelizer 的 mRecordsBuffer 绑定到 UniformBindingPoints::FROXEL_RECORDS.

     * UniformBuffer 总共有8个绑定点, 对应的图形驱动接口是glBindBufferRange().
     */
    view.bindPerViewUniformsAndSamplers();

    RenderPass pass;

    /*
     * 把 FScene 的 mRenderableData 传递到 RenderPass 中.
     * 把 FView 的 mVisibleRenderables 传递到 RenderPass 中.
     * 把 FView 的 mRenderableUbh 传递到 RenderPass 中.
     */
    pass.setGeometry();

    /*
     * 在 RenderPass 的 mCommandArena 上为 Command 分配内存, 用 RenderPass 的 mCommandBegin 和 mCommandEnd 指向该内存区的首尾.
     * 如果是渲染 depth 的 pass, 则1个 primitive 对应1个 Command; 如果是渲染 color 的 pass, 则1个 primitive 对应 2个 Command.
     * 填充Command数据, 即 CommandKey 和 PrimitiveInfo.
     */
    pass.appendCommands();

    /*
     * 为创建的所有 Command 排序, Command 根据 CommandKey 的值进行排序.
     * Command 执行的顺序是 DEPTH command -> COLOR command -> BLENDED command -> pre-CUSTOM command -> post-CUSTOM command -> SENTINEL command.
     */
    pass.sortCommands();

    /*
     * 把 FView 的 mRenderableUbh 绑定到 UniformBindingPoints::PER_RENDERABLE.
     *
     * 把 FMaterialInstance 的 mUbHandle 绑定到 UniformBindingPoints::PER_MATERIAL_INSTANCE.
     * 把 FMaterialInstance 的 mSbHandle 绑定到 SamplerBindingPoints::PER_MATERIAL_INSTANCE.
     * 
     * 根据 Command 的 PrimitiveInfo 中的信息绘制 geometry , 完成所有 geometry 的绘制.
     */
    RendererUtils::colorPass();

}

FView::prepare()
{
    // 往 FScene 的 mRenderableData 和 mLightData 中填充数据.
    mScene->prepare();

    // 往 FScene 的 mRenderableData 的 UBO 字段填充数据.
    mScene->prepareVisibleRenderables();

    // 把 FScene 的 mRenderableData 中的数据传到 FView 的 mRenderableUbh 对应的GPU内存中.
    mScene->updateUBOs();

    FView::prepareLighting();

    // 准备 mPerViewUniforms 数据.
    mPerViewUniforms.prepareTime();
    mPerViewUniforms.prepareFog();
    mPerViewUniforms.prepareTemporalNoise();
    mPerViewUniforms.prepareBlending();

    
}

FView::prepareLighting()
{
    // 把 FScene 的 mLightData 中的数据传到 FView 的 mLightUbh 对应的GPU内存中.
    mScene->prepareDynamicLights();

    // 准备 mPerViewUniforms 数据.
    mPerViewUniforms.prepareExposure();
    mPerViewUniforms.prepareAmbientLight();
    mPerViewUniforms.prepareDirectionalLight();

}




class FView : public View 
{

    FScene* mScene;

    backend::Handle<backend::HwBufferObject> mLightUbh;
    backend::Handle<backend::HwBufferObject> mRenderableUbh;

    PerViewUniforms mPerViewUniforms;

    Range mVisibleRenderables;

}

class FScene : public Scene 
{
    RenderableSoa mRenderableData;
    LightSoa mLightData;
}

class RenderPass 
{
    FScene::RenderableSoa* mRenderableSoa;
    utils::Range<uint32_t> mVisibleRenderables;

    backend::Handle<backend::HwBufferObject> mUboHandle;

}

struct PrimitiveInfo 
{ 
    union {
        FMaterialInstance const* mi; // 该 geometry 所用的材质.
    };                                                             
    backend::RasterState rasterState;                               // 该 geometry 的渲染状态.
    backend::Handle<backend::HwRenderPrimitive> primitiveHandle;    // 该 geometry 的几何数据.
    uint32_t index = 0;                                             // 该 geometry 所属 renderable 在 RenderPass 的 mVisibleRenderables 中对应的值.
    Variant materialVariant;                                        // 该值用于查找该 geometry 所用的 shader program.
}

class FMaterialInstance : public MaterialInstance 
{
    backend::Handle<backend::HwBufferObject> mUbHandle;
    backend::Handle<backend::HwSamplerGroup> mSbHandle;
}


