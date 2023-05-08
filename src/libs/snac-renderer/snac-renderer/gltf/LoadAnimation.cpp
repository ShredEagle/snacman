#include "LoadAnimation.h"

#include "LoadBuffer.h"

namespace ad {
namespace snac {


using namespace ad::arte;


namespace {

    Node::Index addGltfNode(Const_Owned<gltf::Node> aGltfNode,
                            Node::Index aParentIndex,
                            NodeTree<gltf::Node::Matrix> & aOutTree,
                            std::vector<Node::Index> & aOutMapping)
    {
        Node::Index nodeIdx = aOutTree.addNode(aParentIndex,
                                               gltf::getTransformationAsMatrix(aGltfNode));
        aOutTree.mNodeNames[nodeIdx] = aGltfNode->name;
        aOutMapping[aGltfNode.id()] = nodeIdx;

        for(Const_Owned<gltf::Node> child : aGltfNode.iterate(&gltf::Node::children))
        {
            addGltfNode(child, nodeIdx, aOutTree, aOutMapping);
        }

        return nodeIdx;
    }

} //unnamed namespace


std::pair<NodeTree<gltf::Node::Matrix>, std::vector<Node::Index>>
loadHierarchy(const Gltf & aGltf, gltf::Index<gltf::Scene> aSceneIndex)
{
    Const_Owned<gltf::Scene> scene = aGltf.get(aSceneIndex);

    auto nodeCount = aGltf.countNodes();
    NodeTree<gltf::Node::Matrix> tree;
    tree.reserve(nodeCount);
    std::vector<Node::Index> gltfIndexToTreeIndex(nodeCount, Node::gInvalidIndex);

    auto roots = scene.iterate(&gltf::Scene::nodes);
    assert(roots.size() >= 1);
    auto rootIt = roots.begin();
    tree.mFirstRoot = addGltfNode(*rootIt++, Node::gInvalidIndex, tree, gltfIndexToTreeIndex);
    for(; rootIt != roots.end(); ++rootIt)
    {
        addGltfNode(*rootIt, Node::gInvalidIndex, tree, gltfIndexToTreeIndex);
    }

    return {tree, gltfIndexToTreeIndex};
}


namespace {

    // Internal data structure used while loading the gltf animation
    struct Anim
    {
        using AccessorIndex_t = arte::gltf::Index<arte::gltf::Accessor>;
        static constexpr AccessorIndex_t gInvalidIndex = std::numeric_limits<AccessorIndex_t::Value_t>::max();
        using Path = arte::gltf::animation::Target::Path;

        Anim(arte::gltf::Index<arte::gltf::Node> aTargetNode) :
            mGltfNode{aTargetNode}
        {}

        /// @return The index of the input (time) accessor.
        AccessorIndex_t
        add(Path aPath, arte::Const_Owned<arte::gltf::animation::Sampler> aSampler)
        {
            // Only support linear interpolation at the moment.
            assert(aSampler->interpolation == arte::gltf::animation::Sampler::Interpolation::Linear);

            auto write = [](AccessorIndex_t & aTarget, AccessorIndex_t aValue)
            {
                if(aTarget != gInvalidIndex)
                {
                    throw std::logic_error{"Duplicated node path in an animation."};
                }
                else
                {
                    aTarget = aValue;
                }
            };

            switch(aPath)
            {
                case Path::Translation:
                    write(translationAccessor, aSampler->output);
                    break;
                case Path::Rotation:
                    write(rotationAccessor, aSampler->output);
                    break;
                case Path::Scale:
                    write(scaleAccessor, aSampler->output);
                    break;
                default:
                    throw std::logic_error{"Unsupported animation target path."};
            }

            return aSampler->input;
        }

        bool isComplete() const
        {
            return translationAccessor != gInvalidIndex
                && rotationAccessor != gInvalidIndex
                && scaleAccessor != gInvalidIndex;
        }

        // Only used if there is no sampler for a path:
        // The values for the paths should then copied from the gltf node initial pose.
        arte::gltf::Index<arte::gltf::Node> mGltfNode;

        AccessorIndex_t translationAccessor = gInvalidIndex;
        AccessorIndex_t rotationAccessor = gInvalidIndex;
        AccessorIndex_t scaleAccessor = gInvalidIndex;
    };


