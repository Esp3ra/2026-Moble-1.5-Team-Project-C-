#pragma once

#include "Common.hpp"

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <string_view>

namespace devil::render
{
extern const sf::Color earth;
extern const sf::Color air;
extern const sf::Color ink;
extern const sf::Color light;
extern const sf::Color silhouette;

void box(sf::RenderTarget& target, float x, float y, float width, float height, sf::Color color);
void label(sf::RenderTarget& target, std::string_view text, float x, float y,
    float scale, sf::Color color, bool centered = false);
void drawDoor(sf::RenderTarget& target, float x, float floorY, float width, float height);
void drawPlayer(sf::RenderTarget& target, const PlayerState& player,
    float scale = 1.f, float opacity = 1.f);
std::size_t walkingFrame(float modelWalkTime);
}
