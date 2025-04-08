// resource_manager.h
#pragma once

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ng {

/// @brief Manages the loading and caching of game resources such as textures, sound buffers, and fonts.
///        Ensures that resources are loaded only once and provides access to them.
class ResourceManager {
 public:
  ResourceManager() = default;
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager& other) = delete;
  ResourceManager& operator=(const ResourceManager& other) = delete;
  ResourceManager(ResourceManager&& other) = delete;
  ResourceManager& operator=(ResourceManager&& other) = delete;

  /// @brief Loads a texture from the specified file path. If the texture is already loaded, returns the cached instance.
  /// @param filename The relative path to the texture file.
  /// @return A reference to the loaded SFML Texture. Lifetime is bound to the resource manager instance.
  sf::Texture& LoadTexture(const std::filesystem::path& filename);

  /// @brief Loads a sound buffer from the specified file path. If the sound buffer is already loaded, returns the cached instance.
  /// @param filename The relative path to the sound buffer file.
  /// @return A reference to the loaded SFML SoundBuffer. Lifetime is bound to the resource manager instance.
  sf::SoundBuffer& LoadSoundBuffer(const std::filesystem::path& filename);

  /// @brief Loads a font from the specified file path. If the font is already loaded, returns the cached instance.
  /// @param filename The relative path to the font file.
  /// @return A reference to the loaded SFML Font. Lifetime is bound to the resource manager instance.
  sf::Font& LoadFont(const std::filesystem::path& filename);

 private:
  /// @brief The prefix for all resource file paths.
  static constexpr std::string_view kPrefix_ = "resources/";

  /// @brief Cache for loaded textures, mapping file paths to unique pointers of SFML Texture.
  std::unordered_map<std::filesystem::path, std::unique_ptr<sf::Texture>>
      textures_;
  /// @brief Cache for loaded sound buffers, mapping file paths to unique pointers of SFML SoundBuffer.
  std::unordered_map<std::filesystem::path, std::unique_ptr<sf::SoundBuffer>>
      sound_buffers_;
  /// @brief Cache for loaded fonts, mapping file paths to unique pointers of SFML Font.
  std::unordered_map<std::filesystem::path, std::unique_ptr<sf::Font>> fonts_;
};

}  // namespace ng