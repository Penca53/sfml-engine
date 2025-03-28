#include "resource_manager.h"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <filesystem>
#include <memory>
#include <utility>

namespace ng {

ResourceManager& ResourceManager::GetInstance() {
  static ResourceManager instance;
  return instance;
}

sf::Font& ResourceManager::LoadFont(const std::filesystem::path& filename) {
  std::filesystem::path full_path = kPrefix_ / filename;
  if (!fonts_.contains(full_path.string())) {
    auto font = std::make_unique<sf::Font>(full_path);
    fonts_.insert({full_path.string(), std::move(font)});
  }

  return *fonts_.at(full_path.string());
}

sf::Texture& ResourceManager::LoadTexture(
    const std::filesystem::path& filename) {
  std::filesystem::path full_path = kPrefix_ / filename;
  if (!textures_.contains(full_path.string())) {
    auto texture = std::make_unique<sf::Texture>(full_path);
    textures_.insert({full_path.string(), std::move(texture)});
  }

  return *textures_.at(full_path.string());
}

sf::SoundBuffer& ResourceManager::LoadSoundBuffer(
    const std::filesystem::path& filename) {
  std::filesystem::path full_path = kPrefix_ / filename;
  if (!sound_buffers_.contains(full_path.string())) {
    auto sound_buffer = std::make_unique<sf::SoundBuffer>(full_path);
    sound_buffers_.insert({full_path.string(), std::move(sound_buffer)});
  }

  return *sound_buffers_.at(full_path.string());
}

}  // namespace ng