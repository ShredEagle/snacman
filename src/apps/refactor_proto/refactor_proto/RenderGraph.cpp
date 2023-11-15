#include "RenderGraph.h"

#include "Cube.h"
#include "GlApi.h"
#include "Json.h"
#include "Loader.h"
#include "Logging.h"
#include "Profiling.h"
#include "RendererReimplement.h" // TODO Ad 2023/10/18: Should get rid of this repeated implementation
#include "SetupDrawing.h"

#include <handy/vector_utils.h>

#include <imguiui/ImguiUi.h>

#include <math/Transformations.h>
#include <math/Vector.h>
#include <math/VectorUtilities.h>

#include <platform/Path.h>

#include <renderer/BufferLoad.h>
#include <renderer/Query.h>
#include <renderer/ScopeGuards.h>

#define NOMINMAX
#include "../../../libs/snac-renderer/snac-renderer/3rdparty/nvtx/include/nvtx3/nvtx3.hpp"

#include <array>
#include <fstream>


namespace ad::renderer {


const char * gVertexShader = R"#(
    #version 420

    in vec3 ve_Position_local;
    in vec4 ve_Color;

    in uint in_ModelTransformIdx;

    layout(std140, binding = 1) uniform ViewBlock
    {
        mat4 worldToCamera;
        mat4 projection;
        mat4 viewingProjection;
    };

    // TODO #ssbo Use a shader storage block, due to the unbounded nature of the number of instances
    layout(std140, binding = 0) uniform LocalToWorldBlock
    {
        mat4 localToWorld[512];
    };

    out vec4 ex_Color;

    void main()
    {
        ex_Color = ve_Color;
        gl_Position = viewingProjection 
            * localToWorld[in_ModelTransformIdx] 
            * vec4(ve_Position_local, 1.f);
    }
)#";

const char * gFragmentShader = R"#(
    #version 420

    in vec4 ex_Color;

    layout(std140, binding = 2) uniform FrameBlock
    {
        float time;
    };

    out vec4 color;

    void main()
    {
        // TODO Ad 2023/10/12: #transparency This would be a nice transparency effect
        //color = vec4(ex_Color.rgb, mod(time, 1.0));
        color = vec4(ex_Color.rgb, 1.) * vec4(mod(time, 1.0));
    }
)#";

constexpr math::Degree<float> gInitialVFov{50.f};
constexpr float gNearZ{-0.1f};
constexpr float gMinFarZ{-25.f}; // Note: minimum in **absolute** value (i.e. the computed far Z can only be inferior to this).

/// @brief The orbitalcamera is moved away by the largest model dimension times this factor.
[[maybe_unused]] constexpr float gRadialDistancingFactor{1.5f};
/// @brief The far plane will be at least at this factor times the model depth.
constexpr float gDepthFactor{20.f};

constexpr std::size_t gMaxDrawInstances = 2048;

namespace {

    resource::ResourceFinder makeResourceFinder()
    {
        filesystem::path assetConfig = platform::getExecutableFileDirectory() / "assets.json";
        if(exists(assetConfig))
        {
            Json config = Json::parse(std::ifstream{assetConfig});
            
            // This leads to an ambibuity on the path ctor, I suppose because
            // the iterator value_t is equally convertible to both filesystem::path and filesystem::path::string_type
            //return resource::ResourceFinder(config.at("prefixes").begin(),
            //                                config.at("prefixes").end());

            // Take the silly long way
            std::vector<std::string> prefixes{
                config.at("prefixes").begin(),
                config.at("prefixes").end()
            };
            std::vector<std::filesystem::path> prefixPathes;
            prefixPathes.reserve(prefixes.size());
            for (auto & prefix : prefixes)
            {
                prefixPathes.push_back(std::filesystem::canonical(prefix));
            }
            return resource::ResourceFinder(prefixPathes.begin(),
                                            prefixPathes.end());
        }
        else
        {
            return resource::ResourceFinder{platform::getExecutableFileDirectory()};
        }
    }


