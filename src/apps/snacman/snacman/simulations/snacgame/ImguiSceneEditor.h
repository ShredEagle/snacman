#pragma once


#include <entity/Entity.h>


namespace ad {
namespace snacgame {


class SceneEditor
{
public:
    void showEditor(ent::EntityManager & aManager);

private:
    void presentNode(ent::Handle<ent::Entity> aEntityHandle, const char * aName);
    void presentComponents(ent::Handle<ent::Entity> aHandle);

    ent::Handle<ent::Entity> mSelected;
};


} // namespace snacgame
} // namespace ad
