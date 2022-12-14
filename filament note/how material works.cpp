
static constexpr const char* CONFIG_KEY_MATERIAL= "material";
static constexpr const char* CONFIG_KEY_VERTEX_SHADER = "vertex";
static constexpr const char* CONFIG_KEY_FRAGMENT_SHADER = "fragment";

MaterialCompiler::MaterialCompiler() 
{
    mConfigProcessorJSON[CONFIG_KEY_MATERIAL] = &MaterialCompiler::processMaterialJSON;
    mConfigProcessorJSON[CONFIG_KEY_VERTEX_SHADER] = &MaterialCompiler::processVertexShaderJSON;
    mConfigProcessorJSON[CONFIG_KEY_FRAGMENT_SHADER] = &MaterialCompiler::processFragmentShaderJSON;
}

MaterialCompiler::run() 
{
    MaterialBuilder::init();

    MaterialBuilder builder;

    // 把mat材质的信息传递到 MaterialBuilder 中.
    parseMaterialAsJSON();

    // 编译材质.
    Package package = builder.build();

    MaterialBuilder::shutdown();
}

MaterialCompiler::parseMaterialAsJSON()
{
    // 此处json是mat材质文件解析成的json对象.
    for (auto& entry : json->getEntries()) 
    {
        const std::string& key = entry.first;
        MaterialConfigProcessorJSON p =  mConfigProcessorJSON.at(key);
        bool ok = (*this.*p)(entry.second, builder);
    }

}

// 把 mat 文件 material key 的所有参数传递到 MaterialBuilder 中.
MaterialCompiler::processMaterialJSON()
{
    ParametersProcessor parametersProcessor;
    parametersProcessor.process();
}

// 顶点着色器代码传递到了 MaterialBuilder 中.
MaterialCompiler::processVertexShaderJSON(const JsonishValue* value, filamat::MaterialBuilder& builder)
{
    builder.materialVertex(value->toJsonString()->getString().c_str());
}

// 片元着色器代码传递到了 MaterialBuilder 中.
MaterialCompiler::processFragmentShaderJSON(const JsonishValue* value, filamat::MaterialBuilder& builder)
{
    builder.material(value->toJsonString()->getString().c_str());
}

ParametersProcessor::ParametersProcessor() 
{
    using Type = JsonishValue::Type;
    mParameters["name"]                          = { &processName, Type::STRING };
    mParameters["interpolation"]                 = { &processInterpolation, Type::STRING };
    mParameters["parameters"]                    = { &processParameters, Type::ARRAY };
    mParameters["buffers"]                       = { &processBuffers, Type::ARRAY };
    mParameters["subpasses"]                     = { &processSubpasses, Type::ARRAY };
    mParameters["variables"]                     = { &processVariables, Type::ARRAY };
    mParameters["requires"]                      = { &processRequires, Type::ARRAY };
    mParameters["blending"]                      = { &processBlending, Type::STRING };
    mParameters["postLightingBlending"]          = { &processPostLightingBlending, Type::STRING };
    mParameters["vertexDomain"]                  = { &processVertexDomain, Type::STRING };
    mParameters["culling"]                       = { &processCulling, Type::STRING };
    mParameters["colorWrite"]                    = { &processColorWrite, Type::BOOL };
    mParameters["depthWrite"]                    = { &processDepthWrite, Type::BOOL };
    mParameters["instanced"]                     = { &processInstanced, Type::BOOL };
    mParameters["depthCulling"]                  = { &processDepthCull, Type::BOOL };
    mParameters["doubleSided"]                   = { &processDoubleSided, Type::BOOL };
    mParameters["transparency"]                  = { &processTransparencyMode, Type::STRING };
    mParameters["reflections"]                   = { &processReflectionMode, Type::STRING };
    mParameters["maskThreshold"]                 = { &processMaskThreshold, Type::NUMBER };
    mParameters["shadowMultiplier"]              = { &processShadowMultiplier, Type::BOOL };
    mParameters["transparentShadow"]             = { &processTransparentShadow, Type::BOOL };
    mParameters["shadingModel"]                  = { &processShading, Type::STRING };
    mParameters["variantFilter"]                 = { &processVariantFilter, Type::ARRAY };
    mParameters["specularAntiAliasing"]          = { &processSpecularAntiAliasing, Type::BOOL };
    mParameters["specularAntiAliasingVariance"]  = { &processSpecularAntiAliasingVariance, Type::NUMBER };
    mParameters["specularAntiAliasingThreshold"] = { &processSpecularAntiAliasingThreshold, Type::NUMBER };
    mParameters["clearCoatIorChange"]            = { &processClearCoatIorChange, Type::BOOL };
    mParameters["flipUV"]                        = { &processFlipUV, Type::BOOL };
    mParameters["multiBounceAmbientOcclusion"]   = { &processMultiBounceAO, Type::BOOL };
    mParameters["specularAmbientOcclusion"]      = { &processSpecularAmbientOcclusion, Type::STRING };
    mParameters["domain"]                        = { &processDomain, Type::STRING };
    mParameters["refractionMode"]                = { &processRefractionMode, Type::STRING };
    mParameters["refractionType"]                = { &processRefractionType, Type::STRING };
    mParameters["framebufferFetch"]              = { &processFramebufferFetch, Type::BOOL };
    mParameters["vertexDomainDeviceJittered"]    = { &processVertexDomainDeviceJittered, Type::BOOL };
    mParameters["legacyMorphing"]                = { &processLegacyMorphing, Type::BOOL };
    mParameters["outputs"]                       = { &processOutputs, Type::ARRAY };
    mParameters["quality"]                       = { &processQuality, Type::STRING };
    mParameters["customSurfaceShading"]          = { &processCustomSurfaceShading, Type::BOOL };
    mParameters["featureLevel"]                  = { &processFeatureLevel, Type::NUMBER };
    mParameters["groupSize"]                     = { &processGroupSizes, Type::ARRAY };
}

