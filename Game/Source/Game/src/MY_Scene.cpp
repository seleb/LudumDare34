#pragma once

#include <MY_Scene.h>
#include <MY_ResourceManager.h>

#include <Game.h>
#include <MeshEntity.h>
#include <MeshInterface.h>
#include <MeshFactory.h>
#include <Resource.h>

#include <DirectionalLight.h>
#include <PointLight.h>
#include <SpotLight.h>

#include <shader\ComponentShaderBase.h>
#include <shader\ComponentShaderText.h>
#include <shader\ShaderComponentText.h>
#include <shader\ShaderComponentTexture.h>
#include <shader\ShaderComponentDiffuse.h>
#include <shader\ShaderComponentHsv.h>
#include <Material.h>
#include <shader\ShaderComponentHsv.h>
#include <shader\ShaderComponentMVP.h>
#include <shader\ShaderComponentWorldSpaceUVs.h>

#include <shader\ShaderComponentIndexedTexture.h>
#include <TextureColourTable.h>

#include <Box2DWorld.h>
#include <Box2DMeshEntity.h>
#include <Box2DDebugDrawer.h>
#include <Box2DSprite.h>
#include <SpriteSheet.h>
#include <SpriteSheetAnimation.h>

#include <MousePerspectiveCamera.h>
#include <FollowCamera.h>

#include <System.h>
#include <Mouse.h>
#include <Keyboard.h>
#include <GLFW\glfw3.h>

#include <RenderSurface.h>
#include <StandardFrameBuffer.h>
#include <NumberUtils.h>

#include <NodeBulletBody.h>
#include <BulletMeshEntity.h>
#include <TextArea.h>
#include <Box2DWorld.h>
#include <Box2DDebugDrawer.h>
#include <DrownyDude.h>

#include <RenderOptions.h>

