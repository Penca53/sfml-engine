#include "end.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cstdint>
#include <memory>
#include <utility>

#include "engine/app.h"
#include "engine/rectangle_collider.h"
#include "engine/state.h"
#include "engine/transition.h"
#include "game_manager.h"

namespace game {

static constexpr int32_t kAnimationTPF = 4;

End::IdleState::IdleState(ng::State<Context>::ID id, sf::Sprite& sprite)
    : ng::State<Context>(std::move(id)),
      animation_(sprite, "End/End (Idle).png", kAnimationTPF) {}

void End::IdleState::OnEnter() {
  animation_.Start();
}

void End::IdleState::Update() {
  animation_.Update();
}

End::PressedState::PressedState(ng::State<Context>::ID id, sf::Sprite& sprite,
                                GameManager& game_manager)
    : ng::State<Context>(std::move(id)),
      animation_(sprite, "End/End (Pressed) (64x64).png", kAnimationTPF),
      game_manager_(&game_manager) {}

void End::PressedState::OnEnter() {
  animation_.Start();
  animation_.RegisterOnEndCallback([this]() {
    game_manager_->Win();
    GetContext()->is_pressed_ = false;
  });
}

void End::PressedState::Update() {
  animation_.Update();
}

End::End(GameManager& game_manager)
    : sprite_(ng::App::GetInstance().GetMutableResourceManager().LoadTexture(
          "End/End (Idle).png")),
      animator_(context_, std::make_unique<IdleState>("idle", sprite_)),
      game_manager_(&game_manager) {
  SetName("End");
  sprite_.setScale({2, 2});
  sprite_.setOrigin({32, 32});

  auto collider = std::make_unique<ng::RectangleCollider>(sf::Vector2f(60, 32));
  collider->SetLocalPosition({0, -20});
  AddChild(std::move(collider));

  animator_.AddState(
      std::make_unique<PressedState>("pressed", sprite_, *game_manager_));

  animator_.AddTransition(ng::Transition<Context>(
      "idle", "pressed",
      [](Context context) -> bool { return context.is_pressed_; }));
  animator_.AddTransition(ng::Transition<Context>(
      "pressed", "idle",
      [](Context context) -> bool { return !context.is_pressed_; }));
}

void End::EndGame() {
  context_.is_pressed_ = true;
}

void End::Update() {
  animator_.Update();
}

void End::Draw(sf::RenderTarget& target) {
  target.draw(sprite_, GetGlobalTransform().getTransform());
}

}  // namespace game