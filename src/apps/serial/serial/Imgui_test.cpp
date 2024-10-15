#include "Comp.h"
#include "entity/Entity.h"
#include "entity/EntityManager.h"
#include "graphics/ApplicationGlfw.h"
#include "imguiui/ImguiUi.h"
#include "serial/serialization/Witness.h"

#include <chrono>
#include <imgui.h>
#include <thread>

using Clock = std::chrono::high_resolution_clock;

inline void sleepBusy(Clock::duration aDuration,
                      Clock::time_point aSleepFrom = Clock::now())
{
    while ((aSleepFrom + aDuration) > Clock::now())
    {
        std::this_thread::sleep_for(Clock::duration{0});
    }
}

void testImgui()
{
    ad::graphics::ApplicationFlag glfwFlags =
        ad::graphics::ApplicationFlag::None;

    ad::graphics::ApplicationGlfw glfwApp{
        "0",       1920, 1024,
        glfwFlags, // TODO, handle applicationFlags via settings
        4,         1,    {{GLFW_SAMPLES, 4}},
    };

    ad::imguiui::ImguiUi ui(glfwApp);

    ad::ent::EntityManager world{};
    ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
    ad::ent::Handle<ad::ent::Entity> other = world.addEntity("other");
    {
        ad::ent::Phase phase;
        handle.get(phase)->add(CompA{1, true, "hello", 14121.3f});
        other.get(phase)->add(CompB{{0, 1, 2, 3, 4, 5, 6, 7},
                                    {3, 4, 5},
                                    {{"first", 1}, {"second", 2}},
                                    {{"third", 3}, {"fourth", 4}}});
        other.get(phase)->add(
            CompSnacTest{{{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}},
                         ControllerType::Keyboard,
                         GameInput{1, {0.f, 0.f}, {0.f, 1.f}},
                         std::chrono::high_resolution_clock::time_point{std::chrono::high_resolution_clock::now()}});
    }
    // ad::ent::Handle<ad::ent::Entity> otherHandle = world.addEntity("other");
    // otherHandle.get(phase)->add(CompB{{0,1,2}, {3,4,5}, {{"first", 1},
    // {"second", 2}}, {{"third", 3}, {"fourth", 4}}});

    while (glfwApp.handleEvents())
    {
        glfwApp.getAppInterface()->clear();
        ui.newFrame();
        ImGui::Begin("hello");
        witness_imgui(world, Witness::make(&ui));
        ImGui::End();
        ImGui::ShowDemoWindow();
        ui.render();
        ui.renderBackend();
        glfwApp.swapBuffers();
    }
}