MY_Scene::MY_Scene(Game * _game) :
	Scene(_game),
	screenSurfaceShader(new Shader("assets/engine basics/DefaultRenderSurface", false, true)),
	screenSurface(new RenderSurface(screenSurfaceShader)),
	screenFBO(new StandardFrameBuffer(true)),
	baseShader(new ComponentShaderBase(true)),
	worldspaceShader(new ComponentShaderBase(true)),
	textShader(new ComponentShaderText(true)),
	debugDrawer(nullptr),
	uiLayer(0,0,0,0),
	box2dWorld(new Box2DWorld(b2Vec2(0.f, -10.0f))),
	box2dDebug(new Box2DDebugDrawer(box2dWorld)),
	moving(false),
	giveUp(false)
{
	hsvComponent1 = new ShaderComponentHsv(baseShader, 0, 1, 1);
	hsvComponent2 = new ShaderComponentHsv(worldspaceShader, 0, 1, 1);


	baseShader->addComponent(new ShaderComponentMVP(baseShader));
	baseShader->addComponent(new ShaderComponentTexture(baseShader));
	baseShader->addComponent(new ShaderComponentDiffuse(baseShader));
	baseShader->addComponent(hsvComponent1);
	baseShader->compileShader();

	
	worldspaceShader->addComponent(new ShaderComponentMVP(worldspaceShader));
	worldspaceShader->addComponent(new ShaderComponentTexture(worldspaceShader));
	ShaderComponentWorldSpaceUVs * uvComponent = new ShaderComponentWorldSpaceUVs(worldspaceShader);
	uvComponent->xMultiplier = 0.005f;
	uvComponent->yMultiplier = 0.005f;
	worldspaceShader->addComponent(uvComponent);
	worldspaceShader->addComponent(new ShaderComponentDiffuse(worldspaceShader));
	worldspaceShader->addComponent(hsvComponent2);
	worldspaceShader->compileShader();

	textShader->textComponent->setColor(glm::vec3(0.0f, 0.0f, 0.0f));


	// remove initial camera
	childTransform->removeChild(cameras.at(0)->parents.at(0));
	delete cameras.at(0)->parents.at(0);
	cameras.pop_back();

	//Set up debug camera
	debugCam = new MousePerspectiveCamera();
	cameras.push_back(debugCam);
	childTransform->addChild(debugCam);
	debugCam->farClip = 1000.f;
	debugCam->childTransform->rotate(90, 0, 1, 0, kWORLD);
	debugCam->parents.at(0)->translate(5.0f, 1.5f, 22.5f);
	debugCam->yaw = 90.0f;
	debugCam->pitch = -10.0f;
	debugCam->speed = 1;

	playerCam = new OrthographicCamera(-192/3, 192/3, -108/3, 108/3, -10, 100);
	childTransform->addChild(playerCam);
	
	cameras.push_back(playerCam);

	activeCamera = playerCam;

	childTransform->addChild(box2dDebug, false);
	box2dDebug->drawing = false;
	box2dWorld->b2world->SetDebugDraw(box2dDebug);
	box2dDebug->AppendFlags(b2Draw::e_shapeBit);
	box2dDebug->AppendFlags(b2Draw::e_centerOfMassBit);
	box2dDebug->AppendFlags(b2Draw::e_jointBit);

	//uiLayer.addMouseIndicator();

	SpriteSheet * dudeSpriteSheet = new SpriteSheet(MY_ResourceManager::scenario->getTexture("DUDE-ANIMATION")->texture);
	for(unsigned long int i = 0; i < NUM_SIZES; ++i){
		float pingPonged = (float)sweet::NumberUtils::pingPong(i*2, 1, NUM_SIZES)/NUM_SIZES;
		std::stringstream ss;
		ss << "IDLE_" << (i+1);
		dudeSpriteSheet->addAnimation(ss.str(), i*4, i*4, 16, 16, 1);
		ss.str(""); // clear
		ss << "RUN_" << (i+1);
		dudeSpriteSheet->addAnimation(ss.str(), i*4, i*4+3, 16, 16, (1.f-pingPonged*0.5f)*0.25f+0.01f);
	}

	ground = new Box2DSprite(box2dWorld, MY_ResourceManager::scenario->getTextureSampler("GROUND")->textureSampler, b2_staticBody, worldspaceShader);
	childTransform->addChild(ground);//->scale(1920, 1, 1);
	ground->setTranslationPhysical(0, -ground->getCorrectedHeight()*0.5f, 5);
	ground->mesh->setScaleMode(GL_NEAREST);
	ground->createFixture();
	for(unsigned long int i = 0; i < ground->mesh->vertices.size(); ++i){
		ground->mesh->vertices.at(i).y += ground->getCorrectedHeight()*0.5f;
	}

	ground->childTransform->createAntTweakBarWindow("Ground");

	player = new Box2DSprite(box2dWorld, MY_ResourceManager::scenario->getTextureSampler("DUDE")->textureSampler, b2_dynamicBody, baseShader);
	childTransform->addChild(player);
	player->mesh->setScaleMode(GL_NEAREST);
	player->setSpriteSheet(dudeSpriteSheet, "IDLE_1");
	player->createFixture();
	player->setTranslationPhysical(0, 8, 0);

	playerSize = 1;



	/*for(unsigned long int i = 0; i < 5; ++i){
		DrownyDude * dd = new DrownyDude(box2dWorld, baseShader);

		dd->setTranslationPhysical(sweet::NumberUtils::randomInt(-3, 3), 0, 0);
		childTransform->addChild(dd);
	}*/


	
	light = new PointLight(glm::vec3(1), 0.f, 0.001f, -1.f);
	lights.push_back(light);
	player->meshTransform->addChild(light);
	light->firstParent()->translate(0, -8, 10);
	
	SpriteSheet * buttonSpriteSheet = new SpriteSheet(MY_ResourceManager::scenario->getTexture("BUTTONS")->texture);
	buttonSpriteSheet->addAnimation("X_UP", 0, 0, 16, 16, 1);
	buttonSpriteSheet->addAnimation("X_DOWN", 1, 1, 16, 16, 1);
	buttonSpriteSheet->addAnimation("Z_UP", 2, 2, 16, 16, 1);
	buttonSpriteSheet->addAnimation("Z_DOWN", 3, 3, 16, 16, 1);

	xButton = new Sprite(uiLayer.shader);
	zButton = new Sprite(uiLayer.shader);
	xButton->setPrimaryTexture(MY_ResourceManager::scenario->getTextureSampler("BUTTON-BASE")->textureSampler);
	zButton->setPrimaryTexture(MY_ResourceManager::scenario->getTextureSampler("BUTTON-BASE")->textureSampler);
	xButton->childTransform->scale(32);
	zButton->childTransform->scale(32);
	xButton->setSpriteSheet(buttonSpriteSheet, "X_UP");
	zButton->setSpriteSheet(buttonSpriteSheet, "Z_UP");
	xButton->mesh->setScaleMode(GL_NEAREST);
	zButton->mesh->setScaleMode(GL_NEAREST);

	images = new NodeUI(uiLayer.world);
	images->setRationalHeight(1.f);
	images->setRationalWidth(1.f);
	images->setBackgroundColour(1,1,1,1);
	images->background->mesh->pushTexture2D(MY_ResourceManager::scenario->getTexture("INTRO")->texture);
	images->background->mesh->setScaleMode(GL_NEAREST);

	HorizontalLinearLayout * hl = new HorizontalLinearLayout(uiLayer.world);
	hl->setRationalHeight(1.f);
	hl->setRationalWidth(1.f);
	hl->setPaddingBottom(0.1f);
	hl->horizontalAlignment = kCENTER;
	
	NodeUI * xui = new NodeUI(uiLayer.world);
	xui->setWidth(16);
	xui->setHeight(16);
	xui->background->setVisible(false);
	xui->uiElements->addChild(xButton);
	NodeUI * pad = new NodeUI(uiLayer.world);
	pad->setWidth(16);
	pad->setHeight(16);
	pad->background->setVisible(false);
	NodeUI * zui = new NodeUI(uiLayer.world);
	zui->setWidth(16);
	zui->setHeight(16);
	zui->background->setVisible(false);
	zui->uiElements->addChild(zButton);


	uiLayer.addChild(images);
	images->addChild(hl);
	hl->addChild(zui);
	hl->addChild(pad);
	hl->addChild(xui);

	flag = new Flag(box2dWorld, baseShader);
	childTransform->addChild(flag);
	flag->setTranslationPhysical(DISTANCE, 16/2,0);
	flag->createFixture();

	lowered = false;
}

