#include "IntrospectProgram.h"

#include <array>
#include <iterator>
#include <string>

#include <cassert>


namespace ad {
namespace snac {


graphics::ShaderParameter::Access getShaderAccess(GLenum aType)
{
    switch(aType)
    {
        case GL_FLOAT:
        case GL_FLOAT_VEC2:
        case GL_FLOAT_VEC3:
        case GL_FLOAT_VEC4:
        case GL_FLOAT_MAT2:
        case GL_FLOAT_MAT3:
        case GL_FLOAT_MAT4:
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT2x4:
        case GL_FLOAT_MAT3x2:
        case GL_FLOAT_MAT3x4:
        case GL_FLOAT_MAT4x2:
        case GL_FLOAT_MAT4x3:
        {
            return graphics::ShaderParameter::Access::Float;
        }

        case GL_DOUBLE:
        case GL_DOUBLE_VEC2:
        case GL_DOUBLE_VEC3:
        case GL_DOUBLE_VEC4:
        case GL_DOUBLE_MAT2:
        case GL_DOUBLE_MAT3:
        case GL_DOUBLE_MAT4:
        case GL_DOUBLE_MAT2x3:
        case GL_DOUBLE_MAT2x4:
        case GL_DOUBLE_MAT3x2:
        case GL_DOUBLE_MAT3x4:
        case GL_DOUBLE_MAT4x2:
        case GL_DOUBLE_MAT4x3:
        {
            throw std::logic_error{
                std::string{__func__} + ": Double parameters are not handled at the moment."};
        }

        case GL_INT:
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:

        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_VEC2:
        case GL_UNSIGNED_INT_VEC3:
        case GL_UNSIGNED_INT_VEC4:
        {
            return graphics::ShaderParameter::Access::Integer;
        }

        default:
        {
            throw std::logic_error{
                std::string{__func__} + ": Unhandled enumerator with value: " 
                + std::to_string(aType) + "."};
        }
    }
}


graphics::AttributeDimension getDimension(GLenum aType)
{
    switch(aType)
    {
        case GL_FLOAT:
        case GL_DOUBLE:
        case GL_INT:
        case GL_UNSIGNED_INT:
            return {1, 1};

        case GL_FLOAT_VEC2:
        case GL_DOUBLE_VEC2:
        case GL_INT_VEC2:
        case GL_UNSIGNED_INT_VEC2:
            return {2, 1};

        case GL_FLOAT_VEC3:
        case GL_DOUBLE_VEC3:
        case GL_INT_VEC3:
        case GL_UNSIGNED_INT_VEC3:
            return {3, 1};

        case GL_FLOAT_VEC4:
        case GL_DOUBLE_VEC4:
        case GL_INT_VEC4:
        case GL_UNSIGNED_INT_VEC4:
            return {4, 1};

        case GL_FLOAT_MAT2:
        case GL_DOUBLE_MAT2:
            return {2, 2};

        case GL_FLOAT_MAT3:
        case GL_DOUBLE_MAT3:
            return {3, 3};

        case GL_FLOAT_MAT4:
        case GL_DOUBLE_MAT4:
            return {4, 4};

        // In GL, matNxM has N column and M rows.
        // Following our convention (row major), 
        // the number of colums (i.e. the number of elements in a row) is the first dimension.
        case GL_FLOAT_MAT2x3:
        case GL_DOUBLE_MAT2x3:
            return {2, 3};

        case GL_FLOAT_MAT2x4:
        case GL_DOUBLE_MAT2x4:
            return {2, 4};

        case GL_FLOAT_MAT3x2:
        case GL_DOUBLE_MAT3x2:
            return {3, 2};

        case GL_FLOAT_MAT3x4:
        case GL_DOUBLE_MAT3x4:
            return {3, 4};

        case GL_FLOAT_MAT4x2:
        case GL_DOUBLE_MAT4x2:
            return {4, 2};

        case GL_FLOAT_MAT4x3:
            return {4, 3};

        default:
        {
            throw std::logic_error{
                std::string{__func__} + ": Unhandled enumerator with value: " 
                + std::to_string(aType) + "."};
        }
    }
}


namespace {


    template <GLenum ... VN_properties>
    class InterfaceIterator
    {
        static constexpr std::size_t gPropertiesCount = sizeof...(VN_properties);

    public:
        struct value_type
        {
            GLint operator[](std::size_t aPropertyIndex) const
            {
                assert(aPropertyIndex < gPropertiesCount);
                return mProperties[aPropertyIndex];
            }

            std::string mName;
            std::array<GLint, sizeof...(VN_properties) + 1 /*name length*/> mProperties;
        };

        InterfaceIterator(const graphics::Program & aProgram, GLenum aProgramInterface):
            mProgram{&aProgram},
            mProgramInterface{aProgramInterface}
        {}

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

        value_type & operator*()
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

            // TODO This raises the question of how long should the reference be valid by standard?
            return mValue;
        }

        value_type * operator->()
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
        value_type mValue;
    };


    // TODO can be bidirectional, depending on the requireded validity for returned reference.
    //static_assert(std::input_iterator<InterfaceIterator>);


} // anonymous


IntrospectProgram::IntrospectProgram(graphics::Program aProgram, std::string aName) :
    mProgram{std::move(aProgram)},
    mName{std::move(aName)}
{
    using Iterator = InterfaceIterator<GL_LOCATION, GL_TYPE>;
    for (auto it = Iterator(mProgram, GL_PROGRAM_INPUT);
         it != Iterator::makeEnd(mProgram, GL_PROGRAM_INPUT);
         ++it)
    {
        auto & attribute = *it;
        mAttributes.push_back({
            .mLocation = (GLuint)attribute[0],
            .mType = (GLenum)attribute[1],
            .mSemantic = to_semantic(attribute.mName),
            .mName = attribute.mName,
        });
    }
}


graphics::ShaderParameter IntrospectProgram::Attribute::toShaderParameter() const
{
    graphics::ShaderParameter result{
        mLocation,
        getShaderAccess(mType)
    };
    result.mNormalize = isNormalized(mSemantic);
    return result;
}


} // namespace snac
} // namespace ad