    Handle<Effect> makeSimpleEffect(Storage & aStorage)
    {
        aStorage.mProgramConfigs.emplace_back();
        // Compiler error (msvc) workaround, by taking the initializer list out
        std::initializer_list<IntrospectProgram::TypedShaderSource> il = {
            {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(std::string{gVertexShader}, "RenderGraph.cpp")},
            {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(gFragmentShader, "RenderGraph.cpp")},
        };
        aStorage.mPrograms.push_back(ConfiguredProgram{
            .mProgram = IntrospectProgram{
                il,
                "simple-RenderGraph.cpp",
            },
            .mConfig = &aStorage.mProgramConfigs.back(),
        });

        Effect effect{
            .mTechniques{makeVector(
                Technique{
                    .mAnnotations{
                        {"pass", "forward"},
                    },
                    .mConfiguredProgram = &aStorage.mPrograms.back(),
                }
            )},
            .mName = "simple-effect",
        };
        aStorage.mEffects.push_back(std::move(effect));

        return &aStorage.mEffects.back();
    }


    /// @brief Layout compatible with the shader's `ViewBlock`
    struct GpuViewBlock
    {
        GpuViewBlock(
            math::AffineMatrix<4, GLfloat> aWorldToCamera,
            math::Matrix<4, 4, GLfloat> aProjection 
        ) :
            mWorldToCamera{aWorldToCamera},
            mProjection{aProjection},
            mViewingProjection{aWorldToCamera * aProjection}
        {}

        GpuViewBlock(const Camera & aCamera) :
            GpuViewBlock{aCamera.getParentToCamera(), aCamera.getProjection()}
        {}

        math::AffineMatrix<4, GLfloat> mWorldToCamera; 
        math::Matrix<4, 4, GLfloat> mProjection; 
        math::Matrix<4, 4, GLfloat> mViewingProjection;
    };


    void loadFrameUbo(const graphics::UniformBufferObject & aUbo)
    {
        GLfloat time =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000.f;
        proto::loadSingle(aUbo, time, graphics::BufferHint::DynamicDraw);
    }


