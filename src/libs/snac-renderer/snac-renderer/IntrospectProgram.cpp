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


IntrospectProgram::IntrospectProgram(graphics::Program aProgram) :
    mProgram{std::move(aProgram)}
{
    using Iterator = InterfaceIterator<GL_LOCATION, GL_TYPE>;
    for (auto it = Iterator(mProgram, GL_PROGRAM_INPUT);
         it != Iterator::makeEnd(mProgram, GL_PROGRAM_INPUT);
         ++it)
    {
        auto & attribute = *it;
        mAttributes.push_back({
            .mLocation = attribute[0],
            .mType = (GLenum)attribute[1],
            .mSemantic = to_semantic(attribute.mName),
            .mName = attribute.mName,
        });
    }
}


} // namespace snac
} // namespace ad
