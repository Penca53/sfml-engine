#include "player.h"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>

#include "banana.h"
#include "end.h"
#include "engine/app.h"
#include "engine/collider.h"
#include "engine/fsm.h"
#include "engine/input.h"
#include "engine/node.h"
#include "engine/rectangle_collider.h"
#include "engine/resource_manager.h"
#include "engine/state.h"
#include "engine/tilemap.h"
#include "game_manager.h"
#include "mushroom.h"
#include "plant.h"
#include "score_manager.h"
#include "tile_id.h"

namespace game {

static constexpr int32_t kAnimationTPF = 4;

Player::IdleState::IdleState(ng::State::ID id, sf::Sprite& sprite)
    : ng::State(std::move(id)),
      animation_(sprite, "Player/Idle (32x32).png", kAnimationTPF) {}

void Player::IdleState::OnEnter() {
  animation_.Start();
}

void Player::IdleState::Update() {
  animation_.Update();
}

Player::RunState::RunState(ng::State::ID id, sf::Sprite& sprite)
    : ng::State(std::move(id)),
      animation_(sprite, "Player/Run (32x32).png", kAnimationTPF) {}

void Player::RunState::OnEnter() {
  animation_.Start();
}

void Player::RunState::Update() {
  animation_.Update();
}

Player::JumpState::JumpState(ng::State::ID id, sf::Sprite& sprite)
    : ng::State(std::move(id)),
      animation_(sprite, "Player/Jump (32x32).png", kAnimationTPF),
      sound_(ng::App::GetInstance().GetMutableResourceManager().LoadSoundBuffer(
          "Player/Jump_2.wav")) {}

void Player::JumpState::OnEnter() {
  animation_.Start();
  sound_.play();
}

void Player::JumpState::Update() {
  animation_.Update();
}

Player::FallState::FallState(ng::State::ID id, sf::Sprite& sprite)
    : ng::State(std::move(id)),
      animation_(sprite, "Player/Fall (32x32).png", kAnimationTPF) {}

void Player::FallState::OnEnter() {
  animation_.Start();
}

void Player::FallState::Update() {
  animation_.Update();
}

Player::HitState::HitState(ng::State::ID id, sf::Sprite& sprite, ng::Node& node,
                           GameManager& game_manager)
    : ng::State(std::move(id)),
      animation_(sprite, "Player/Hit (32x32).png", kAnimationTPF),
      node_(&node),
      game_manager_(&game_manager) {
  animation_.RegisterOnEndCallback([this]() { Die(); });
}

void Player::HitState::OnEnter() {
  animation_.Start();
}

void Player::HitState::Update() {
  animation_.Update();
}

void Player::HitState::Die() {
  game_manager_->Lose();
  node_->Destroy();
}

Player::Player(ng::Tilemap& tilemap, ScoreManager& score_manager,
               GameManager& game_manager)
    : tilemap_(&tilemap),
      score_manager_(&score_manager),
      game_manager_(&game_manager),
      sprite_(ng::App::GetInstance().GetMutableResourceManager().LoadTexture(
          "Player/Idle (32x32).png")),
      animator_(std::make_unique<IdleState>("idle", sprite_)),
      plastic_block_sound_(
          ng::App::GetInstance().GetMutableResourceManager().LoadSoundBuffer(
              "Hit_1.wav")),
      banana_sound_(
          ng::App::GetInstance().GetMutableResourceManager().LoadSoundBuffer(
              "Banana/Collectibles_2.wav")) {
  SetName("Player");

  sprite_.setScale({2, 2});
  sprite_.setOrigin({16, 16});
  sprite_.setTextureRect(sf::IntRect({0, 0}, {32, 32}));

  auto collider = std::make_unique<ng::RectangleCollider>(sf::Vector2f(32, 48));
  collider->SetLocalPosition({0, 8});
  collider_ = collider.get();
  AddChild(std::move(collider));

  animator_.AddState(std::make_unique<RunState>("run", sprite_));
  animator_.AddState(std::make_unique<JumpState>("jump", sprite_));
  animator_.AddState(std::make_unique<FallState>("fall", sprite_));
  animator_.AddState(
      std::make_unique<HitState>("hit", sprite_, *this, *game_manager_));

  animator_.AddTransition(ng::Transition(
      "idle", "run", [&]() -> bool { return velocity_.x != 0; }));
  animator_.AddTransition(ng::Transition(
      "run", "idle", [&]() -> bool { return velocity_.x == 0; }));

  animator_.AddTransition(ng::Transition(
      "idle", "jump", [&]() -> bool { return velocity_.y < 0; }));
  animator_.AddTransition(
      ng::Transition("run", "jump", [&]() -> bool { return velocity_.y < 0; }));

  animator_.AddTransition(ng::Transition("jump", "fall", [&]() -> bool {
    return velocity_.y > 0 && !is_on_ground_;
  }));

  animator_.AddTransition(
      ng::Transition("jump", "idle", [&]() -> bool { return is_on_ground_; }));
  animator_.AddTransition(
      ng::Transition("fall", "idle", [&]() -> bool { return is_on_ground_; }));

  animator_.AddTransition(
      ng::Transition("idle", "hit", [&]() -> bool { return is_dead_; }));
  animator_.AddTransition(
      ng::Transition("run", "hit", [&]() -> bool { return is_dead_; }));
  animator_.AddTransition(
      ng::Transition("jump", "hit", [&]() -> bool { return is_dead_; }));
  animator_.AddTransition(
      ng::Transition("fall", "hit", [&]() -> bool { return is_dead_; }));
}

sf::Vector2f Player::GetVelocity() const {
  return velocity_;
}

void Player::TakeDamage() {
  if (is_dead_) {
    return;
  }

  is_dead_ = true;
}

namespace {

bool DoesCollide(sf::Vector2f position, const ng::Tilemap& tilemap) {
  TileID id = tilemap.GetWorldTile(position).GetID();
  return id == TileID::kInvisibleBarrier ||
         (id >= TileID::kDirtTopLeft && id <= TileID::kDirtBottomRight) ||
         (id >= TileID::kStoneHorizontalLeft &&
          id <= TileID::kStoneVerticalBottom) ||
         id == TileID::kPlasticBlock;
}

}  // namespace

void Player::Update() {  // NOLINT
  animator_.Update();

  if (is_dead_) {
    return;
  }

  sf::Vector2f dir;
  if (!has_won_) {
    if (ng::GetKey(sf::Keyboard::Scancode::A)) {
      dir.x += -1;
      sprite_.setScale(sf::Vector2f{-2.F, 2.F});
    }
    if (ng::GetKey(sf::Keyboard::Scancode::D)) {
      dir.x += 1;
      sprite_.setScale(sf::Vector2f{2.F, 2.F});
    }
  }

  velocity_.x = dir.x * 4;
  velocity_.y += 1;

  if (!has_won_ && is_on_ground_ &&
      ng::GetKeyDown(sf::Keyboard::Scancode::Space)) {
    velocity_.y -= 15;
  }

  sf::Vector2f old_pos = collider_->GetGlobalTransform().getPosition();
  sf::Vector2f new_pos = old_pos + velocity_;

  sf::Vector2f col_size = collider_->GetSize() / 2.F;
  sf::Vector2f tilemap_size = sf::Vector2f(tilemap_->GetTileSize());

  static constexpr float kEps = 0.001F;
  sf::Vector2f top_left = {new_pos.x - (col_size.x - kEps),
                           old_pos.y - (col_size.y - kEps)};
  sf::Vector2f middle_left = {new_pos.x + (col_size.x - kEps),
                              old_pos.y + (0 - kEps)};
  sf::Vector2f bottom_left = {new_pos.x - (col_size.x - kEps),
                              old_pos.y + (col_size.y - kEps)};

  if (!tilemap_->IsWithinWorldBounds(top_left) ||
      !tilemap_->IsWithinWorldBounds(middle_left) ||
      !tilemap_->IsWithinWorldBounds(bottom_left)) {
    is_dead_ = true;
    return;
  }

  if (velocity_.x < 0 && (DoesCollide(top_left, *tilemap_) ||
                          DoesCollide(middle_left, *tilemap_) ||
                          DoesCollide(bottom_left, *tilemap_))) {
    new_pos.x =
        std::ceil(top_left.x / tilemap_size.x) * tilemap_size.x + col_size.x;
    velocity_.x = 0;
  }

  sf::Vector2f top_right = {new_pos.x + (col_size.x - kEps),
                            old_pos.y - (col_size.y - kEps)};
  sf::Vector2f middle_right = {new_pos.x + (col_size.x - kEps),
                               old_pos.y + (0 - kEps)};
  sf::Vector2f bottom_right = {new_pos.x + (col_size.x - kEps),
                               old_pos.y + (col_size.y - kEps)};

  if (!tilemap_->IsWithinWorldBounds(top_right) ||
      !tilemap_->IsWithinWorldBounds(middle_right) ||
      !tilemap_->IsWithinWorldBounds(bottom_right)) {
    is_dead_ = true;
    return;
  }

  if (velocity_.x > 0 && (DoesCollide(top_right, *tilemap_) ||
                          DoesCollide(middle_right, *tilemap_) ||
                          DoesCollide(bottom_right, *tilemap_))) {
    new_pos.x =
        std::floor(top_right.x / tilemap_size.x) * tilemap_size.x - col_size.x;
    velocity_.x = 0;
  }

  top_left = {new_pos.x - (col_size.x - kEps), new_pos.y - (col_size.y - kEps)};
  top_right = {new_pos.x + (col_size.x - kEps),
               new_pos.y - (col_size.y - kEps)};

  if (!tilemap_->IsWithinWorldBounds(top_left) ||
      !tilemap_->IsWithinWorldBounds(top_right)) {
    is_dead_ = true;
    return;
  }

  if (velocity_.y < 0 &&
      (DoesCollide(top_left, *tilemap_) || DoesCollide(top_right, *tilemap_))) {
    {
      if (tilemap_->GetWorldTile(top_left).GetID() == TileID::kPlasticBlock) {
        tilemap_->SetWorldTile(top_left, TileID::kVoid);
        plastic_block_sound_.play();
      }

      if (tilemap_->GetWorldTile(top_right).GetID() == TileID::kPlasticBlock) {
        tilemap_->SetWorldTile(top_right, TileID::kVoid);
        plastic_block_sound_.play();
      }
    }

    new_pos.y =
        std::ceil(top_left.y / tilemap_size.y) * tilemap_size.y + col_size.y;
    velocity_.y = 0;
  }

  bottom_left = {new_pos.x - (col_size.x - kEps),
                 new_pos.y + (col_size.y - kEps)};
  bottom_right = {new_pos.x + (col_size.x - kEps),
                  new_pos.y + (col_size.y - kEps)};

  if (!tilemap_->IsWithinWorldBounds(bottom_left) ||
      !tilemap_->IsWithinWorldBounds(bottom_right)) {
    is_dead_ = true;
    return;
  }

  if (velocity_.y > 0 && (DoesCollide(bottom_left, *tilemap_) ||
                          DoesCollide(bottom_right, *tilemap_))) {
    new_pos.y = std::floor(bottom_left.y / tilemap_size.y) * tilemap_size.y -
                col_size.y;
    velocity_.y = 0;
    is_on_ground_ = true;
  } else {
    is_on_ground_ = false;
  }

  SetLocalPosition(new_pos - sf::Vector2f{0, 8});

  const ng::Collider* other =
      ng::App::GetInstance().GetMutablePhysics().Overlap(*collider_);
  if (other != nullptr) {
    if (other->GetParent()->GetName() == "Mushroom") {
      if (velocity_.y > 0) {
        auto* mushroom = dynamic_cast<Mushroom*>(other->GetParent());
        if (!mushroom->GetIsDead()) {
          mushroom->TakeDamage();
          velocity_.y = -10;
          score_manager_->AddScore(100);
        }
      }
    } else if (other->GetParent()->GetName() == "Plant") {
      if (velocity_.y > 0) {
        auto* plant = dynamic_cast<Plant*>(other->GetParent());
        if (!plant->GetIsDead()) {
          plant->TakeDamage();
          velocity_.y = -10;
          score_manager_->AddScore(150);
        }
      }
    } else if (other->GetParent()->GetName() == "Banana") {
      auto* banana = dynamic_cast<Banana*>(other->GetParent());
      if (!banana->GetIsCollected()) {
        banana->Collect();
        score_manager_->AddScore(500);
        banana_sound_.play();
      }
    } else if (other->GetParent()->GetName() == "End") {
      if (!has_won_) {
        velocity_.y = -15;
        dynamic_cast<End*>(other->GetParent())->EndGame();
        has_won_ = true;
      }
    }
  }
}

void Player::Draw(sf::RenderTarget& target) {
  target.draw(sprite_, GetGlobalTransform().getTransform());
}

}  // namespace game