    void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera)
    {
        proto::loadSingle(aUbo, GpuViewBlock{aCamera}, graphics::BufferHint::DynamicDraw);
    }


    void draw(const PassCache & aPassCache,
              const RepositoryUbo & aUboRepository,
              const RepositoryTexture & aTextureRepository)
    {
        //TODO Ad 2023/08/01: META todo, should we have "compiled state objects" (a-la VAO) for interface bocks, textures, etc
        // where we actually map a state setup (e.g. which texture name to which image unit and which sampler)
        // those could be "bound" before draw (potentially doing some binds and uniform setting, but not iterating the program)
        // (We could even separate actual texture from the "format", allowing to change an underlying texture without revisiting the program)
        // This would address the warnings repetitions (only issued when the compiled state is (re-)generated), and be better for perfs.

        GLuint firstInstance = 0; 
        for (const DrawCall & aCall : aPassCache.mCalls)
        {
            PROFILER_SCOPE_SECTION("drawcall_iteration", CpuTime);

            PROFILER_PUSH_SECTION("discard_2", CpuTime);
            const IntrospectProgram & selectedProgram = *aCall.mProgram;
            const graphics::VertexArrayObject & vao = *aCall.mVao;
            PROFILER_POP_SECTION;

            // TODO Ad 2023/10/05: #perf #azdo 
            // Only change what is necessary, instead of rebiding everything each time.
            // Since we sorted our draw calls, it is very likely that program remain the same, and VAO changes.
            {
                PROFILER_PUSH_SECTION("bind_VAO", CpuTime);
                graphics::ScopedBind vaoScope{vao};
                PROFILER_POP_SECTION;
                
                {
                    PROFILER_SCOPE_SECTION("set_buffer_backed_blocks", CpuTime);
                    // TODO #repos This should be consolidated
                    RepositoryUbo uboRepo{aUboRepository};
                    if(aCall.mCallContext)
                    {
                        RepositoryUbo callRepo{aCall.mCallContext->mUboRepo};
                        uboRepo.merge(callRepo);
                    }
                    setBufferBackedBlocks(selectedProgram, uboRepo);
                }

                {
                    PROFILER_SCOPE_SECTION("set_textures", CpuTime);
                    // TODO #repos This should be consolidated
                    RepositoryTexture textureRepo{aTextureRepository};
                    if(aCall.mCallContext)
                    {
                        RepositoryTexture callRepo{aCall.mCallContext->mTextureRepo};
                        textureRepo.merge(callRepo);
                    }
                    setTextures(selectedProgram, textureRepo);
                }

                PROFILER_PUSH_SECTION("bind_program", CpuTime);
                graphics::ScopedBind programScope{selectedProgram};
                PROFILER_POP_SECTION;

                {
                    // TODO Ad 2023/08/23: Measuring GPU time here has a x2 impact on cpu performance
                    // Can we have efficient GPU measures?
                    PROFILER_SCOPE_SECTION("glDraw_call", CpuTime/*, GpuTime*/);
                    
                    gl.MultiDrawElementsIndirect(
                        aCall.mPrimitiveMode,
                        aCall.mIndicesType,
                        (void *)(firstInstance * sizeof(DrawElementsIndirectCommand)),
                        aCall.mPartCount,
                        sizeof(DrawElementsIndirectCommand));
                }
            }
            firstInstance += aCall.mPartCount;
        }
    }

    void populatePartList(PartList & aPartList, const Node & aNode, const Pose & aParentPose, const Material * aMaterialOverride)
    {
        const Instance & instance = aNode.mInstance;
        const Pose & localPose = instance.mPose;
        Pose absolutePose = aParentPose.transform(localPose);

        if(instance.mMaterialOverride)
        {
            aMaterialOverride = &*instance.mMaterialOverride;
        }

        if(Object * object = aNode.mInstance.mObject;
           object != nullptr)
        {
            for(const Part & part: object->mParts)
            {
                const Material * material =
                    aMaterialOverride ? aMaterialOverride : &part.mMaterial;

                aPartList.mParts.push_back(&part);
                aPartList.mMaterials.push_back(material);
                // pushed after
                aPartList.mTransformIdx.push_back((GLsizei)aPartList.mInstanceTransforms.size());
            }

            aPartList.mInstanceTransforms.push_back(
                math::trans3d::scaleUniform(absolutePose.mUniformScale)
                * math::trans3d::translate(absolutePose.mPosition));
        }

        for(const auto & child : aNode.mChildren)
        {
            populatePartList(aPartList, child, absolutePose, aMaterialOverride);
        }
    }


    /// @brief Find the program satisfying all annotations.
    /// @return The first program in aEffect for which all annotations are matching (i.e. present and same value),
    /// or a null handle otherwise.
    template <class T_iterator>
    Handle<ConfiguredProgram> getProgram(const Effect & aEffect,
                                         T_iterator aAnnotationsBegin, T_iterator aAnnotationsEnd)
    {
        for (const Technique & technique : aEffect.mTechniques)
        {
            if(std::all_of(aAnnotationsBegin, aAnnotationsEnd,
                    [&technique](const Technique::Annotation & aRequiredAnnotation)
                    {
                        const auto & availableAnnotations = technique.mAnnotations;
                        if(auto found = availableAnnotations.find(aRequiredAnnotation.mCategory);
                            found != availableAnnotations.end())
                        {
                            return (found->second == aRequiredAnnotation.mValue);
                        }
                        return false;
                    }))
            {
                return technique.mConfiguredProgram;
            }
        }

        return nullptr;
    }


    /// @brief Specializes getProgram, returning first program matching provided pass name.
    Handle<ConfiguredProgram> getProgramForPass(const Effect & aEffect, StringKey aPassName)
    {
        const std::array<Technique::Annotation, 2> annotations{/*aggregate init of std::array*/{/*aggregate init of inner C-array*/
            {"pass", aPassName},
            {"pass", aPassName},
        }};
        return getProgram(aEffect, annotations.begin(), annotations.end());
    }
    

    /// @brief Returns the VAO matching the given program and part.
    /// Under the hood, the VAO is cached.
    Handle<graphics::VertexArrayObject> getVao(const ConfiguredProgram & aProgram,
                                               const Part & aPart,
                                               Storage & aStorage)
    {
        // This is a common mistake, it would be nice to find a safer way
        assert(aProgram.mConfig);

        // Note: the config is via "handle", hosted by in cache that is mutable, so loosing the constness is correct.
        return
            [&, &entries = aProgram.mConfig->mEntries]() 
            -> graphics::VertexArrayObject *
            {
                if(auto foundConfig = std::find_if(entries.begin(), entries.end(),
                                                [&aPart](const auto & aEntry)
                                                {
                                                    return aEntry.mVertexStream == aPart.mVertexStream;
                                                });
                    foundConfig != entries.end())
                {
                    return foundConfig->mVao;
                }
                else
                {
                    aStorage.mVaos.push_back(prepareVAO(aProgram.mProgram, *aPart.mVertexStream));
                    entries.push_back(
                        ProgramConfig::Entry{
                            .mVertexStream = aPart.mVertexStream,
                            .mVao = &aStorage.mVaos.back(),
                        });
                    return entries.back().mVao;
                }
            }();
    }


    // TODO Ad 2023/10/05: for OpenGL resource, maybe we should directly use the GL name?
    // or do we want to reimplement Handle<> in term of index in storage containers?
    /// @brief Hackish class, associating a resource to an integer value.
    ///
    /// The value is suitable to compose a DrawEntry::Key via bitwise operations.
    template <class T_resource, unsigned int N_valueBits>
    struct ResourceIdMap
    {
        static_assert(N_valueBits <= 16);
        static constexpr std::uint16_t gMaxValue = (std::uint16_t)((1 << N_valueBits) - 1);

        ResourceIdMap()
        {
            // By adding the nullptr as the zero value, this helps debugging
            get(nullptr);
            assert(get(nullptr)== 0);
        }

        /// @brief Get the value associated to provided resource.
        /// A given map instance will always return the same value for the same resource.
        std::uint16_t get(Handle<T_resource> aResource)
        {
            assert(mResourceToId.size() < gMaxValue);
            auto [iterator, didInsert] = mResourceToId.try_emplace(aResource,
                                                                   (std::uint16_t)mResourceToId.size());
            return iterator->second;
        }

        /// @brief Return the resource from it value.
        Handle<T_resource> reverseLookup(std::uint16_t aValue)
        {
            if(auto found = std::find_if(mResourceToId.begin(), mResourceToId.end(),
                                         [aValue](const auto & aPair)
                                         {
                                             return aPair.second == aValue;
                                         });
               found != mResourceToId.end())
            {
                return found->first;
            }
            throw std::logic_error{"The provided value is not present in this ResourceIdMap."};
        }

        std::unordered_map<Handle<T_resource>, std::uint16_t> mResourceToId;
    };


    /// @brief Associate an integer key to a part.
    /// Used to sort an array with one entry for each part, by manually composing a key by sort dimensions.
    /// @see https://realtimecollisiondetection.net/blog/?p=86
    struct DrawEntry
    {
        using Key = std::uint64_t;
        static constexpr Key gInvalidKey = std::numeric_limits<Key>::max();

        /// @brief Order purely by key.
        /// @param aRhs 
        /// @return 
        bool operator<(const DrawEntry & aRhs) const
        {
            return mKey < aRhs.mKey;
        }

        Key mKey = 0;
        std::size_t mPartListIdx;
    };


    constexpr std::uint64_t makeMask(unsigned int aBitCount)
    {
        assert(aBitCount <= 64);
        return (1 << aBitCount) - 1;
    }


    PassCache preparePass(StringKey aPass,
                          const PartList & aPartList,
                          Storage & aStorage)
    {
        PROFILER_SCOPE_SECTION("prepare_pass", CpuTime);

        constexpr unsigned int gProgramIdBits = 16;
        constexpr std::uint64_t gProgramIdMask = makeMask(gProgramIdBits);
        using ProgramId = std::uint16_t;
        assert(aStorage.mPrograms.size() < (1 << gProgramIdBits));

        //constexpr unsigned int gPrimitiveModes = 12; // I counted 12 primitive modes
        //unsigned int gPrimitiveModeIdBits = (unsigned int)std::ceil(std::log2(gPrimitiveModes));

        constexpr unsigned int gVaoBits = 10;
        constexpr std::uint64_t gVaoIdMask = makeMask(gVaoBits);
        assert(aStorage.mVaos.size() < (1 << gVaoBits));
        using VaoId = std::uint16_t;

        constexpr unsigned int gMaterialContextBits = 10;
        constexpr std::uint64_t gMaterialContextIdMask = makeMask(gMaterialContextBits);
        assert(aStorage.mMaterialContexts.size() < (1 << gMaterialContextBits));
        using MaterialContextId = std::uint16_t;

        ResourceIdMap<ConfiguredProgram, gProgramIdBits> programToId;
        ResourceIdMap<graphics::VertexArrayObject, gVaoBits> vaoToId;
        ResourceIdMap<MaterialContext, gMaterialContextBits> materialContextToId;

        //
        // For each part, associated it to its sort key and store them in an array
        // (this will already prune parts for which there is no program for aPass)
        //
        std::vector<DrawEntry> entries;
        entries.reserve(aPartList.mParts.size());

        for (std::size_t partIdx = 0; partIdx != aPartList.mParts.size(); ++partIdx)
        {
            const Part & part = *aPartList.mParts[partIdx];
            const Material & material = *aPartList.mMaterials[partIdx];

            // TODO Ad 2023/10/05: Also add those as sorting dimensions (to the LSB)
            assert(part.mPrimitiveMode == GL_TRIANGLES);
            assert(part.mVertexStream->mIndicesType == GL_UNSIGNED_INT);
            
            if(Handle<ConfiguredProgram> configuredProgram = getProgramForPass(*material.mEffect, aPass);
               configuredProgram)
            {
                Handle<graphics::VertexArrayObject> vao = getVao(*configuredProgram, part, aStorage);

                ProgramId programId = programToId.get(configuredProgram);
                VaoId vaoId = vaoToId.get(vao);
                MaterialContextId materialContextId = materialContextToId.get(material.mContext);

                std::uint64_t key = vaoId ;
                key |= materialContextId << gVaoBits;
                key |= programId << (gVaoBits + gMaterialContextBits);

                entries.push_back(DrawEntry{.mKey = key, .mPartListIdx = partIdx});
            }
        }

        //
        // Sort the entries
        //
        {
            PROFILER_SCOPE_SECTION("sort_draw_entries", CpuTime);
            std::sort(entries.begin(), entries.end());
        }

        //
        // Traverse the sorted array to generate the actual draw commands and draw calls
        //
        PassCache result;
        DrawEntry::Key drawKey = DrawEntry::gInvalidKey;
        for(const DrawEntry & entry : entries)
        {
            const Part & part = *aPartList.mParts[entry.mPartListIdx];
            const VertexStream & vertexStream = *part.mVertexStream;

            // If true: this is the start of a new DrawCall
            if(entry.mKey != drawKey)
            {
                // Record the new drawkey
                drawKey = entry.mKey;

                // Push the new DrawCall
                result.mCalls.push_back(
                    DrawCall{
                        .mPrimitiveMode = part.mPrimitiveMode,
                        .mIndicesType = vertexStream.mIndicesType,
                        .mProgram = 
                            &programToId.reverseLookup(gProgramIdMask & (drawKey >> (gVaoBits + gMaterialContextBits)))
                                ->mProgram,
                        .mVao = vaoToId.reverseLookup(gVaoIdMask & drawKey),
                        .mCallContext = materialContextToId.reverseLookup(gMaterialContextIdMask & (drawKey >> gVaoBits)),
                        .mPartCount = 0,
                    }
                );
            }
            
            // Increment the part count of current DrawCall
            ++result.mCalls.back().mPartCount;

            const std::size_t indiceSize = graphics::getByteSize(vertexStream.mIndicesType);

            // Indirect draw commands do not accept a (void*) offset, but a number of indices (firstIndex).
            // This means the offset into the buffer must be aligned with the index size.
            assert((vertexStream.mIndexBufferView.mOffset %  indiceSize) == 0);

            result.mDrawCommands.push_back(
                DrawElementsIndirectCommand{
                    .mCount = part.mIndicesCount,
                    // TODO Ad 2023/09/26: #bench Is it worth it to group identical parts and do "instanced" drawing?
                    // For the moment, draw a single instance for each part (i.e. no instancing)
                    .mInstanceCount = 1,
                    .mFirstIndex = (GLuint)(vertexStream.mIndexBufferView.mOffset / indiceSize)
                                    + part.mIndexFirst,
                    .mBaseVertex = part.mVertexFirst,
                    .mBaseInstance = (GLuint)result.mDrawInstances.size(), // pushed below
                }
            );

            const Material & material = *aPartList.mMaterials[entry.mPartListIdx];

            result.mDrawInstances.push_back(DrawInstance{
                .mInstanceTransformIdx = aPartList.mTransformIdx[entry.mPartListIdx],
                .mMaterialIdx = (GLsizei)material.mPhongMaterialIdx,
            });
        }

        return result;
    }

} // unnamed namespace


