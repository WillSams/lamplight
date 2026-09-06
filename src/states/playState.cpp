#include "playState.h"

#include "../arena/collision.h"
#include "../dungeon/dungeonDraw.h"
#include "../game/world.h"
#include "../render/sheet.h"

namespace storm {

const std::string PlayState::s_playID = "PLAY_STATE";

static constexpr int kScale = 3;
static constexpr float kPlayerSpeed = 180.0f;

PlayState::PlayState(SDL_Renderer *r, int w, int h, bool dbg, AssetStore_Ptr a,
                     bool &run, NetMode m, const std::string &ip, std::uint16_t p,
                     std::uint32_t s)
    : renderer_(r), windowWidth_(w), windowHeight_(h), isDebugging_(dbg),
      assets_(std::move(a)), isRunning_(run), mode_(m), ip_(ip), port_(p), seed_(s) {}

PlayState::~PlayState() {}

bool PlayState::onEnter() {
    LoadAssets();
    registry_.AddSystem<MovementSystem>();
    registry_.AddSystem<RenderSystem>();
    if (mode_ == NetMode::Host) {
        host_ = std::make_unique<HostSession>();
        if (!host_->Start(port_, seed_)) logger_.Err("host listen failed");
        dungeon_ = host_->dungeon;
        px_ = host_->world.hostX;
        py_ = host_->world.hostY;
    } else if (mode_ == NetMode::Join) {
        join_ = std::make_unique<JoinSession>();
        if (!join_->Connect(ip_, port_)) logger_.Err("join connect failed");
    } else {
        dungeon_ = Generate(seed_);
        world_ = SpawnWorld(dungeon_);
        px_ = world_.hostX;
        py_ = world_.hostY;
    }
    SpawnActors();
    LightingOverlay::Params lp;
    lp.width = windowWidth_;
    lp.height = windowHeight_;
    lp.radius = 220;
    lamp_.Build(renderer_, lp);
    pad_.OpenFirstAttached();
    millisecondsPreviousFrame_ = SDL_GetTicks();
    return true;
}

bool PlayState::onExit() {
    pad_.Shutdown();
    lamp_.Release();
    if (assets_) assets_->ClearAssets();  // ClearAssets before SDL teardown
    return true;
}

void PlayState::LoadAssets() {
    assets_->AddTexture(renderer_, "sheet", "./assets/gfx/tilemap_packed.png");
    if (!assets_->GetTexture("sheet"))
        logger_.Err("Missing ./assets/gfx/tilemap_packed.png -- run from the game root.");
}

void PlayState::SpawnActors() {
    // Full component set at admission: membership is computed once, at flush.
    Entity p = registry_.CreateEntity();
    p.Tag("player");
    p.AddComponent<TransformComponent>(glm::vec2(px_, py_), glm::vec2(3.0f, 3.0f), 0.0);
    p.AddComponent<RigidBodyComponent>(glm::vec2(0, 0));
    // Roles are fixed by the net mode, not by which window you're in: the
    // host player is always hero 1 (2,8), the joiner always hero 2 (4,8).
    const bool hostSide = mode_ != NetMode::Join;
    TileRect t = TileAt(hostSide ? 2 : 4, 8);
    p.AddComponent<SpriteComponent>("sheet", 16, 16, 2, false, t.x, t.y);
    p.AddComponent<CircleColliderComponent>(6, glm::vec2(8, 8));
    player_ = p;

    Entity q = registry_.CreateEntity();
    q.Tag("remote");
    q.AddComponent<TransformComponent>(glm::vec2(px_ + 48, py_), glm::vec2(3.0f, 3.0f), 0.0);
    q.AddComponent<RigidBodyComponent>(glm::vec2(0, 0));
    TileRect t2 = TileAt(hostSide ? 4 : 2, 8);
    q.AddComponent<SpriteComponent>("sheet", 16, 16, 1, false, t2.x, t2.y);
    q.AddComponent<CircleColliderComponent>(6, glm::vec2(8, 8));
    remote_ = q;
}

MoveInput PlayState::ReadInputs() {
    MoveInput in;
    in.left = keys_[2];
    in.right = keys_[3];
    in.up = keys_[0];
    in.down = keys_[1];
    const GamepadState &g = pad_.Current();
    in.stickX = g.leftX;
    in.stickY = g.leftY;
    in.dpadLeft = pad_.Down(GamepadButton::Left);
    in.dpadRight = pad_.Down(GamepadButton::Right);
    in.dpadUp = pad_.Down(GamepadButton::Up);
    in.dpadDown = pad_.Down(GamepadButton::Down);
    // Deduplicate: d-pad folded into stick by the engine when |axis| > 0.5;
    // MoveDirection already prefers the stick.
    in.dpadLeft = in.dpadLeft && g.leftX == 0.0f;
    in.dpadRight = in.dpadRight && g.leftX == 0.0f;
    in.dpadUp = in.dpadUp && g.leftY == 0.0f;
    in.dpadDown = in.dpadDown && g.leftY == 0.0f;
    return in;
}

void PlayState::processInput() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        pad_.HandleEvent(e);
        if (e.type == SDL_QUIT) { isRunning_ = false; return; }
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool d = e.type == SDL_KEYDOWN;
            switch (e.key.keysym.sym) {
            case SDLK_w: case SDLK_UP: keys_[0] = d; break;
            case SDLK_s: case SDLK_DOWN: keys_[1] = d; break;
            case SDLK_a: case SDLK_LEFT: keys_[2] = d; break;
            case SDLK_d: case SDLK_RIGHT: keys_[3] = d; break;
            case SDLK_SPACE: if (d) swingQueued_ = true; break;
            case SDLK_f: if (d) shootQueued_ = true; break;
            case SDLK_e: if (d) dropQueued_ = true; break;
            case SDLK_ESCAPE: if (d) { isRunning_ = false; return; } break;
            default: break;
            }
        }
    }
    // Pad action edges. Update() runs in update(); polling here reads the
    // state the last frame's Update() left.
    if (pad_.Pressed(GamepadButton::A)) swingQueued_ = true;
    if (pad_.Pressed(GamepadButton::X) || pad_.Current().triggerRight > 0.5f) shootQueued_ = true;
    if (pad_.Pressed(GamepadButton::B)) dropQueued_ = true;
}

