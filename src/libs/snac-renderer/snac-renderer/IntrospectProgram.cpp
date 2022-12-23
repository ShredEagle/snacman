#include "IntrospectProgram.h"

#include <array>
#include <iterator>
#include <string>

#include <cassert>


namespace ad {
namespace snac {


namespace {


    template <GLenum ... VN_properties>
    class InterfaceIterator
    {
        static constexpr std::size_t gPropertiesCount = sizeof...(VN_properties);

    public:
        using difference_type = GLint;

        struct value_type
        {
            GLint operator[](std::size_t aPropertyIndex) const
            {
                assert(aPropertyIndex < gPropertiesCount);
                return mProperties[aPropertyIndex];
            }

            std::string mName;
            GLuint mIndex;
            std::array<GLint, sizeof...(VN_properties) + 1 /*name length*/> mProperties;
        };

        using pointer = const value_type *;
        using reference = const value_type &;

        InterfaceIterator(const graphics::Program & aProgram, GLenum aProgramInterface):
            mProgram{&aProgram},
            mProgramInterface{aProgramInterface}
        {
            // TODO maybe could be bidirectional,
            // depending on the requireded validity for returned reference.
            // (the reference changes target on each call to operator*() after ++/--,
            //  and the reference becomes dangling when the iterator is destroyed.)
            static_assert(std::input_iterator<InterfaceIterator>);
        }

        static InterfaceIterator makeEnd(const graphics::Program & aProgram, GLenum aProgramInterface)
        {
            InterfaceIterator end(aProgram, aProgramInterface);
            glGetProgramInterfaceiv(*end.mProgram, end.mProgramInterface, 
                                    GL_ACTIVE_RESOURCES, &end.mCurrentId);
            return end;
        }

        // Prefix increment
        InterfaceIterator & operator++()
        {
            ++mCurrentId;
            return *this;
        }

        // Postfix increment
        InterfaceIterator operator++(int)
        {
            InterfaceIterator old = *this;
            operator++();
            return old;
        }

        bool operator!=(const InterfaceIterator & aRhs) const
        {
            assert(mProgram == aRhs.mProgram);
            assert(mProgramInterface == aRhs.mProgramInterface);
            return mCurrentId != aRhs.mCurrentId;
        }

        reference operator*() const
        {
            const GLenum properties[] = {
                VN_properties...,
                GL_NAME_LENGTH, // put last, because we respect the order of VN_properties
            };
            GLint length;

            glGetProgramResourceiv(*mProgram, mProgramInterface, mCurrentId,
                                   (GLsizei)std::size(properties), properties,
                                   (GLsizei)std::size(mValue.mProperties), &length, mValue.mProperties.data());
            assert(length == std::size(properties)); // Sanity check

            // The returned length makes provision for a terminating null character.
            // this character will be written to the buffer by glGetProgramResourceName()
            GLint & nameLength = mValue.mProperties[gPropertiesCount];
            auto nameBuffer = std::make_unique<char[]>(nameLength);
            // This will write the length **without** terminating null character.
            glGetProgramResourceName(*mProgram, mProgramInterface, mCurrentId,
                                     nameLength, &nameLength, nameBuffer.get());
            mValue.mName = std::string(nameBuffer.get(), nameLength);
            mValue.mIndex = (GLuint)mCurrentId;

            // TODO This raises the question of how long should the reference be valid by standard?
            return mValue;
        }

        pointer operator->()
        {
            // TODO this is a pessimisation, as it might dereference for each member access.
            return &(this->operator*());
        }

