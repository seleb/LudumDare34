#pragma once

#include <Box2DSprite.h>
#include <SpriteSheet.h>

class DrownyDude : public Box2DSprite{
	static SpriteSheet * globalSpriteSheet;
	
public:
	DrownyDude(Box2DWorld * _world, Shader * _shader);
};