ParametersProcessor::process(MaterialBuilder& builder, const JsonishObject& jsonObject)
{
    for(const auto& entry : jsonObject.getEntries()) 
    {
        const std::string& key = entry.first;
        const JsonishValue* field = entry.second;

        auto fPointer = mParameters[key].callback;
        bool ok = fPointer(builder, *field);
    }

}


class MaterialCompiler final: public Compiler 
{

    using MaterialConfigProcessorJSON = bool (MaterialCompiler::*)(const JsonishValue*, filamat::MaterialBuilder& builder) const;
    std::unordered_map<std::string, MaterialConfigProcessorJSON> mConfigProcessorJSON;

}


class UTILS_PUBLIC MaterialBuilder : public MaterialBuilderBase 
{
    utils::CString mMaterialName;
    utils::CString mFileName;

    ShaderCode mMaterialFragmentCode;
    ShaderCode mMaterialVertexCode;

    PropertyList mProperties;
    ParameterList mParameters;
    VariableList mVariables;
    OutputList mOutputs;

    BlendingMode mBlendingMode = BlendingMode::OPAQUE;
    BlendingMode mPostLightingBlendingMode = BlendingMode::TRANSPARENT;
    CullingMode mCullingMode = CullingMode::BACK;
    Shading mShading = Shading::LIT;
    MaterialDomain mMaterialDomain = MaterialDomain::SURFACE;
    RefractionMode mRefractionMode = RefractionMode::NONE;
    RefractionType mRefractionType = RefractionType::SOLID;
    ReflectionMode mReflectionMode = ReflectionMode::DEFAULT;
    Interpolation mInterpolation = Interpolation::SMOOTH;
    VertexDomain mVertexDomain = VertexDomain::OBJECT;
    TransparencyMode mTransparencyMode = TransparencyMode::DEFAULT;

    filament::AttributeBitset mRequiredAttributes;

    float mMaskThreshold = 0.4f;
    float mSpecularAntiAliasingVariance = 0.15f;
    float mSpecularAntiAliasingThreshold = 0.2f;

    bool mDoubleSided = false;
    bool mDoubleSidedCapability = false;
    bool mColorWrite = true;
    bool mDepthTest = true;
    bool mInstanced = false;
    bool mDepthWrite = true;
    bool mDepthWriteSet = false;

}

class ShaderCode 
{
    utils::CString mCode;
}

