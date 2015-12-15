#pragma once

#include <Box2DSprite.h>
#include <SpriteSheet.h>

class Flag : public Box2DSprite{
public:
	Flag(Box2DWorld * _world, Shader * _shader);
};