    private:
        const graphics::Program * mProgram;
        GLenum mProgramInterface;
        GLint mCurrentId{0};
        // It seems we must store the value, because operator() has to return true references.
        // see: https://stackoverflow.com/a/52856349/1027706
        mutable value_type mValue; // needs to be written to by operator*() const.
    };


} // anonymous


std::ostream & operator<<(std::ostream & aOut, const IntrospectProgram & aProgram)
{
    auto lister = [&aOut](std::string_view aCategory, const auto & aCollection)
    {
        aOut << "\n* " << aCollection.size() << " " << aCategory 
             << (aCollection.size() > 1 ? "s" : "") << ":";
        for (const auto & resource : aCollection)
        {
            aOut << "\n\t*" << resource;
        }
    };

    aOut << "Program '" << aProgram.mName << "' with:";
    lister("attribute", aProgram.mAttributes);
    lister("uniform", aProgram.mUniforms);
    lister("uniform block", aProgram.mUniformBlocks);
    return aOut;
}


std::ostream & operator<<(std::ostream & aOut, const IntrospectProgram::Resource & aResource)
{
    aOut << aResource.mName 
         << " (" << to_string(aResource.mSemantic) << ")"
         ;
    
    if (aResource.mLocation != -1)
    {
        aOut << " at location " << aResource.mLocation;
    }

    return aOut << " has type " << graphics::to_string(getResourceComponentType(aResource.mType))
                << " and dimension " << getResourceDimension(aResource.mType)
                << "."
                ;
}


std::ostream & operator<<(std::ostream & aOut, const IntrospectProgram::UniformBlock & aBlock)
{
    aOut << aBlock.mName 
         // TODO add block semantic output
         //<< " (" << to_string(aResource.mSemantic) << ")"
         << " at binding index " << aBlock.mBindingIndex
         << " has " << aBlock.mUniforms.size() << " active uniform variable(s):"
         ;

    for (const auto & resource : aBlock.mUniforms)
    {
        // TODO Address the dirty encoding of two tabs
        // (because we currently intend to use it under the IntrospectProgram output operator)
        aOut << "\n\t\t*" << resource;
    }

    return aOut;
}


IntrospectProgram::IntrospectProgram(graphics::Program aProgram, std::string aName) :
    mProgram{std::move(aProgram)},
    mName{std::move(aName)}
{
    auto makeResource = [](const auto &aAttribute) -> Resource
    {
        return Resource{
            .mLocation = aAttribute[0],
            .mType = (GLenum)aAttribute[1],
            .mSemantic = to_semantic(aAttribute.mName),
            .mName = aAttribute.mName,
        };
    };

    // Attributes
    {
        using Iterator = InterfaceIterator<GL_LOCATION, GL_TYPE>;
        for (auto it = Iterator(mProgram, GL_PROGRAM_INPUT);
            it != Iterator::makeEnd(mProgram, GL_PROGRAM_INPUT);
            ++it)
        {
            mAttributes.push_back(Attribute{makeResource(*it)});
        }
    }

    // Uniform blocks
    {
        using Iterator = InterfaceIterator<GL_BUFFER_BINDING>;
        for (auto it = Iterator(mProgram, GL_UNIFORM_BLOCK);
            it != Iterator::makeEnd(mProgram, GL_UNIFORM_BLOCK);
            ++it)
        {
            mUniformBlocks.push_back(UniformBlock{
                .mBlockIndex = it->mIndex,
                .mBindingIndex = (GLuint)(*it)[0],
                .mName = it->mName,
            });
        }
    }

    // All uniforms (from uniform blocks, and not)
    {
        using Iterator = InterfaceIterator<GL_LOCATION, GL_TYPE, GL_BLOCK_INDEX>;
        for (auto it = Iterator(mProgram, GL_UNIFORM);
            it != Iterator::makeEnd(mProgram, GL_UNIFORM);
            ++it)
        {
            GLint blockIndex = (*it)[2];
            GLint location = (*it)[0];
            if ((*it)[2] == -1) // BLOCK_INDEX == -1 means not part of an interface blocks
            {
                assert(location != -1);
                mUniforms.push_back(Attribute{makeResource(*it)});
            }
            // Note: Instead of re-iterating all uniforms of a block by uniform id
            //       (obtained via GL_ACTIVE_VARIABLES), match-up on block index.
            else
            {
                assert(location == -1);
                auto found = std::find_if(mUniformBlocks.begin(), mUniformBlocks.end(),
                                          [blockIndex](const auto & block)
                                          {
                                            return block.mBlockIndex == (GLuint)blockIndex;
                                          });
                if (found == mUniformBlocks.end())
                {
                    // This is a thing that should not be
                    throw std::logic_error("Uniform found with non-existent block index.");
                }
                else
                {
                    found->mUniforms.push_back(makeResource(*it));
                }
            }
        }
    }
}


graphics::ShaderParameter IntrospectProgram::Attribute::toShaderParameter() const
{
    graphics::ShaderParameter result{
        (GLuint)mLocation,
        getResourceShaderAccess(mType)
    };
    result.mNormalize = isNormalized(mSemantic);
    return result;
}


} // namespace snac
} // namespace ad