enum class Property : uint8_t 
{
    BASE_COLOR,              //!< float4, all shading models
    ROUGHNESS,               //!< float,  lit shading models only
    METALLIC,                //!< float,  all shading models, except unlit and cloth
    REFLECTANCE,             //!< float,  all shading models, except unlit and cloth
    AMBIENT_OCCLUSION,       //!< float,  lit shading models only, except subsurface and cloth
    CLEAR_COAT,              //!< float,  lit shading models only, except subsurface and cloth
    CLEAR_COAT_ROUGHNESS,    //!< float,  lit shading models only, except subsurface and cloth
    CLEAR_COAT_NORMAL,       //!< float,  lit shading models only, except subsurface and cloth
    ANISOTROPY,              //!< float,  lit shading models only, except subsurface and cloth
    ANISOTROPY_DIRECTION,    //!< float3, lit shading models only, except subsurface and cloth
    THICKNESS,               //!< float,  subsurface shading model only
    SUBSURFACE_POWER,        //!< float,  subsurface shading model only
    SUBSURFACE_COLOR,        //!< float3, subsurface and cloth shading models only
    SHEEN_COLOR,             //!< float3, lit shading models only, except subsurface
    SHEEN_ROUGHNESS,         //!< float3, lit shading models only, except subsurface and cloth
    SPECULAR_COLOR,          //!< float3, specular-glossiness shading model only
    GLOSSINESS,              //!< float,  specular-glossiness shading model only
    EMISSIVE,                //!< float4, all shading models
    NORMAL,                  //!< float3, all shading models only, except unlit
    POST_LIGHTING_COLOR,     //!< float4, all shading models
    CLIP_SPACE_TRANSFORM,    //!< mat4,   vertex shader only
    ABSORPTION,              //!< float3, how much light is absorbed by the material
    TRANSMISSION,            //!< float,  how much light is refracted through the material
    IOR,                     //!< float,  material's index of refraction
    MICRO_THICKNESS,         //!< float, thickness of the thin layer
    BENT_NORMAL,             //!< float3, all shading models only, except unlit

};


//----------------------------------------------------------------------------------------------------------

MaterialBuilder::build()
{
    MaterialInfo info;

    prepareToBuild(info);

    info.samplerBindings.init(info.sib);

    // 把该材质的信息写进 ChunkContainer.
    ChunkContainer container;
    writeCommonChunks(container, info);

    // 把该材质的信息写进 ChunkContainer.
    if (mMaterialDomain == MaterialDomain::SURFACE) 
    {
        writeSurfaceChunks(container);
    }

    // 生成 shader, 把 shader 字符串写到 ChunkContainer 中.
    generateShaders(container, info);

    // 把 ChunkContainer 中的信息写到 Package 中, Package 以二进制的形式保存信息, Package 的信息再写进 filamat 文件中.
    Package package(container.getSize());
    Flattener f(package);
    container.flatten(f);
}

/*
 * 主要构建该材质的 SamplerInterfaceBlock 和 BufferInterfaceBlock 信息.
 */
MaterialBuilder::prepareToBuild(MaterialInfo& info)
{
    SamplerInterfaceBlock::Builder sbb;
    BufferInterfaceBlock::Builder ibb;

    for (size_t i = 0, c = mParameterCount; i < c; i++) 
    {
        auto const& param = mParameters[i];

        if (param.isSampler()) 
        {
            // 把 mParameters 中的信息传递到 SamplerInterfaceBlock::Builder 中.
            sbb.add();
        } 
        else if (param.isUniform()) 
        {
            // 把 mParameters 中的信息传递到 BufferInterfaceBlock::Builder 中.
            ibb.add();
        }

    }

    // 此处 _specularAntiAliasingVariance 在shader中会以 materialParams._specularAntiAliasingVariance 的形式引用.
    if (mSpecularAntiAliasing) 
    {
        ibb.add({{ "_specularAntiAliasingVariance",  0, UniformType::FLOAT }, { "_specularAntiAliasingThreshold", 0, UniformType::FLOAT }});
    }

    if (mBlendingMode == BlendingMode::MASKED) {
        ibb.add({{ "_maskThreshold", 0, UniformType::FLOAT }});
    }

    if (mDoubleSidedCapability) {
        ibb.add({{ "_doubleSided", 0, UniformType::BOOL }});
    }

    /*
     * 把上面传递到 SamplerInterfaceBlock::Builder 中的信息传递到 SamplerInterfaceBlock 中.
     * 在此过程中设置 SamplerInfo 的 uniformName 字段的值为 "materialParams" + "_" + SamplerInfo 中 name 字段的值.
     * 然后赋值给 MaterialInfo 的 sib 字段.
     */
    info.sib = sbb.name("MaterialParams").build();

    /*
     * 把上面传递到 BufferInterfaceBlock::Builder 中的信息传递到 BufferInterfaceBlock 中.
     * 然后赋值给 MaterialInfo 的 uib 字段.
     */
    info.uib = ibb.name("MaterialParams").build();


    // 把 MaterialBuilder 中的信息传递到 MaterialInfo 中.
    info.hasDoubleSidedCapability = mDoubleSidedCapability;
    info.specularAntiAliasing = mSpecularAntiAliasing;
    info.clearCoatIorChange = mClearCoatIorChange;
    info.flipUV = mFlipUV;
    info.requiredAttributes = mRequiredAttributes;
    info.blendingMode = mBlendingMode;
    info.postLightingBlendingMode = mPostLightingBlendingMode;
    info.shading = mShading;
    info.hasShadowMultiplier = mShadowMultiplier;
    info.hasTransparentShadow = mTransparentShadow;
    info.multiBounceAO = mMultiBounceAO;
    info.multiBounceAOSet = mMultiBounceAOSet;
    info.specularAO = mSpecularAO;
    info.specularAOSet = mSpecularAOSet;
    info.refractionMode = mRefractionMode;
    info.refractionType = mRefractionType;
    info.reflectionMode = mReflectionMode;
    info.quality = mShaderQuality;
    info.hasCustomSurfaceShading = mCustomSurfaceShading;
    info.useLegacyMorphing = mUseLegacyMorphing;
    info.instanced = mInstanced;

}