PartList Scene::populatePartList() const
{
    PROFILER_SCOPE_SECTION("populate_draw_list", CpuTime);

    PartList partList;
    renderer::populatePartList(partList, mRoot, mRoot.mInstance.mPose, 
                               mRoot.mInstance.mMaterialOverride ? &*mRoot.mInstance.mMaterialOverride : nullptr);
    return partList;
}


HardcodedUbos::HardcodedUbos(Storage & aStorage)
{
    aStorage.mUbos.emplace_back();
    mFrameUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gFrame, mFrameUbo);

    aStorage.mUbos.emplace_back();
    mViewingUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gView, mViewingUbo);

    aStorage.mUbos.emplace_back();
    mModelTransformUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLocalToWorld, mModelTransformUbo);
}


enum class EscKeyBehaviour
{
    Ignore,
    Close,
};

//TODO Ad 2023/07/26: Move to the glfw app library,
// only register the subset actually provided by T_callbackProvider (so it does not need to implement them all)
template <class T_callbackProvider>
void registerGlfwCallbacks(graphics::AppInterface & aAppInterface,
                           T_callbackProvider & aProvider,
                           EscKeyBehaviour aEscBehaviour,
                           const imguiui::ImguiUi * aImguiUi)
{
    using namespace std::placeholders;

    aAppInterface.registerMouseButtonCallback(
        [aImguiUi, &aProvider](int button, int action, int mods, double xpos, double ypos)
        {
            if(!aImguiUi->isCapturingMouse())
            {
                aProvider.callbackMouseButton(button, action, mods, xpos, ypos);
            }
        });
    aAppInterface.registerCursorPositionCallback(
        [aImguiUi, &aProvider](double xpos, double ypos)
        {
            if(!aImguiUi->isCapturingMouse())
            {
                aProvider.callbackCursorPosition(xpos, ypos);
            }
        });
    aAppInterface.registerScrollCallback(
        [aImguiUi, &aProvider](double xoffset, double yoffset)
        {
            if(!aImguiUi->isCapturingMouse())
            {
                aProvider.callbackScroll(xoffset, yoffset);
            }
        });

    switch(aEscBehaviour)
    {
        case EscKeyBehaviour::Ignore:
            aAppInterface.registerKeyCallback([aImguiUi, &aProvider](int key, int scancode, int action, int mods)
            {
                if(!aImguiUi->isCapturingKeyboard())
                {
                    aProvider.callbackKeyboard(key, scancode, action, mods);
                }
            });
            break;
        case EscKeyBehaviour::Close:
            aAppInterface.registerKeyCallback(
                [&aAppInterface, &aProvider, aImguiUi](int key, int scancode, int action, int mods)
                {
                    if(!aImguiUi->isCapturingKeyboard())
                    {
                        // TODO would be cleaner to factorize that and the ApplicationGlfw::default_key_callback
                        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
                        {
                            aAppInterface.requestCloseApplication();
                        }
                        aProvider.callbackKeyboard(key, scancode, action, mods);
                    }
                });
            break;
    }
}