    void assertAccessor(arte::Const_Owned<arte::gltf::Accessor> aAccessor, 
                        arte::gltf::Accessor::ElementType aElementType,
                        arte::gltf::EnumType aComponentType) 
    {
        assert(aAccessor->type == aElementType);
        assert(aAccessor->componentType == aComponentType);
    }


    NodeAnimation readAnimation(arte::Const_Owned<arte::gltf::Animation> aGltfAnimation,
                                const std::vector<Node::Index> & aGltfToTreeIndex,
                                // Note: For the moment, we implicitly couple animations with a specific rig.
                                // Ideally, this coupling could be alleviated with better data models on our side.
                                const Rig & aRigSanityCheck)
    {
        std::unordered_map<Node::Index, Anim> perNode;

        Anim::AccessorIndex_t inputAccessorIdx = Anim::gInvalidIndex;

        auto samplers = aGltfAnimation.iterate(&arte::gltf::Animation::samplers);
        for (const arte::gltf::animation::Channel & channel : aGltfAnimation->channels)
        {
            // Extension could have animation target without node, but we do not support it.
            assert(channel.target.node.has_value());

            auto gltfNodeIndex = *channel.target.node;
            // The mapping should contain an entry for this node
            assert(aGltfToTreeIndex.size() > gltfNodeIndex);

            if(aGltfToTreeIndex[gltfNodeIndex] == Node::gInvalidIndex)
            {
                throw std::logic_error{"Animation targets a node which is not part of the loaded hierarchy."};
            }

            // TODO #anim This coupling from Rig to Animation is not necessarily a good thing 
            // (see note on the parameter)
            if(std::find(aRigSanityCheck.mJoints.begin(),
                         aRigSanityCheck.mJoints.end(),
                         aGltfToTreeIndex[gltfNodeIndex]) 
               == aRigSanityCheck.mJoints.end())
            {
                throw std::logic_error{"Animation targets a node which is not a joint in the rig."};
            }
            
            // Get the existing Anim for this nodetree index, or insert an Anim if its is not present.
            auto [iterator, didInsert] = perNode.try_emplace(
                aGltfToTreeIndex[gltfNodeIndex], // Key
                gltfNodeIndex // gltf node index to construct the Anim if needed
            );
            auto samplerInput = iterator->second.add(channel.target.path, samplers.at(channel.sampler));

            if(inputAccessorIdx != Anim::gInvalidIndex && inputAccessorIdx != samplerInput)
            {
                throw std::logic_error{"Expecting all channels of a given animation to have the same input."};
            }
            else
            {
                inputAccessorIdx = samplerInput;
            }
        }

        NodeAnimation animation;

        // Load the vector of timepoints from the input accessor.
        auto inputAccessor = aGltfAnimation.get(inputAccessorIdx);
        assertAccessor(inputAccessor, arte::gltf::Accessor::ElementType::Scalar, GL_FLOAT);
        animation.mTimepoints = loadTypedData<float>(inputAccessor);
        animation.mEndTime = std::get<arte::gltf::Accessor::MinMax<float>>(*inputAccessor->bounds).max[0];

        // Load the vectors of keyframe poses from the accessors stored in Anim instances.
        for(const auto & [nodeIndex, anim] : perNode)
        {
            // For each path (rotation, translation, scale), read the keyframe data if the path is animated
            // or make N copies of the node's initial value for the path if it is not animated.
            // TODO #anim Can we not make N copies of the constant channels?

            NodeAnimation::NodeKeyframes nodeKeyframes;
            auto gltfNode = aGltfAnimation.get(anim.mGltfNode);

            if (anim.translationAccessor != Node::gInvalidIndex)
            {
                auto translationAccessor = aGltfAnimation.get(anim.translationAccessor);
                assertAccessor(translationAccessor, arte::gltf::Accessor::ElementType::Vec3, GL_FLOAT);
                nodeKeyframes.mTranslations = loadTypedData<math::Vec<3, float>>(translationAccessor);
            }
            else
            {
                std::fill_n(std::back_inserter(nodeKeyframes.mTranslations),
                            animation.mTimepoints.size(),
                            getTransformationAsTRS(gltfNode).translation);
            }

            if (anim.rotationAccessor != Node::gInvalidIndex)
            {
                auto rotationAccessor = aGltfAnimation.get(anim.rotationAccessor);
                // TODO Rotation might be stored as normalized integral values, but not supported yet.
                assertAccessor(rotationAccessor, arte::gltf::Accessor::ElementType::Vec4, GL_FLOAT);
                nodeKeyframes.mRotations = loadTypedData<math::Quaternion<float>>(rotationAccessor);
            }
            else
            {
                std::fill_n(std::back_inserter(nodeKeyframes.mRotations),
                            animation.mTimepoints.size(),
                            getTransformationAsTRS(gltfNode).rotation);
            }

            if (anim.scaleAccessor != Node::gInvalidIndex)
            {
                auto scaleAccessor = aGltfAnimation.get(anim.scaleAccessor);
                assertAccessor(scaleAccessor, arte::gltf::Accessor::ElementType::Vec3, GL_FLOAT);
                nodeKeyframes.mScales = loadTypedData<math::Vec<3, float>>(scaleAccessor);
            }
            else
            {
                std::fill_n(std::back_inserter(nodeKeyframes.mScales),
                            animation.mTimepoints.size(),
                            getTransformationAsTRS(gltfNode).scale);
            }

            // Sanity check: all paths have the same number of keyframes, matching the timepoints.
            assert(nodeKeyframes.mTranslations.size() == animation.mTimepoints.size()
                && nodeKeyframes.mRotations.size() == animation.mTimepoints.size()
                && nodeKeyframes.mScales.size() == animation.mTimepoints.size());

            animation.mNodes.push_back(nodeIndex);
            animation.mKeyframesEachNode.push_back(std::move(nodeKeyframes));
        }

        return animation;
    }