/*
 * 构建该材质的 SamplerBindingMap 信息.
 */
SamplerBindingMap::init(SamplerInterfaceBlock const& perMaterialSib)
{
    uint8_t offset = 0;

    auto processSamplerGroup = [&](SamplerBindingPoints bindingPoint)
    {
        // PER_MATERIAL_INSTANCE 对应的 sampler 是随材质变化的, 所以需要通过参数 perMaterialSib 传进来; PER_VIEW 和 PER_RENDERABLE_MORPHING 对应的 sampler 是固定的.
        SamplerInterfaceBlock const* const sib = (bindingPoint == SamplerBindingPoints::PER_MATERIAL_INSTANCE) ? &perMaterialSib : SibGenerator::getSib(bindingPoint, {});
        if (sib) 
        {
            auto const& list = sib->getSamplerInfoList();
            const size_t samplerCount = list.size(); //  本SamplerInterfaceBlock 中 sampler 的数量.

            // 该信息在生成 shader program 的时候会用到.
            // 参考 ShaderGenerator::createVertexProgram()、ShaderGenerator::createFragmentProgram()、ShaderGenerator::createComputeProgram()、ShaderGenerator::createPostProcessVertexProgram()、ShaderGenerator::createPostProcessFragmentProgram()
            // 参考 CodeGenerator::generateSamplers(), 最终生成 layout(binding = bindingIndex) uniform sampler2D materialParams_xxx 这样的代码.
            mSamplerBlockOffsets[bindingPoint] = { offset, samplerCount };

            for (size_t i = 0; i < samplerCount; i++) 
            {
                // 在 shader 中的绑定点和对应的 uniform 名字一一对应, 一个 sampler 对应一个 uniform 名字.
                mSamplerNamesBindingMap[offset + i] = list[i].uniformName;
            }

            offset += samplerCount;
        }
    };

    switch(materialDomain) 
    {
        // surface 材质除了会使用本材质定义的 sampler 外, 还会使用其它必要的 sampler.
        case MaterialDomain::SURFACE:
            for (size_t i = 0; i < Enum::count<SamplerBindingPoints>(); i++) 
            {
                processSamplerGroup((SamplerBindingPoints)i);
            }
            break;

        // 后处理材质只会使用本材质定义的 sampler.
        case MaterialDomain::POST_PROCESS:
            processSamplerGroup(SamplerBindingPoints::PER_MATERIAL_INSTANCE);
            break;
    }
}

SibGenerator::getSib(SamplerBindingPoints bindingPoint)
{
    switch (bindingPoint) 
    {
        case SamplerBindingPoints::PER_VIEW:
            return &getPerViewSib(variant);
        case SamplerBindingPoints::PER_RENDERABLE_MORPHING:
            return &getPerRenderPrimitiveMorphingSib(variant);
    }
}

