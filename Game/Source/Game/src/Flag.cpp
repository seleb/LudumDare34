#pragma once

#include <Flag.h>
#include <MY_ResourceManager.h>

Flag::Flag(Box2DWorld * _world, Shader * _shader) :
	Box2DSprite(_world, b2_staticBody, _shader)
{
	SpriteSheet * s = new SpriteSheet(MY_ResourceManager::scenario->getTexture("FLAG")->texture);
	s->addAnimation("IDLE", 0, 3, 16, 16, 1);
	height = 16;
	width = 16;
	//meshTransform->scale(32);
	setSpriteSheet(s, "IDLE");

	mesh->setScaleMode(GL_NEAREST);
	//createFixture();
}