void PlayState::update() {
    int wait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecondsPreviousFrame_);
    if (wait > 0 && wait <= MILLISECS_PER_FRAME) SDL_Delay(wait);
    pad_.Update();
    double dt = (SDL_GetTicks() - millisecondsPreviousFrame_) / 1000.0;
    millisecondsPreviousFrame_ = SDL_GetTicks();
    registry_.Update();  // deferred create/destroy flushes first, always

    const MoveInput input = ReadInputs();
    const glm::vec2 dir = MoveDirection(input);
    facing_ = FacingFrom(dir, pad_.Current().rightX, pad_.Current().rightY, facing_);
    if (facing_.x < -0.2f) faceLeft_ = true;
    else if (facing_.x > 0.2f) faceLeft_ = false;

    // Grid-snap walls, one axis at a time. Never wall collider entities.
    // Box is the middle 36x36 of the 48px sprite: the current box (20x22 at
    // top-left) sat off-centre and read as "walking through the lower wall".
    const float solidTile = kTile * kScale;  // 48 world px per cell
    auto solid = [&](float wx, float wy) {
        return dungeon_.IsSolid(static_cast<int>(wx / solidTile),
                                static_cast<int>(wy / solidTile));
    };
    constexpr float kBox = 36.0f, kInset = (16 * 3 - kBox) * 0.5f;
    glm::vec2 body(px_ + kInset, py_ + kInset);
    body = ResolveGridMove(body, glm::vec2(kBox, kBox),
                           glm::vec2(dir.x * kPlayerSpeed * dt, 0.0f), solid);
    body = ResolveGridMove(body, glm::vec2(kBox, kBox),
                           glm::vec2(0.0f, dir.y * kPlayerSpeed * dt), solid);
    px_ = body.x - kInset;
    py_ = body.y - kInset;

    meleeCd_ -= static_cast<float>(dt);
    shootCd_ -= static_cast<float>(dt);

    std::uint32_t actions = 0;
    bool swing = swingQueued_; swingQueued_ = false;
    bool shoot = shootQueued_ || (pad_.Current().triggerRight > 0.5f && shootCd_ <= 0);
    shootQueued_ = false;
    bool drop = dropQueued_; dropQueued_ = false;
    if (swing) actions |= ActSwing;
    if (drop) actions |= ActDrop;

    if (mode_ == NetMode::Host && host_) {
        host_->world.hostX = px_;
        host_->world.hostY = py_;
        if (swing && meleeCd_ <= 0) { meleeCd_ = 0.4f; host_->HostSwing(px_, py_, facing_.x, facing_.y); }
        if (shoot && shootCd_ <= 0) { shootCd_ = 0.6f; host_->HostShoot(px_, py_, facing_.x, facing_.y); }
        if (drop) {
            if (!TryPickUpLamp(host_->world, 1)) {
                int chest = ChestInReach(host_->world, px_, py_);
                if (chest >= 0) TryOpenChest(host_->world, chest);
                else DropLamp(host_->world, 1);
            }
        }
        host_->Tick(dt);
        dungeon_ = host_->dungeon;
    } else if (mode_ == NetMode::Join && join_) {
        // px_/py_ are this frame's collision-resolved position; the session
        // reports them verbatim. Passing dir (−1..1) here once sent the
        // joiner to (0,0) -- the exact wall-spawn bug in the screenshots.
        join_->Tick(dt, px_, py_, facing_.x, facing_.y, actions);
        if (swing) dropQueued_ = false;  // swing travels inside Input actions
        if (shoot && shootCd_ <= 0) { shootCd_ = 0.6f; join_->Shoot(); }
        if (drop) {
            if (!TryPickUpLamp(join_->world, 2)) {
                int chest = join_->ChestNearby(px_, py_, join_->world);
                if (chest >= 0) join_->OpenChest(chest);
            }
        }
        if (join_->ready) {
            dungeon_ = join_->dungeon;
            px_ = join_->myX;
            py_ = join_->myY;
        }
    } else {
        if (swing && meleeCd_ <= 0) { meleeCd_ = 0.4f; }
        if (drop) {
            if (!TryPickUpLamp(world_, 1)) {
                int chest = ChestInReach(world_, px_, py_);
                if (chest >= 0) TryOpenChest(world_, chest);
                else DropLamp(world_, 1);
            }
        }
        world_.hostX = px_;
        world_.hostY = py_;
        for (Monster &m : world_.monsters) TickMonster(m, world_, kMonsterSpeed, kMonsterTouchDps, static_cast<float>(dt));
    }

    if (player_) {
        player_->GetComponent<TransformComponent>().position = glm::vec2(px_, py_);
        // Flip only, never negative scale. A negative scale makes
        // RenderSystem emit dstRect.w = sprite.width * scale = -48, which
        // shifts the visible sprite a full tile into the wall while the
        // collision box holds. Facing is flip alone.
        player_->GetComponent<SpriteComponent>().flip =
            faceLeft_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    }
    if (remote_) {
        float rx = px_ + 48, ry = py_;
        if (host_) { rx = host_->world.joinX; ry = host_->world.joinY; }
        if (join_) { rx = join_->world.hostX; ry = join_->world.hostY; }
        remote_->GetComponent<TransformComponent>().position = glm::vec2(rx, ry);
    }

    camera_ = CameraFor(glm::vec2(px_, py_), windowWidth_, windowHeight_,
                        glm::vec2(dungeon_.cols_ * static_cast<float>(static_cast<int>(kTile) * kScale),
                                  dungeon_.rows_ * static_cast<float>(static_cast<int>(kTile) * kScale)));
    cameraSdl_.x = camera_.x; cameraSdl_.y = camera_.y;
    cameraSdl_.w = camera_.w; cameraSdl_.h = camera_.h;
}

