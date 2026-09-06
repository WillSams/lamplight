#pragma once
#include <SDL2/SDL.h>

#include <stormengine2/input/gamepad.h>
#include <stormengine2/lighting.h>
#include <stormengine2/states/gameState.h>

#include <memory>
#include <optional>
#include <string>

#include "../arena/camera.h"
#include "../dungeon/generate.h"
#include "../input/input.h"
#include "../net/hostSession.h"
#include "../net/joinSession.h"

namespace storm {

enum class NetMode { Solo, Host, Join };

class PlayState : public GameState {
public:
    PlayState(SDL_Renderer *renderer, int w, int h, bool dbg,
              AssetStore_Ptr assets, bool &running, NetMode mode = NetMode::Solo,
              const std::string &ip = "", std::uint16_t port = 5000,
              std::uint32_t seed = 12345);
    ~PlayState();

    void processInput() override;
    void update() override;
    void render() override;
    bool onEnter() override;
    bool onExit() override;
    std::string getStateID() const override { return s_playID; }

private:
    void LoadAssets();
    void SpawnActors();
    void DrawSprites();
    MoveInput ReadInputs();

    static const std::string s_playID;

    SDL_Renderer *renderer_;
    int windowWidth_, windowHeight_;
    bool isDebugging_;
    AssetStore_Ptr assets_;
    Logger logger_;
    bool &isRunning_;
    Registry registry_;

    std::optional<Entity> player_, remote_;

    NetMode mode_;
    std::string ip_;
    std::uint16_t port_;
    std::uint32_t seed_;

    std::unique_ptr<HostSession> host_;
    std::unique_ptr<JoinSession> join_;
    Dungeon dungeon_;
    WorldState world_;  // solo mode: no sessions, so local world

    float px_ = 96, py_ = 96;
    glm::vec2 facing_{1, 0};
    bool faceLeft_ = false;
    float meleeCd_ = 0, shootCd_ = 0;

    bool keys_[4] = {};
    bool swingQueued_ = false, shootQueued_ = false, dropQueued_ = false;

    Gamepad pad_;
    CameraRect camera_;
    SDL_Rect cameraSdl_{0, 0, 0, 0};
    LightingOverlay lamp_;
    glm::vec2 lampBuildCentre_{0, 0};
    bool lampBuilt_ = false;

    int millisecondsPreviousFrame_ = 0;
};

} // namespace storm