SibGenerator::getPerViewSib(Variant variant)
{
    static SamplerInterfaceBlock sibPcf = SamplerInterfaceBlock::Builder()
            .name("Light") // 该 SamplerInterfaceBlock 生成的 uniform 名字以 "light" + "_" 开头.
            .stageFlags(backend::ShaderStageFlags::FRAGMENT)
            .add(  {{ "shadowMap",   Type::SAMPLER_2D_ARRAY, Format::SHADOW, Precision::MEDIUM },
                    { "froxels",     Type::SAMPLER_2D,       Format::UINT,   Precision::MEDIUM },
                    { "iblDFG",      Type::SAMPLER_2D,       Format::FLOAT,  Precision::MEDIUM },
                    { "iblSpecular", Type::SAMPLER_CUBEMAP,  Format::FLOAT,  Precision::MEDIUM },
                    { "ssao",        Type::SAMPLER_2D_ARRAY, Format::FLOAT,  Precision::MEDIUM },
                    { "ssr",         Type::SAMPLER_2D_ARRAY, Format::FLOAT,  Precision::MEDIUM },
                    { "structure",   Type::SAMPLER_2D,       Format::FLOAT,  Precision::HIGH   }}
            )
            .build();

    static SamplerInterfaceBlock sibVsm = SamplerInterfaceBlock::Builder()
            .name("Light")
            .stageFlags(backend::ShaderStageFlags::FRAGMENT)
            .add(  {{ "shadowMap",   Type::SAMPLER_2D_ARRAY, Format::FLOAT,  Precision::HIGH   },
                    { "froxels",     Type::SAMPLER_2D,       Format::UINT,   Precision::MEDIUM },
                    { "iblDFG",      Type::SAMPLER_2D,       Format::FLOAT,  Precision::MEDIUM },
                    { "iblSpecular", Type::SAMPLER_CUBEMAP,  Format::FLOAT,  Precision::MEDIUM },
                    { "ssao",        Type::SAMPLER_2D_ARRAY, Format::FLOAT,  Precision::MEDIUM },
                    { "ssr",         Type::SAMPLER_2D_ARRAY, Format::FLOAT,  Precision::MEDIUM },
                    { "structure",   Type::SAMPLER_2D,       Format::FLOAT,  Precision::HIGH   }}
            )
            .build();

    static SamplerInterfaceBlock sibSsr = SamplerInterfaceBlock::Builder()
            .name("Light")
            .stageFlags(backend::ShaderStageFlags::FRAGMENT)
            .add(  {{ "shadowMap",   Type::SAMPLER_2D_ARRAY, Format::SHADOW, Precision::MEDIUM },
                    { "froxels",     Type::SAMPLER_2D,       Format::UINT,   Precision::MEDIUM },
                    { "iblDFG",      Type::SAMPLER_2D,       Format::FLOAT,  Precision::MEDIUM },
                    { "iblSpecular", Type::SAMPLER_CUBEMAP,  Format::FLOAT,  Precision::MEDIUM },
                    { "ssao",        Type::SAMPLER_2D_ARRAY, Format::FLOAT,  Precision::MEDIUM },
                    { "ssr",         Type::SAMPLER_2D,       Format::FLOAT,  Precision::MEDIUM },
                    { "structure",   Type::SAMPLER_2D,       Format::FLOAT,  Precision::HIGH   }}
            )
            .build();

    if (Variant::isSSRVariant(variant)) 
    {
        return sibSsr;
    } 
    else if (Variant::isVSMVariant(variant)) 
    {
        return sibVsm;
    } 
    else 
    {
        return sibPcf;
    }
}



SibGenerator::getPerRenderPrimitiveMorphingSib(Variant variant) 
{
    static SamplerInterfaceBlock sib = SamplerInterfaceBlock::Builder()
            .name("MorphTargetBuffer") // 该 SamplerInterfaceBlock 生成的 uniform 名字以 "morphTargetBuffer" + "_" 开头.
            .stageFlags(backend::ShaderStageFlags::VERTEX)
            .add({  { "positions", Type::SAMPLER_2D_ARRAY, Format::FLOAT, Precision::HIGH },
                    { "tangents",  Type::SAMPLER_2D_ARRAY, Format::INT,   Precision::HIGH }})
            .build();

    return sib;
}


MaterialBuilder::writeCommonChunks(ChunkContainer& container, MaterialInfo& info)
{
    if (info.featureLevel <= FeatureLevel::FEATURE_LEVEL_1) 
    {
        FixedCapacityVector<std::pair<std::string_view, UniformBindingPoints>> list = 
        {
                { PerViewUib::_name,               UniformBindingPoints::PER_VIEW },
                { PerRenderableUib::_name,         UniformBindingPoints::PER_RENDERABLE },
                { LightsUib::_name,                UniformBindingPoints::LIGHTS },
                { ShadowUib::_name,                UniformBindingPoints::SHADOW },
                { FroxelRecordUib::_name,          UniformBindingPoints::FROXEL_RECORDS },
                { PerRenderableBoneUib::_name,     UniformBindingPoints::PER_RENDERABLE_BONES },
                { PerRenderableMorphingUib::_name, UniformBindingPoints::PER_RENDERABLE_MORPHING },
                { info.uib.getName(),              UniformBindingPoints::PER_MATERIAL_INSTANCE }
        };
        container.addChild<MaterialUniformBlockBindingsChunk>(std::move(list));
    }

    container.addChild<MaterialSamplerBlockBindingChunk>(info.samplerBindings);
    container.addChild<MaterialUniformInterfaceBlockChunk>(info.uib);
    container.addChild<MaterialSamplerInterfaceBlockChunk>(info.sib);

     if (mMaterialDomain != MaterialDomain::COMPUTE) 
     {
        container.addSimpleChild<bool>(ChunkType::MaterialDoubleSidedSet, mDoubleSidedCapability);
        container.addSimpleChild<bool>(ChunkType::MaterialDoubleSided, mDoubleSided);
        container.addSimpleChild<uint8_t>(ChunkType::MaterialBlendingMode, static_cast<uint8_t>(mBlendingMode));
        container.addSimpleChild<uint8_t>(ChunkType::MaterialTransparencyMode, static_cast<uint8_t>(mTransparencyMode));
        container.addSimpleChild<uint8_t>(ChunkType::MaterialReflectionMode, static_cast<uint8_t>(mReflectionMode));
        container.addSimpleChild<bool>(ChunkType::MaterialDepthWriteSet, mDepthWriteSet);
        container.addSimpleChild<bool>(ChunkType::MaterialColorWrite, mColorWrite);
        container.addSimpleChild<bool>(ChunkType::MaterialDepthWrite, mDepthWrite);
        container.addSimpleChild<bool>(ChunkType::MaterialDepthTest, mDepthTest);
        container.addSimpleChild<bool>(ChunkType::MaterialInstanced, mInstanced);
        container.addSimpleChild<uint8_t>(ChunkType::MaterialCullingMode, static_cast<uint8_t>(mCullingMode));
    }

}


