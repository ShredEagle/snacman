## The question of naming

**Renderer** could be the library, and features implemented via **backends**
(e.g. OpenGL backend).
The question is which level is **Renderer**, is it low level with something like a scene-aware
renderer building on top?

## Trying to describre the problem

My current view, in essence a renderer is two things:
* an API allowing clients to render frames of their own composition.
* an implementation providing the API and rendering with the best speed and quality it can provide.

A lot of tension exists in providing a good API, not exposing too much while allowing clients
to customize rendering (e.g. custom materials and effects),
provided by an efficient implementation.

### Frame graphs

When rendering a scene, there is a bipartite graph (resources are input to passes which produces resources as output, used by other passes as input, etc.)

At the lowest level, a pass is making drawcall(s) to a graphics API,
using the currently active program. The program takes input from:
* Vertex Buffers/Primitive assembly (VAO in OGL. Anything else?)
* Uniforms (Constants in Vulkan?)
* Buffer backed blocks (Uniform Blocks and Shader Storage Blocks)
* Textures
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
They model it with the same types in code, but the first can easily be provided as a consolidated stack (mapping to the code scopes),
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

## Questions and Decisions

### Prelude: Of perfectionnist humans

The path is long, especially because it is filled with questions and design decisions.

To try to keep decision-paralysis under control, I have to be better at taking a direction, accepting to revisit early / often.

### How should one wrap graphics library (OpenGL) calls?

We can read in the litterature / see in presentation that "developers usually have some inhouse abstraction sitting on top of the API".
We use a loader (glad) to give access to OpenGL functions, it does not seem to easily let us insert code.

Should we look for a way to insert ourselve in between the loader and the actual driver call?
This might not be robust to separately compiled libraries.

Should we model some API object? It could be static and easily accessible by all client code.
But how do we allow "wrapped resources" such as textures to use it for freeing? Overloaded ctors?

How to handle binds?

How to count allocated memory? For example, calling glBufferData() discard the buffer memory if it was already allocated, but how to track that?

### The decision log

#### Binding Meshes to shader programs

**Decision DMB**: Dynamic mesh binding is done via the "shader binding set" determined via introspection at shader compile time (`IntrospectProgram`).
The solution is closes to what is described by https://www.gamedev.net/forums/topic/713364-vertex-attribute-abstraction-design/5452686/.

#### VAO cache

Designing a VAO cache is always proving difficult for me.

The vertex format is kept open (is it a good decision?), and the system provides dynamic mapping of buffers to custom programs -> There is not closed list of potential VAOs.
We can construct VAO as we need them, but this is a lengthy operation and we need to cache the results.

What we identify as VAO identity:
* Shader program has the same semantic and type, at the same attribute indices. (This means distinct program can share a VAO as long as this holds true).
* Buffer have the same "data-format" (element sizes, component types, relative offset (i.e. same interleaving))
* Without ARB_vertex_attrib_binding (VAB): Same actual buffers (applying offsets when drawing does not reqire a distinct VAO).

* We expect the programs to exist in the largest number (larger than variations in data-format, and even vertex buffers), we make a link from the program to its VAO (via reference, for sharing).
* Without VAB, GL buffer identity should cover all cases of changes in data-format (i.e. if the data-format is different, it will always be via a different GL buffer). The program links to a repository of VAOs, one for each "set of buffers" it encountered.

**Decision VSU**: To make the last point easier, we decided to merge the per-instance (glDivisor 1) "vertex" attributes into the Vertex Stream at the `Part` level.
This decision is not very strong, as it blurs some lines:
initially, `VertexStream` was only for per-part data (actual vertex arrays, index buffer).
Now it also holds data that cannot be loaded with the model (instance positions, material index, ...), and has to be patched-in.
On the other hand it has other benefits:
* simplyfing some signatures (no need to pass a collection of extra buffers alongside the main VertexStream).

#### OpenGL state caching

It is tempting, to avoid potential redundant state setting.
But Nicol Bolas points out how dangerous it is, (since it can go out of sync with the real GL state)[https://stackoverflow.com/a/38887089/1027706].

#### Bounding boxes

Assimp can compute bounding boxes for meshes (i.e. `Part`), and we can unite all parts AABB to get the `Object` AABB.

A complication arises for Nodes: should they cache their bounding box?
Nodes can have an `Object`, plus potential children nodes (with transformations).
The transformations change the bounding boxes, and if they are dynamic it means cached AABB has to be recomputed each time.

## Validation scene

* 3D meshes made of multiple parts.
  * Some transparency.
  * PBR material.
  * Custom material.
  * Skeletal animation (distinct timelines).
    * Have the skeletal animation implemented client-side, to validate that the API let clients implement their own systems.

* Billboards

* Skybox

* Full screen / viewport effect

* Text
  * World space
  * Screen space

* Multiple viewports

## Hand waving processus - godot inspired

Attention: not necessarily what we want to implement, this is a bit rigid.

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
