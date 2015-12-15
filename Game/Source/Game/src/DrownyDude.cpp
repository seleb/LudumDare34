#pragma once

#include <DrownyDude.h>
#include <MY_ResourceManager.h>

SpriteSheet * DrownyDude::globalSpriteSheet = nullptr;
DrownyDude::DrownyDude(Box2DWorld * _world, Shader * _shader) :
	Box2DSprite(_world, b2_dynamicBody, _shader)
{
	if(globalSpriteSheet == nullptr){
		globalSpriteSheet = new SpriteSheet(MY_ResourceManager::scenario->getTexture("DROWNY")->texture);
		globalSpriteSheet->addAnimation("IDLE", 0, 3, 16, 16, 1);
	}
	height = 16;
	width = 16;
	//meshTransform->scale(32);
	setSpriteSheet(globalSpriteSheet, "IDLE");

	mesh->setScaleMode(GL_NEAREST);
	createFixture();
}