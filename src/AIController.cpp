#include "AIController.hpp"
#include <Geode/Geode.hpp>
#include <chrono>
#include <algorithm>

using namespace geode::prelude;

AIController* AIController::get() {
    static AIController instance;
    return &instance;
}

AIController::AIController() {
    m_rng = std::mt19937(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
}

void AIController::setPlayLayer(PlayLayer* pl) {
    m_playLayer = pl;
}

void AIController::reset() {
    m_holdingJump       = false;
    m_timeSinceLastJump = 0.f;
    m_reactionDelay     = 0.f;
}

float AIController::randomDelay(float minMs, float maxMs) {
    std::uniform_real_distribution<float> dist(minMs, maxMs);
    return dist(m_rng);
}

void AIController::onUpdate(float dt) {
    if (!m_playLayer) return;

    m_timeSinceLastJump += dt;
    m_reactionDelay     -= dt;

    if (m_reactionDelay > 0.f) return;

    GameState state = captureState();
    if (state.isDead)   return;

    Decision decision = makeDecision(state);
    executeDecision(decision);
}

GameState AIController::captureState() {
    GameState state;
    auto* player = m_playLayer->m_player1;

    state.playerY           = player->getPositionY();
    state.playerVelocityY   = player->m_yVelocity;
    state.isOnGround        = player->m_isOnGround;
    state.isDead            = player->m_isDead;
    state.completionPercent = m_playLayer->getCurrentPercent();
    state.gameSpeed         = m_playLayer->m_gameState.m_timeWarp;
    state.upcomingObstacles = scanAhead(300.f);

    return state;
}

std::vector<ObstacleInfo> AIController::scanAhead(float scanDistance) {
    std::vector<ObstacleInfo> obstacles;
    if (!m_playLayer) return obstacles;

    auto* player = m_playLayer->m_player1;
    float playerX = player->getPositionX();

    auto& objects = m_playLayer->m_objects;
    for (auto* obj : objects) {
        if (!obj) continue;

        float objX = obj->getPositionX();
        float objY = obj->getPositionY();
        float dist = objX - playerX;

        if (dist > 0.f && dist <= scanDistance) {
            ObstacleInfo info;
            info.x    = dist;
            info.y    = objY;
            info.type = "obstacle";
            obstacles.push_back(info);
        }
    }

    std::sort(obstacles.begin(), obstacles.end(),
        [](const ObstacleInfo& a, const ObstacleInfo& b) {
            return a.x < b.x;
        });

    return obstacles;
}

bool AIController::shouldJump(const GameState& state) {
    if (state.upcomingObstacles.empty()) return false;

    float playerY         = state.playerY;
    const auto& nearest   = state.upcomingObstacles.front();
    bool obstacleIsClose  = nearest.x < 150.f;
    bool obstacleIsAbove  = nearest.y > playerY - 10.f;
    bool weAreOnGround    = state.isOnGround;
    bool haventJumpedYet  = m_timeSinceLastJump > 0.3f;

    return obstacleIsClose && obstacleIsAbove && weAreOnGround && haventJumpedYet;
}

bool AIController::shouldHold(const GameState& state) {
    if (state.upcomingObstacles.empty()) return false;

    const auto& nearest = state.upcomingObstacles.front();
    float playerY       = state.playerY;

    return (nearest.y > playerY && nearest.x < 200.f);
}

Decision AIController::makeDecision(const GameState& state) {
    Decision decision;
    decision.confidence = 0.5f;
    decision.action     = "none";

    if (shouldJump(state)) {
        decision.action     = "jump";
        decision.confidence = 0.9f;
        m_reactionDelay     = randomDelay(0.05f, 0.15f);
    }
    else if (m_holdingJump && shouldHold(state)) {
        decision.action     = "hold";
        decision.confidence = 0.7f;
    }
    else if (m_holdingJump && !shouldHold(state)) {
        decision.action     = "release";
        decision.confidence = 0.8f;
    }

    return decision;
}

void AIController::executeDecision(const Decision& decision) {
    if (!m_playLayer) return;

    if (decision.action == "jump") {
        m_playLayer->pushButton(PlayerButton::Jump, true);
        m_holdingJump       = true;
        m_timeSinceLastJump = 0.f;
    }
    else if (decision.action == "release") {
        m_playLayer->releaseButton(PlayerButton::Jump, true);
        m_holdingJump = false;
    }
}
