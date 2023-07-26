## The question of naming

**Renderer** could be the library, and features implemented via **backends**
(e.g. OpenGL backend).
The question is which level is **Renderer**, is it low level with something like a scene-aware
renderer building on top?

## Trying to describre the problem

When rendering a scene, there is a bipartite graph (resources are input to passes which produces resources as output, used by other passes as input, etc.)

At the lowest level, a pass is making drawcall(s) to a graphics API,
using the currently active program. The program takes input from:
* Vertex Buffers/Primitive assembly (VAO in OGL. Anything else?)
* Uniforms (Constants in Vulkan?)
* Textures
* Buffer backed blocks (Uniform Blocks and Shader Storage Blocks)
* The active Pipeline State (fixed functions configuration)

The input can come from:
* Different level of context (in ourmachinery: _execution contexts_)
  * Frame (time, frame number, ...)
  * Viewport (resolution, ...) and Camera (pose, projection)
  * Pass
  * Scene (environment, ...)
* The instance being rendered
  * Vertex/Index buffers (the "Object" which is instanced)
  * Buffer backed blocks (e.g. Bones (matrix palette))

Where are the logical lights sitting?

The overall "Render Graph" (or "Frame Graph") should create/list the passes that are needed.
Each pass should declare its input and output. If done dynamically, this allows interesting tooling.

### Material / shader system

**Disclaimer**: Written as it came, what follows is a bit of an "idea soup".

Overriding a materials (at any level) would ideally allow to control "per pass"
i.e. the idea that a material contains the map of "pass" -> "shader program" seems valid.

ourmachinery make the conceptual distinction between "context state" (~ the current scope of the code provides it)
and the "instance state", that might be shared between distinct passes but can change with each instance/object.
They model it with the same types in code, but the first can easily be provided consolidated as a stack (mapping the code scope),
while the second is more "ad-hoc", provided with each instance.

It seems the context could be fixed for a pass (takable as const ref).
But does the pass mean "applied to a whole scene"?
In which case then there is potential for per instance/per object/per material variation
In particular, since the Shader Program depends on the material,
the program is not uniform per pass, it might vary per instance.

There is a tension between "features activated/available per instance", selection of the matching
program variant, and providing such values.
Providing can be done in distinct ways (see AZDO talks: buffers with array and index, buffer with driver side selection of one index before each draw call, textures, uniforms),
some engine hide this with their opaque preprocessor injecting the corresponding shader code for "data access",
client shader code then using those "get_whatever()" function to access the values.
Also, the open/closed principle would dictate that the client can add custom features without touching the renderer code.
This implies some dynamic design, where instances can maintain a dynamic collection of features and their data, of unbounded types.
Yet, if the actual Render Graph is somehow hardcoded into the renderer, it might limit what can be achieved with such features?
In particular, what if a pass needs to not enable some specific feature, how would that be controlled?
(e.g. a scene where shadows should be of the instance without its animations applied [not a realistic example, I know]).
Maybe the feature, added by the client, could have some filter on the pass. But what if a material override expect another feature set for the same pass?
Also, some features data is dynamic and need updating(e.g. computing bones matrix palette, each frame).
Also, some features are common and should be provided by the lib, not having clients copy it each project.
Base feature might simply be provided by the lib, and then used by the clients from there.

A complication with dynamic feature set on material (impacting shader code):
* The shader code variation becomes dynamic, and this can lead to new variants compilation at "unintended" moments during runtime.

Idea: should the library let the client actually "preload" the material, providing the requested feature at this point?
Reducing the Material shader options to a static "map" indexed by predefined attributes.

The material/shader sytem should ideally ouptut:
* non-context resources
* A shader program (picking correct variant)
to be fed into the lower level renderer.

## Validation scene

* 3D meshes made of multiple parts.
  * Some transparency.
  * PBR material.
  * Custom material.
  * Skeletal animation (distinct timelines).

* Billboards

* Skybox

* Full screen / viewport effect

* Text
  * World space
  * Screen space

* Multiple viewports

## Hand waving processus

beginFrame()
    //scope frame statistics


for each viewport
    // scope viewport statistics

    get camera
    get scene

    prepareScene()
        // TODO must be moved down when we have culling
        renderList = makeRenderList(scene.mInstances if instance.mCastShadows,
                                    DEPTH_PASS)

        for each light in scene.mLights if light.mCastShadows:
            renderShadowmap(light)
                setCamera(light)
                renderList.sort_depth_from_camera()
                pick shadow texture for rendering into
                renderList.draw()

    renderScene()
        setCamera(camera)

        setupLights()
            // Fill UBO

        renderList = makeRenderList(scene.mInstances,
                                    MAIN_PASS)

        renderList.filter(opaque).sort_depth_from_camera() // closer first
        setPipelineState(depth)
        renderList.draw()

        renderList.filter(transparent).sort_depth_from_camera(inverted) // further first
        setPipelineState(blending, depth)
        renderList.draw()

    renderFullScreenEffect()

    blit viewport to default framebuffer

## Data Model

struct Camera
{
    Pose;
    Projection;
};

struct Viewport
{
    Camera mCamera;
    Scene mScene;
};

struct GeometryInstance
{
    Geometry;
    Pose; // model transformation local->world
    Material mMaterialOverride;

    bool mCastShadows, mReceiveShadows;

    bool mVisible;

    bool mInterpolate; // can be extended with an enum offering several interpolation methods

    // some form of Skeleton
};

struct LightInstance
{
    // Godot Light
    LightType; (directional, omni, spot) // or distinct Cpp type?
    Color mColor
    bool mShadow
    Color mShadowColor

    // Godot LightInstance
    Pose;
}

struct Scene
{
    <LightInstance> mLights;
    <GeometryInstance> mInstances;
};

struct Geometry
{
    <Part>;
};

struct Part
{
    // Handles into OpenGL buffers / arrays
    Material;
};

struct Material
{
    // Textures
    // Uniforms
    // Shader, somehow (this will be more complicated, with effects and passes thrown in the mix...)
};

// Keep (at least) totals, per frame, per viewport
struct RendererStatistics
{
    mDrawcalls;
    mVertices;
    mPrimitives;
    ...

    mShaders; (what does that mean? total compiled? currently active?)
};

struct RenderList
{
};
