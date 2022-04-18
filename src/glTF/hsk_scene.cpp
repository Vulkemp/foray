#include "hsk_scene.hpp"

namespace hsk{
    void Scene::AssertSceneloaded(bool loaded)
    {
        bool isLoaded = false;
        if (loaded != isLoaded){
            throw Exception("Assertion failed: Call executed expecting scene to be {}, but scene was {}!", (loaded ? "loaded" : "unloaded"), (isLoaded ? "loaded" : "unloaded"));
        }
    }
}