MY_Scene::~MY_Scene(){
	deleteChildTransform();
	baseShader->safeDelete();
	worldspaceShader->safeDelete();
	textShader->safeDelete();

	screenSurface->safeDelete();
	//screenSurfaceShader->safeDelete();
	screenFBO->safeDelete();
}


void MY_Scene::update(Step * _step){
	//hsvComponent1->setHue(_step->time/100.f);
	//hsvComponent2->setHue(_step->time/100.f);
	if(keyboard->keyJustDown(GLFW_KEY_ESCAPE)){
		game->exit();
		return;
	}

	glm::uvec2 sd = sweet::getScreenDimensions();
	
	uiLayer.resize(0, sd.x/10, 0, sd.y/10);
	
	float w = uiLayer.cam.getWidth();
	float h = uiLayer.cam.getHeight();
	playerCam->resize(uiLayer.cam.left - w/3, uiLayer.cam.right - w/3, uiLayer.cam.bottom - h/3, uiLayer.cam.top - h/3);

	glm::vec3 playerPos = player->meshTransform->getWorldPos();
	glm::vec3 groundPos = ground->meshTransform->getWorldPos();
	box2dWorld->update(_step);

	glm::vec3 camPos = playerCam->getWorldPos();


	playerCam->firstParent()->translate((playerPos.x - camPos.x)*0.01f, (playerPos.y - camPos.y)*0.01f, 0);
	ground->setTranslationPhysical(playerPos.x, groundPos.y, groundPos.z);
	
	if(keyboard->keyJustDown(GLFW_KEY_F12)){
		game->toggleFullScreen();
	}



	// button animations
	if(keyboard->keyDown(GLFW_KEY_X)){
		xButton->setCurrentAnimation("X_DOWN");
	}else{
		xButton->setCurrentAnimation("X_UP");
	}if(keyboard->keyDown(GLFW_KEY_Z)){
		zButton->setCurrentAnimation("Z_DOWN");
	}else{
		zButton->setCurrentAnimation("Z_UP");
	}

	if(!giveUp){
		// player controls
		if(keyboard->keyDown(GLFW_KEY_X)){
			moving = true;
		}else{
			moving = false;
		}if(keyboard->keyDown(GLFW_KEY_Z)){
			giveUp = true;
		}else{
		}

		playerSize = (NUM_SIZES - ((glm::distance(player->meshTransform->getWorldPos().x, flag->meshTransform->getWorldPos().x))/ DISTANCE)*NUM_SIZES)+1;
	
		float pingPonged = (float)sweet::NumberUtils::pingPong(playerSize*2, 1, NUM_SIZES)/NUM_SIZES;
	
		light->setAmbientCoefficient(light->getAmbientCoefficient() + (pingPonged - light->getAmbientCoefficient())*0.01f);
	
		// stop the player at the last phase
		if(playerSize >= NUM_SIZES){
			moving = false;
		}

		if(playerSize == 10 && !lowered){
			lowered = true;
			for(unsigned long int i = 0; i < player->mesh->vertices.size(); ++i){
				player->mesh->vertices.at(i).y -= 2;
			}
			player->mesh->dirty = true;
		}

		if(playerSize == NUM_SIZES){
			giveUp = true;
		}

		std::stringstream anim;
		if(moving){
			player->body->SetLinearVelocity(b2Vec2(pingPonged*10.f+0.1f, 0));
			anim << "RUN_";
		}else{
			player->body->SetLinearVelocity(b2Vec2(0, 0));
			anim << "IDLE_";
			for(unsigned long int i = 0; i < NUM_SIZES; ++i){
				std::stringstream ss;
				ss << "RUN_" << (i+1);
				player->spriteSheet->animations.at(ss.str())->frameIndices.currentAnimationTime = 0;
				player->spriteSheet->animations.at(ss.str())->frameIndices.currentTween = 0;
				player->spriteSheet->animations.at(ss.str())->frameIndices.currentTweenTime = 0;
			}
		}
		anim << playerSize;
		player->setCurrentAnimation(anim.str());


		if(giveUp){
			std::stringstream ss;
			ss << "IDLE_" << NUM_SIZES;
			player->setCurrentAnimation(ss.str());
			player->body->SetLinearVelocity(b2Vec2(0, 0));
			images->background->setVisible(true);
			while(images->background->mesh->textures.size() > 0){
				images->background->mesh->popTexture2D();
			}
			images->background->mesh->pushTexture2D(MY_ResourceManager::scenario->getTexture("END")->texture);
		}
	}else{
		if(keyboard->keyJustDown(GLFW_KEY_X)){
			std::stringstream ss;
			ss << _step->time;
			game->scenes[ss.str()] = new MY_Scene(game);
			game->switchScene(ss.str(), true);
		}else if(keyboard->keyJustDown(GLFW_KEY_Z)){
			game->exit();
		}
	}

	Scene::update(_step);
	uiLayer.update(_step);
}

void MY_Scene::render(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions){
	_renderOptions->depthEnabled = false;
	_renderOptions->setClearColour(238/255.f, 252/255.f, 1.f, 1.f);

	screenFBO->resize(game->viewPortWidth, game->viewPortHeight);
	//Bind frameBuffer
	screenFBO->bindFrameBuffer();
	//render the scene to the buffer
	
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	_renderOptions->clear();
	Scene::render(_matrixStack, _renderOptions);

	//Render the buffer to the render surface
	screenSurface->render(screenFBO->getTextureId());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	uiLayer.render(_matrixStack, _renderOptions);
}

void MY_Scene::load(){
	Scene::load();	

	screenSurface->load();
	screenFBO->load();
	uiLayer.load();
}

void MY_Scene::unload(){
	uiLayer.unload();
	screenFBO->unload();
	screenSurface->unload();

	Scene::unload();	
}