MaterialBuilder::generateShaders(ChunkContainer& container, const MaterialInfo& info)
{
    LineDictionary textDictionary;

    std::vector<TextEntry> glslEntries;

    // 根据不同的平台(PC、手机), 不同的API(Opengl、Metal、Vulkan),不同的shader语言(GLSL、SPIRV)生成 shader.
    for (const auto& params : mCodeGenPermutations) 
    {
        // 根据不同的 Variant 生成 shader.
        for (const auto& v : variants) 
        {
            TextEntry glslEntry;

            std::string shader;
            if (v.stage == backend::ShaderStage::VERTEX) 
            {
                shader = sg.createVertexProgram();
            } 
            else if (v.stage == backend::ShaderStage::FRAGMENT) 
            {
                shader = sg.createFragmentProgram();
            } 
            else if (v.stage == backend::ShaderStage::COMPUTE) 
            {
                shader = sg.createComputeProgram();
            }

            switch (targetApi) 
            {
                case TargetApi::OPENGL:
                    glslEntry.stage = uint8_t(v.stage);
                    glslEntry.shader = shader;
                    glslEntries.push_back(glslEntry);
                    break;
                case TargetApi::VULKAN:
                    break;
                case TargetApi::METAL:
                    break;
            }
        }
    }

    // 把 shader 的信息写到 LineDictionary 中.
    for (const auto& s : glslEntries) 
    {
        textDictionary.addText(s.shader);
    }

    // 把 LineDictionary 的信息写到 ChunkContainer 中.
    container.addChild<filamat::DictionaryTextChunk>(std::move(textDictionary), ChunkType::DictionaryText);

    // 把 glslEntries 的信息写到 ChunkContainer 中, 这里好像和上面的重复了 ?
    if (!glslEntries.empty()) 
    {
        container.addChild<MaterialTextChunk>(std::move(glslEntries), dictionaryChunk.getDictionary(), ChunkType::MaterialGlsl);
    }
}


ShaderGenerator::createFragmentProgram(MaterialInfo const& material)
{
    // mat 文件中 material key 的非 parameters 字段变成了 shader 中的 Defines.
    const bool lit = material.isLit;
    CodeGenerator::generateDefine(fs, "GEOMETRIC_SPECULAR_AA", material.specularAntiAliasing && lit);
    CodeGenerator::generateDefine(fs, "CLEAR_COAT_IOR_CHANGE", material.clearCoatIorChange);
    auto specularAO = material.specularAOSet ? material.specularAO : defaultSpecularAO;
    CodeGenerator::generateDefine(fs, "SPECULAR_AMBIENT_OCCLUSION", uint32_t(specularAO));
    CodeGenerator::generateDefine(fs, "MATERIAL_HAS_REFRACTION", material.refractionMode != RefractionMode::NONE);
    CodeGenerator::generateDefine(fs, "MATERIAL_HAS_SHADOW_MULTIPLIER", material.hasShadowMultiplier);
    CodeGenerator::generateDefine(fs, "MATERIAL_HAS_INSTANCES", material.instanced);
    CodeGenerator::generateDefine(fs, "MATERIAL_HAS_VERTEX_DOMAIN_DEVICE_JITTERED", material.vertexDomainDeviceJittered);
    CodeGenerator::generateDefine(fs, "MATERIAL_HAS_TRANSPARENT_SHADOW", material.hasTransparentShadow);


    CodeGenerator::generateShaderInputs(fs, ShaderStage::FRAGMENT, material.requiredAttributes, interpolation);

    // mat 文件中的 parameters 中的非 sampler 类型的字段变成了 shader 中 以 mat 文件材质名称为名字的 uniform 的字段, 该uniform 绑定点是 UniformBindingPoints::PER_MATERIAL_INSTANCE.
    // mat 文件中的 parameters 中的 sampler 类型的字段, 每一个都变成了一个 uniform.
    // 除了为本材质在 shader 中生成 uniform 代码, 也要为其它必要元素生成 uniform 代码, 即 PER_VIEW、PER_RENDERABLE、LIGHTS、FROXEL_RECORDS等.
    cg.generateUniforms(fs, ShaderStage::FRAGMENT, UniformBindingPoints::PER_VIEW, UibGenerator::getPerViewUib());
    cg.generateUniforms(fs, ShaderStage::FRAGMENT, UniformBindingPoints::PER_RENDERABLE, UibGenerator::getPerRenderableUib());
    cg.generateUniforms(fs, ShaderStage::FRAGMENT, UniformBindingPoints::LIGHTS, UibGenerator::getLightsUib());
    cg.generateUniforms(fs, ShaderStage::FRAGMENT, UniformBindingPoints::FROXEL_RECORDS, UibGenerator::getFroxelRecordUib());
    cg.generateUniforms(fs, ShaderStage::FRAGMENT, UniformBindingPoints::PER_MATERIAL_INSTANCE, material.uib);

    // 把 mat 文件中定义的 shader 字符串加入到本次要生成的 shader 代码中.
    appendShader();

}

