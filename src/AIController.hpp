#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <vector>
#include <deque>
#include <random>

using namespace geode::prelude;

struct ObstacleInfo {
    float x;
    float y;
    std::string type;
};

struct GameState {
    float playerY;
    float playerVelocityY;
    float gameSpeed;
    bool  isOnGround;
    bool  isDead;
    float completionPercent;
    std::vector<ObstacleInfo> upcomingObstacles;
};

struct Decision {
    std::string action;
    float confidence;
};

class AIController {
public:
    static AIController* get();
    void onUpdate(float dt);
    void reset();
    void setPlayLayer(PlayLayer* pl);

private:
    AIController();
    GameState captureState();
    Decision  makeDecision(const GameState& state);
    void      executeDecision(const Decision& decision);
    std::vector<ObstacleInfo> scanAhead(float scanDistance = 300.f);
    bool shouldJump(const GameState& state);
    bool shouldHold(const GameState& state);

    PlayLayer*   m_playLayer         = nullptr;
    bool         m_holdingJump       = false;
    float        m_timeSinceLastJump = 0.f;
    float        m_reactionDelay     = 0.f;
    std::mt19937 m_rng;
    float randomDelay(float minMs, float maxMs);
};
