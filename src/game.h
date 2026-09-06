#pragma once

#include <SDL2/SDL.h>

#include <stormengine2/assetStore.h>
#include <stormengine2/gameStateMachine.h>
#include <stormengine2/logger.h>

#include "states/playState.h"

using namespace storm;

// The engine ships no Game class, no main loop and no window management --
// only GameStateMachine. Every game writes this file. It is ~50 lines and is
// near-identical across games, which is exactly why it belongs in a scaffold.
class Game {
public:
    Game(NetMode mode = NetMode::Solo, std::string ip = "127.0.0.1",
         uint16_t port = 5000, uint32_t seed = 12345);
    ~Game();

    void Initialize();
    void ProcessInput();
    void Update();
    void Render();
    void Run();
    void Destroy();

private:
    bool isRunning   = false;
    bool isDebugging = false;

    SDL_Window   *window   = nullptr;
    SDL_Renderer *renderer = nullptr;

    GameStateMachine gameStateMachine;
    Logger_Ptr       logger;
    AssetStore_Ptr   assetStore;

    int windowWidth  = 800;
    int windowHeight = 600;

    NetMode netMode_ = NetMode::Solo;
    std::string netIp_ = "127.0.0.1";
    uint16_t netPort_ = 5000;
    uint32_t netSeed_ = 12345;
};