    std::unordered_map<std::string, NodeAnimation>
    loadAnimations(const arte::Gltf & aGltf,
                   const std::vector<Node::Index> & aGltfToTreeIndex,
                   const Rig & aRigSanityCheck)
    {
        std::unordered_map<std::string, NodeAnimation> animations;

        for(arte::Const_Owned<arte::gltf::Animation> animation : aGltf.getAnimations())
        {
            std::string name = animation->name;
            name = (name.empty() ? "anim_#" + animation.id() : name);
            animations.emplace(name, readAnimation(animation, aGltfToTreeIndex, aRigSanityCheck));
        }

        return animations;
    }


    Rig loadSkin(arte::Const_Owned<arte::gltf::Skin> skin,
                 const std::vector<Node::Index> & aGltfToTreeIndex)
    {
        Rig result{
            .mName = skin->name,
        };

        for (auto gltfNodeIdx : skin->joints)
        {
            result.mJoints.push_back(aGltfToTreeIndex[gltfNodeIdx]);
        }

        // Only support skin with inverse bind matrices
        assert(skin->inverseBindMatrices);
        auto ibmAccessor = skin.get(*skin->inverseBindMatrices);
        assertAccessor(ibmAccessor, arte::gltf::Accessor::ElementType::Mat4, GL_FLOAT);
        result.mInverseBindMatrices = loadTypedData<math::AffineMatrix<4, float>>(ibmAccessor);

        return result;
    }

} // unnamed namespace


std::pair<Rig, std::unordered_map<std::string, NodeAnimation>>
loadSkeletalAnimation(const Gltf & aGltf,
                      gltf::Index<gltf::Skin> aSkinIndex,
                      gltf::Index<gltf::Scene> aSceneIndex)
{
    Rig rig;
    std::unordered_map<std::string, NodeAnimation> animations;

    std::vector<Node::Index> indexMapping; // gltf node index to NodeTree index
    NodeTree<gltf::Node::Matrix> scene;

    // TODO #anim should only load the joints instead of the whole scene.
    std::tie(scene, indexMapping) = loadHierarchy(aGltf, 0/*scene idx*/);

    rig = loadSkin(aGltf.get(aSkinIndex), indexMapping);
    rig.mScene = std::move(scene);

    animations = loadAnimations(aGltf, indexMapping, rig);

    return {std::move(rig), std::move(animations)};
}


} // namespace snac
} // namespace ad