class BufferInterfaceBlock 
{
    class Builder 
    {
        utils::CString mName;
        std::vector<FieldInfo> mEntries;
    }

    utils::CString mName; // 赋值为 MaterialParams.
    utils::FixedCapacityVector<FieldInfo> mFieldInfoList;
}

using Type = backend::SamplerType;
using Format = backend::SamplerFormat;

struct SamplerInfo 
{ 
    utils::CString name;        // name of this sampler
    utils::CString uniformName; // name of the uniform holding this sampler (needed for glsl)
    uint8_t offset;             // offset in "Sampler" of this sampler in the buffer
    Type type;                  // type of this sampler
    Format format;              // format of this sampler
}

enum class SamplerType : uint8_t 
{
    SAMPLER_2D,             //!< 2D texture
    SAMPLER_2D_ARRAY,       //!< 2D array texture
    SAMPLER_CUBEMAP,        //!< Cube map texture
    SAMPLER_EXTERNAL,       //!< External texture
    SAMPLER_3D,             //!< 3D texture
    SAMPLER_CUBEMAP_ARRAY,  //!< Cube map array texture (feature level 2)
}

enum class SamplerFormat : uint8_t 
{
    INT = 0,        //!< signed integer sampler
    UINT = 1,       //!< unsigned integer sampler
    FLOAT = 2,      //!< float sampler
    SHADOW = 3      //!< shadow sampler (PCF)
}

class SamplerInterfaceBlock 
{
    class Builder 
    {
        utils::CString mName; 
        std::vector<SamplerInfo> mEntries;
    }

    utils::CString mName; // 赋值为 MaterialParams.
    utils::FixedCapacityVector<SamplerInfo> mSamplersInfoList;
}


struct UTILS_PUBLIC MaterialInfo 
{
    bool hasDoubleSidedCapability;
    bool hasExternalSamplers;
    bool hasShadowMultiplier;
    bool hasTransparentShadow;
    bool specularAntiAliasing;
    bool clearCoatIorChange;
    bool flipUV;
    bool multiBounceAO;
    bool multiBounceAOSet;
    bool specularAOSet;
    bool hasCustomSurfaceShading;
    bool useLegacyMorphing;
    bool instanced;
    bool vertexDomainDeviceJittered;
    filament::SpecularAmbientOcclusion specularAO;
    filament::RefractionMode refractionMode;
    filament::RefractionType refractionType;
    filament::ReflectionMode reflectionMode;
    filament::AttributeBitset requiredAttributes;
    filament::BlendingMode blendingMode;
    filament::BlendingMode postLightingBlendingMode;
    filament::Shading shading;

    filament::BufferInterfaceBlock uib;
    filament::SamplerInterfaceBlock sib;

    filament::SamplerBindingMap samplerBindings;

}


class SamplerBindingMap 
{
    using SamplerGroupBindingInfoList = std::array<SamplerGroupBindingInfo, utils::Enum::count<SamplerBindingPoints>()>;
    using SamplerBindingToNameMap = utils::FixedCapacityVector<utils::CString>;

    SamplerGroupBindingInfoList mSamplerBlockOffsets;
    SamplerBindingToNameMap mSamplerNamesBindingMap;
}

