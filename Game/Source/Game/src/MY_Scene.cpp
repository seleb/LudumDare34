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
	baseShader->addComponent(new ShaderComponentMVP(baseShader));
	baseShader->addComponent(new ShaderComponentTexture(baseShader));
	baseShader->addComponent(new ShaderComponentDiffuse(baseShader));
	baseShader->compileShader();

	
	worldspaceShader->addComponent(new ShaderComponentMVP(worldspaceShader));
	worldspaceShader->addComponent(new ShaderComponentTexture(worldspaceShader));
	ShaderComponentWorldSpaceUVs * uvComponent = new ShaderComponentWorldSpaceUVs(worldspaceShader);
	uvComponent->xMultiplier = 0.005f;
	uvComponent->yMultiplier = 0.005f;
	worldspaceShader->addComponent(uvComponent);
	worldspaceShader->addComponent(new ShaderComponentDiffuse(worldspaceShader));
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
		std::stringstream ss;
		ss << "IDLE_" << (i+1);
		dudeSpriteSheet->addAnimation(ss.str(), i*4, i*4, 16, 16, 1);
		ss.str(""); // clear
		ss << "RUN_" << (i+1);
		dudeSpriteSheet->addAnimation(ss.str(), i*4, i*4+3, 16, 16, 0.16f);
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

	playerSize = 1;

	
	PointLight * light1 = new PointLight(glm::vec3(2), 0, 0.001f, -1.f);
	lights.push_back(light1);
	player->meshTransform->addChild(light1);
	light1->firstParent()->translate(0, 0, 10);
	
	SpriteSheet * buttonSpriteSheet = new SpriteSheet(MY_ResourceManager::scenario->getTexture("BUTTONS")->texture);
	buttonSpriteSheet->addAnimation("X_UP", 0, 0, 16, 16, 1);
	buttonSpriteSheet->addAnimation("X_DOWN", 1, 1, 16, 16, 1);
	buttonSpriteSheet->addAnimation("Z_UP", 2, 2, 16, 16, 1);
	buttonSpriteSheet->addAnimation("Z_DOWN", 3, 3, 16, 16, 1);

	xButton = new Sprite(uiLayer.shader);
	zButton = new Sprite(uiLayer.shader);
	xButton->setPrimaryTexture(MY_ResourceManager::scenario->getTextureSampler("BUTTON-BASE")->textureSampler);
	zButton->setPrimaryTexture(MY_ResourceManager::scenario->getTextureSampler("BUTTON-BASE")->textureSampler);
	xButton->childTransform->scale(16);
	zButton->childTransform->scale(16);
	xButton->setSpriteSheet(buttonSpriteSheet, "X_UP");
	zButton->setSpriteSheet(buttonSpriteSheet, "Z_UP");
	xButton->mesh->setScaleMode(GL_NEAREST);
	zButton->mesh->setScaleMode(GL_NEAREST);

	NodeUI * l = new NodeUI(uiLayer.world);
	l->setRationalHeight(1.f);
	l->setRationalWidth(1.f);
	l->background->setVisible(false);

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


	uiLayer.addChild(l);
	l->addChild(hl);
	hl->addChild(zui);
	hl->addChild(pad);
	hl->addChild(xui);
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

	if(keyboard->keyJustDown(GLFW_KEY_1)){
		cycleCamera();
	}
	
	if (keyboard->keyJustDown(GLFW_KEY_2)){
		box2dDebug->drawing = Transform::drawTransforms = !Transform::drawTransforms;
		if(Transform::drawTransforms){
			uiLayer.bulletDebugDrawer->setDebugMode(BulletDebugDrawer::DBG_MAX_DEBUG_DRAW_MODE);
		}else{
			uiLayer.bulletDebugDrawer->setDebugMode(BulletDebugDrawer::DBG_NoDebug);
		}
	}

	float speed = 1;
	MousePerspectiveCamera * cam = dynamic_cast<MousePerspectiveCamera *>(activeCamera);
	if(cam != nullptr){
		speed = cam->speed;
	}
	// camera controls
	if (keyboard->keyDown(GLFW_KEY_UP)){
		activeCamera->parents.at(0)->translate((activeCamera->forwardVectorRotated) * speed);
	}
	if (keyboard->keyDown(GLFW_KEY_DOWN)){
		activeCamera->parents.at(0)->translate((activeCamera->forwardVectorRotated) * -speed);
	}
	if (keyboard->keyDown(GLFW_KEY_LEFT)){
		activeCamera->parents.at(0)->translate((activeCamera->rightVectorRotated) * -speed);
	}
	if (keyboard->keyDown(GLFW_KEY_RIGHT)){
		activeCamera->parents.at(0)->translate((activeCamera->rightVectorRotated) * speed);
	}

	if(keyboard->keyJustUp(GLFW_KEY_3)){
		sweet::toggleAntTweakBar();
	}

	// player controls
	glm::vec2 f(0);
	float m = player->body->GetMass();
	/*if (keyboard->keyJustDown(GLFW_KEY_W)){
		f.y += 10;
	}
	if (keyboard->keyJustDown(GLFW_KEY_S)){
		playerSize += 1;
		if(playerSize > NUM_SIZES){
			playerSize = 1;
		}
	}
	if (keyboard->keyDown(GLFW_KEY_A)){
		f.x -= 1;
	}
	if (keyboard->keyDown(GLFW_KEY_D)){
		f.x += 1;
	}*/


	if(keyboard->keyDown(GLFW_KEY_X)){
		moving = true;
		xButton->setCurrentAnimation("X_DOWN");
	}else{
		moving = false;	
		xButton->setCurrentAnimation("X_UP");
	}if(keyboard->keyDown(GLFW_KEY_Z)){
		giveUp = true;
		zButton->setCurrentAnimation("Z_DOWN");
	}else{
		zButton->setCurrentAnimation("Z_UP");
	}

	
	if (keyboard->keyJustDown(GLFW_KEY_Z)){
		playerSize += 1;
		if(playerSize > NUM_SIZES){
			playerSize = 1;
		}
	}

	
	std::stringstream anim;
	if(moving){
		player->body->SetLinearVelocity(b2Vec2(playerSize, 0));
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