void PlayState::DrawSprites() {
    SDL_Texture *tex = assets_->GetTexture("sheet");
    if (!tex) return;
    auto blit = [&](float wx, float wy, TileRect t, int flip) {
        SDL_Rect src{t.x, t.y, t.w, t.h};
        SDL_Rect dst{static_cast<int>(wx) - cameraSdl_.x - 24,
                     static_cast<int>(wy) - cameraSdl_.y - 24, 48, 48};
        SDL_RenderCopyEx(renderer_, tex, &src, &dst, 0.0, nullptr,
                         flip == 1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    };
    // Verified cells: monster (10,0), chest closed (0,10)/open (2,9), lamp (7,8)
    const auto draw = [&](const WorldState &w) {
        for (const Monster &m : w.monsters) if (m.alive) blit(m.x, m.y, TileAt(1, 10), 0);
        for (const Chest &ch : w.chests) blit(ch.x, ch.y, ch.opened ? TileAt(8, 7) : TileAt(5, 7), 0);
        blit(w.lampX, w.lampY, TileAt(8, 10), 0);
    };
    if (host_) draw(host_->world);
    else if (join_) draw(join_->world);
    else draw(world_);
}

void PlayState::render() {
    SDL_SetRenderDrawColor(renderer_, 8, 8, 16, 255);
    SDL_RenderClear(renderer_);
    DrawDungeon(renderer_, *assets_, dungeon_, cameraSdl_, kScale);
    DrawSprites();
    registry_.GetSystem<RenderSystem>().Update(renderer_, *assets_, &cameraSdl_);
    if (isDebugging_) registry_.GetSystem<RenderColliderSystem>().Update(renderer_, &cameraSdl_);

    // Light follows the lamp. Params.centre is baked into the texture, so a
    // moving lamp would tick Build every frame -- per-pixel work the design
    // forbids. The fix here is quantization: the centre snaps to a 32px grid,
    // so Build runs only when the lamp crosses a boundary (a 4px jump at
    // sight distance is invisible at lamp scale). No per-frame rebuilds.
    float lx, ly;
    if (host_) { lx = host_->world.lampX; ly = host_->world.lampY; }
    else if (join_) { lx = join_->world.lampX; ly = join_->world.lampY; }
    else { lx = world_.lampX; ly = world_.lampY; }
    const glm::vec2 raw(lx - camera_.x, ly - camera_.y);
    constexpr float kSnap = 32.0f;
    const glm::vec2 centre(std::round(raw.x / kSnap) * kSnap,
                           std::round(raw.y / kSnap) * kSnap);
    if (!lampBuilt_ || centre != lampBuildCentre_) {
        LightingOverlay::Params lp;
        lp.width = windowWidth_;
        lp.height = windowHeight_;
        lp.centre = centre;
        // Darkness has teeth: outside the lamp's pool is nearly opaque. The
        // lamp is the visibility budget; the non-carrier works at its edge.
        lp.radius = 200.0f;
        lp.keyOpacity = 96;
        lp.vignetteOpacity = 235;
        lp.falloff = 2.6f;
        if (lamp_.Build(renderer_, lp)) {
            lampBuildCentre_ = centre;
            lampBuilt_ = true;
        }
    }
    lamp_.Draw(renderer_);
    SDL_RenderPresent(renderer_);
}

} // namespace storm