RenderGraph::RenderGraph(const std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                         const std::filesystem::path & aSceneFile,
                         const imguiui::ImguiUi & aImguiUi) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    // At the moment, hardcode a maximum number
    mInstanceStream{makeInstanceStream(mStorage, gMaxDrawInstances)},
    mLoader{makeResourceFinder()},
    mUbos{mStorage},
    mCameraControl{mGlfwAppInterface->getWindowSize(),
                   gInitialVFov,
                   Orbital{2/*initial radius*/}
    }
{
    registerGlfwCallbacks(*mGlfwAppInterface, mCameraControl, EscKeyBehaviour::Close, &aImguiUi);
    registerGlfwCallbacks(*mGlfwAppInterface, mFirstPersonControl, EscKeyBehaviour::Close, &aImguiUi);

    mScene = mLoader.loadScene(aSceneFile, "effects/Mesh.sefx", mInstanceStream, mStorage);
    /*const*/Node & model = mScene.mRoot.mChildren.front();

    // TODO Ad 2023/10/03: Sort out this bit of logic: remove hardcoded sections,
    // better handle camera placement / projections scene wide
    // Setup instance and camera poses
    {
        // TODO For a "Model viewer mode", automatically handle scaling via model bounding box.
        // Note that visually this has the same effect as moving the radial camera away
        // (it is much harder when loading and navigating inside an environment)

        // move the orbital camera away, depending on the model size
        //mCameraControl.mOrbital.mSpherical.radius() = 
        //    std::max(mCameraControl.mOrbital.mSpherical.radius(),
        //             gRadialDistancingFactor * (*model.mAabb.mDimension.getMaxMagnitudeElement()));


        // TODO #camera do this only the correct setup, on each projection change
        // We waited to load the model before setting up the projection,
        // in order to set the far plane based on the model depth.
        mCamera.setupOrthographicProjection({
            .mAspectRatio = math::getRatio<GLfloat>(mGlfwAppInterface->getWindowSize()),
            // TODO #camera
            .mViewHeight = mCameraControl.getViewHeightAtOrbitalCenter(),
            .mNearZ = gNearZ,
            .mFarZ = std::min(gMinFarZ, -gDepthFactor * model.mAabb.depth())
        });

        mCamera.setupPerspectiveProjection({
            .mAspectRatio = math::getRatio<GLfloat>(mGlfwAppInterface->getWindowSize()),
            .mVerticalFov = gInitialVFov,
            .mNearZ = gNearZ,
            .mFarZ = std::min(gMinFarZ, -gDepthFactor * model.mAabb.depth())
        });
    }

    // Add basic shapes to the the scene
    Handle<Effect> simpleEffect = makeSimpleEffect(mStorage);
    auto [triangle, cube] = loadTriangleAndCube(mStorage, simpleEffect, mInstanceStream);
    mScene.addToRoot(triangle);
    mScene.addToRoot(cube);
}


