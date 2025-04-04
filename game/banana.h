#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "engine/circle_collider.h"
#include "engine/fsm.h"
#include "engine/node.h"
#include "engine/sprite_sheet_animation.h"
#include "engine/tilemap.h"

namespace game {

class Banana : public ng::Node {
 public:
  Banana();

  bool GetIsCollected() const;
  void Collect();

 protected:
  void Update() override;
  void Draw(sf::RenderTarget& target) override;

 private:
  class IdleState : public ng::State {
   public:
    IdleState(ng::State::ID id, sf::Sprite& sprite);

   protected:
    void OnEnter() override;

    void Update() override;

   private:
    ng::SpriteSheetAnimation animation_;
  };

  sf::Sprite sprite_;
  bool is_collected_ = false;
  ng::FSM animator_;
};

}  // namespace game