enum class SamplerBindingPoints : uint8_t 
{
    PER_VIEW                   = 0,    // samplers updated per view
    PER_RENDERABLE_MORPHING    = 1,    // morphing sampler updated per render primitive
    PER_MATERIAL_INSTANCE      = 2,    // samplers updates per material
}

struct SamplerGroupBindingInfo 
{
    // global binding of this block, or UNKNOWN_OFFSET if not used.
    uint8_t bindingOffset = UNKNOWN_OFFSET;

    // number of samplers in this block. Can be zero.
    uint8_t count = 0;
};

class ChunkContainer 
{
    using ChunkPtr = std::unique_ptr<Chunk>;
    std::vector<ChunkPtr> mChildren;
}

struct TextEntry 
{
    uint8_t stage;
    std::string shader;
}

class LineDictionary 
{
    std::unordered_map<std::string_view, size_t> mLineIndices; // 查找表, key 是 shader 每一行的字符串, value 是该字符串在 mStrings 中的index.
    std::vector<std::unique_ptr<std::string>> mStrings; // 保存 shader 的每一行的字符串.
}



//----------------------------------------------------------------------------------------------------------

// filamat 材质传入  Material::Builder
Material::Builder::package(const void* payload, size_t size) 
{
    mImpl->mPayload = payload;
    mImpl->mSize = size;
}

Material::Builder::build(Engine& engine) 
{
    std::unique_ptr<MaterialParser> materialParser{ createParser(upcast(engine).getBackend(), mImpl->mPayload, mImpl->mSize) };

    mImpl->mMaterialParser = materialParser.release();

    upcast(engine).createMaterial(*this);
}

// MaterialParser 通过 Unflattener 读取材质信息.
FMaterial::FMaterial(FEngine& engine, const Material::Builder& builder)
{
    MaterialParser* parser = builder->mMaterialParser;

    parser->getSIB(&mSamplerInterfaceBlock);

    parser->getUIB(&mUniformInterfaceBlock);

    parser->getUniformBlockBindings(&mUniformBlockBindings);

    parser->getSamplerBlockBindings(&mSamplerGroupBindingInfoList, &mSamplerBindingToNameMap);

    parser->getShading(&mShading);
    parser->getMaterialProperties(&mMaterialProperties);
    parser->getBlendingMode(&mBlendingMode);
    parser->getInterpolation(&mInterpolation);
    parser->getVertexDomain(&mVertexDomain);
    parser->getMaterialDomain(&mMaterialDomain);
    parser->getRequiredAttributes(&mRequiredAttributes);
    parser->getRefractionMode(&mRefractionMode);
    parser->getRefractionType(&mRefractionType);
    parser->getReflectionMode(&mReflectionMode);

}



class Material 
{
    class Builder
    {
        Builder& package(const void* payload, size_t size);
        Material* build(Engine& engine);
    }

}

class FMaterial : public Material 
{
    mutable std::array<backend::Handle<backend::HwProgram>, VARIANT_COUNT> mCachedPrograms;

    backend::RasterState mRasterState;
    BlendingMode mRenderBlendingMode = BlendingMode::OPAQUE;
    TransparencyMode mTransparencyMode = TransparencyMode::DEFAULT;
    backend::FeatureLevel mFeatureLevel = backend::FeatureLevel::FEATURE_LEVEL_1;
    Shading mShading = Shading::UNLIT;
    BlendingMode mBlendingMode = BlendingMode::OPAQUE;
    Interpolation mInterpolation = Interpolation::SMOOTH;
    VertexDomain mVertexDomain = VertexDomain::OBJECT;
    MaterialDomain mMaterialDomain = MaterialDomain::SURFACE;
    CullingMode mCullingMode = CullingMode::NONE;
    AttributeBitset mRequiredAttributes;
    RefractionMode mRefractionMode = RefractionMode::NONE;
    RefractionType mRefractionType = RefractionType::SOLID;
    ReflectionMode mReflectionMode = ReflectionMode::DEFAULT;
    uint64_t mMaterialProperties = 0;

    bool mDoubleSided = false;
    bool mDoubleSidedCapability = false;
    bool mHasShadowMultiplier = false;
    bool mHasCustomDepthShader = false;
    bool mIsDefaultMaterial = false;
    bool mSpecularAntiAliasing = false;

    SamplerInterfaceBlock mSamplerInterfaceBlock;
    BufferInterfaceBlock mUniformInterfaceBlock;
    utils::FixedCapacityVector<std::pair<utils::CString, uint8_t>> mUniformBlockBindings;
    SamplerGroupBindingInfoList mSamplerGroupBindingInfoList;
    SamplerBindingToNameMap mSamplerBindingToNameMap;

}