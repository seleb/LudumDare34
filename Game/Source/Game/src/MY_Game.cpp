#pragma once

#include <MY_Game.h>
#include <MY_Scene.h>

MY_Game::MY_Game() :
	Game(true, std::pair<std::string, Scene *>("test", new MY_Scene(this)), false)
{
}

MY_Game::~MY_Game(){

}