void RenderGraph::update(float aDeltaTime)
{
    PROFILER_SCOPE_SECTION("RenderGraph::update()", CpuTime);
    mFirstPersonControl.update(aDeltaTime);
}


// TODO Ad 2023/09/26: Could be splitted between the part list load (which should be valid accross passes)
// and the pass cache load (which might only be valid for a single pass)
void RenderGraph::loadDrawBuffers(const PartList & aPartList,
                                  const PassCache & aPassCache)
{
    PROFILER_SCOPE_SECTION("load_draw_buffers", CpuTime, BufferMemoryWritten);

    assert(aPassCache.mDrawInstances.size() <= gMaxDrawInstances);

    proto::load(*mUbos.mModelTransformUbo,
                std::span{aPartList.mInstanceTransforms},
                graphics::BufferHint::DynamicDraw);

    proto::load(*mInstanceStream.mVertexBufferViews.at(0).mGLBuffer,
                std::span{aPassCache.mDrawInstances},
                graphics::BufferHint::DynamicDraw);

    proto::load(mIndirectBuffer,
                std::span{aPassCache.mDrawCommands},
                graphics::BufferHint::DynamicDraw);
}


void RenderGraph::drawUi()
{
    mSceneGui.present(mScene);
}


void RenderGraph::render()
{
    PROFILER_SCOPE_SECTION("RenderGraph::render()", CpuTime, GpuTime, BufferMemoryWritten);
    nvtx3::mark("RenderGraph::render()");
    NVTX3_FUNC_RANGE();

    // TODO: How to handle material/program selection while generating the part list,
    // if the camera (or pass itself?) might override the materials?
    PartList partList = mScene.populatePartList();

    // TODO should be done per pass
    PassCache passCache = preparePass("forward", partList, mStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(partList, passCache);

    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO handle pipeline state with an abstraction
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // We implemented alpha testing (cut-out), no blending.
    glDisable(GL_BLEND);

    // TODO #camera: handle this when necessary
    // Update camera to match current values in orbital control.
    //mCamera.setPose(mCameraControl.mOrbital.getParentToLocal());
    //if(mCamera.isProjectionOrthographic())
    //{
    //    // Note: to allow "zooming" in the orthographic case, we change the viewed height of the ortho projection.
    //    // An alternative would be to apply a scale factor to the camera Pose transformation.
    //    changeOrthographicViewportHeight(mCamera, mCameraControl.getViewHeightAtOrbitalCenter());
    //}
    mCamera.setPose(mFirstPersonControl.getParentToLocal());

    {
        PROFILER_SCOPE_SECTION("load_dynamic_UBOs", CpuTime, GpuTime, BufferMemoryWritten);
        loadFrameUbo(*mUbos.mFrameUbo); // TODO Separate, the frame ubo should likely be at the top, once per frame
        // Note in a more realistic application, several cameras would be used per frame.
        loadCameraUbo(*mUbos.mViewingUbo, mCamera);
    }

    RepositoryTexture textureRepository;

    // Use the same indirect buffer for all drawings
    graphics::bind(mIndirectBuffer, graphics::BufferType::DrawIndirect);

    // Reset texture bindings (to make sure that texturing does not work by "accident")
    // this could be extended to reset everything in the context.
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    // TODO should be done once per viewport
    glViewport(0, 0,
               mGlfwAppInterface->getFramebufferSize().width(),
               mGlfwAppInterface->getFramebufferSize().height());

    {
        PROFILER_SCOPE_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
        draw(passCache, mUbos.mUboRepository, textureRepository);
    }
}


} // namespace ad::renderer