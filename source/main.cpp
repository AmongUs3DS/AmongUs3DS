#include <includes.hpp>
#include <malloc.h>
#include <game/scenes/SplashScene.hpp>
#include <game/scenes/AssetHelperScene.hpp>
#include <sys/stat.h>

bool checkAssetsExist() {
    struct stat st;
    if (stat("sdmc:/3ds/AmongUs3DS/assets", &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    romfsInit();

    if (!checkAssetsExist()) {
        C2D_Prepare();

        auto& director = engine::Director::getInstance();
        director.setClearColor(0, 0, 0, 255);

        auto helperTop = game::AssetHelperSceneTop::create();
        auto helperBottom = game::AssetHelperSceneBottom::create();

        director.runWithScenes(helperTop, helperBottom);
        director.mainLoop();

        romfsExit();
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 0;
    }

    engine::NetworkManager::getInstance().init();
    engine::AudioEngine::getInstance().init();

    C2D_Prepare();

    auto& director = engine::Director::getInstance();

    director.setClearColor(0, 0, 0, 255);

    auto topScene = game::SplashSceneTop::create();
    auto bottomScene = game::SplashSceneBottom::create();

    director.runWithScenes(topScene, bottomScene);

    director.mainLoop();

    engine::AudioEngine::getInstance().shutdown();
    engine::NetworkManager::getInstance().shutdown();

    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    
    return 0;
}
