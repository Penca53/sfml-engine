#include "tileset.h"

namespace ng {

Tileset::Tileset(sf::Vector2u tile_size, const sf::Texture& texture)
    : tile_size_(tile_size), texture_(&texture) {}

const Tile& Tileset::GetTile(TileID id) const {
  return tiles_.at(id);
}

void Tileset::AddTile(Tile tile) {
  tiles_.insert({tile.GetID(), tile});
}

sf::Vector2u Tileset::GetTileSize() const {
  return tile_size_;
}

const sf::Texture& Tileset::GetTexture() const {
  return *texture_;
}

}  // namespace ng