#pragma once


#include <entity/Entity.h>


namespace ad {
namespace snacgame {


class SceneEditor
{
public:
    void showEditor(ent::Handle<ent::Entity> aSceneRoot);

private:
    void presentNode(ent::Handle<ent::Entity> aEntityHandle);

    ent::Handle<ent::Entity> mSelected;
};


} // namespace snacgame
} // namespace ad
