#include "plant.h"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <memory>
#include <utility>

#include "engine/app.h"
#include "engine/collider.h"
#include "engine/node.h"
#include "engine/rectangle_collider.h"
#include "engine/sprite_sheet_animation.h"
#include "engine/state.h"
#include "engine/tilemap.h"
#include "engine/transition.h"
#include "plant_bullet.h"
#include "player.h"

namespace game {

static constexpr int32_t kAnimationTPF = 4;

Plant::IdleState::IdleState(ng::State<Context>::ID id,
                            ng::SpriteSheetAnimation animation)
    : ng::State<Context>(std::move(id)), animation_(std::move(animation)) {}

void Plant::IdleState::OnEnter() {
  animation_.Start();
}

void Plant::IdleState::Update() {
  animation_.Update();
}

Plant::AttackState::AttackState(ng::State<Context>::ID id,
                                ng::SpriteSheetAnimation animation,
                                const Plant& plant, const ng::Tilemap& tilemap,
                                sf::Vector2f direction)
    : ng::State<Context>(std::move(id)),
      animation_(std::move(animation)),
      plant_(&plant),
      tilemap_(&tilemap),
      direction_(direction) {
  animation_.RegisterOnEndCallback(
      [this]() -> void { GetContext()->is_attacking = false; });
}

void Plant::AttackState::OnEnter() {
  animation_.Start();
}

void Plant::AttackState::Update() {
  animation_.Update();
  if (animation_.GetFrameIndex() == 4 && animation_.GetTicksCounter() == 0) {
    Attack();
  }
}

void Plant::AttackState::Attack() {
  auto& bullet =
      plant_->GetParent()->MakeChild<PlantBullet>(*tilemap_, direction_);
  bullet.SetLocalPosition(plant_->GetLocalTransform().getPosition() +
                          sf::Vector2f{-16.F, -6.F});
}

Plant::HitState::HitState(ng::State<Context>::ID id,
                          ng::SpriteSheetAnimation animation,
                          const sf::SoundBuffer& sound_buffer, Plant& plant)
    : ng::State<Context>(std::move(id)),
      animation_(std::move(animation)),
      sound_(sound_buffer),
      plant_(&plant) {
  animation_.RegisterOnEndCallback([this]() { Die(); });
}

void Plant::HitState::OnEnter() {
  animation_.Start();
  sound_.play();
}

void Plant::HitState::Update() {
  animation_.Update();
}

void Plant::HitState::Die() {
  plant_->Destroy();
}

Plant::Plant(ng::App& app, const ng::Tilemap& tilemap)
    : ng::Node(app),
      tilemap_(&tilemap),
      sprite_(
          GetApp().GetResourceManager().LoadTexture("Plant/Idle (44x42).png")),
      animator_(context_, std::make_unique<IdleState>(
                              "idle", ng::SpriteSheetAnimation(
                                          sprite_, &sprite_.getTexture(),
                                          kAnimationTPF, {44, 42}))) {
  SetName("Plant");
  sprite_.setScale({2, 2});
  sprite_.setOrigin({22, 21});
  sprite_.setTextureRect(sf::IntRect({0, 0}, {44, 42}));

  auto& collider = MakeChild<ng::RectangleCollider>(sf::Vector2f(40, 42));
  collider.SetLocalPosition({8, 0});
  collider_ = &collider;

  animator_.AddState(std::make_unique<AttackState>(
      "attack",
      ng::SpriteSheetAnimation(sprite_,
                               &GetApp().GetResourceManager().LoadTexture(
                                   "Plant/Attack (44x42).png"),
                               kAnimationTPF, {44, 42}),
      *this, *tilemap_, direction_));
  animator_.AddState(std::make_unique<HitState>(
      "hit",
      ng::SpriteSheetAnimation(
          sprite_,
          &GetApp().GetResourceManager().LoadTexture("Plant/Hit (44x42).png"),
          kAnimationTPF, {44, 42}),
      GetApp().GetResourceManager().LoadSoundBuffer("Mushroom/Hit_2.wav"),
      *this));

  animator_.AddTransition(ng::Transition<Context>(
      "idle", "hit", [](Context context) -> bool { return context.is_dead; }));
  animator_.AddTransition(ng::Transition<Context>(
      "idle", "attack",
      [](Context context) -> bool { return context.is_attacking; }));
  animator_.AddTransition(ng::Transition<Context>(
      "attack", "idle",
      [](Context context) -> bool { return !context.is_attacking; }));
  animator_.AddTransition(ng::Transition<Context>(
      "attack", "hit",
      [](Context context) -> bool { return context.is_dead; }));
}

bool Plant::GetIsDead() const {
  return context_.is_dead;
}

void Plant::TakeDamage() {
  context_.is_dead = true;
}

void Plant::Update() {
  animator_.Update();

  if (context_.is_dead) {
    return;
  }

  static constexpr int32_t kAttackCooldown = 240;
  if (attack_timer_ > 0) {
    --attack_timer_;
  } else {
    context_.is_attacking = true;
    attack_timer_ = kAttackCooldown;
  }

  const ng::Collider* other = GetScene().GetPhysics().Overlap(*collider_);
  if (other != nullptr) {
    if (other->GetParent()->GetName() == "Player") {
      auto* player = dynamic_cast<Player*>(other->GetParent());
      if (player->GetVelocity().y <= 0) {
        player->TakeDamage();
      }
    }
  }
}

void Plant::Draw(sf::RenderTarget& target) {
  sprite_.setScale(sf::Vector2f{-direction_.x * 2, 2.F});
  target.draw(sprite_, GetGlobalTransform().getTransform());
}

}  // namespace game