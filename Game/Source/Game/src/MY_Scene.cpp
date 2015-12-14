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
#include <shader\ShaderComponentMVP.h>

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
	characterShader(new ComponentShaderBase(true)),
	textShader(new ComponentShaderText(true)),
	debugDrawer(nullptr),
	uiLayer(0,0,0,0),
	box2dWorld(new Box2DWorld(b2Vec2(0.f, -10.0f))),
	box2dDebug(new Box2DDebugDrawer(box2dWorld))
{
	baseShader->addComponent(new ShaderComponentMVP(baseShader));
	//baseShader->addComponent(new ShaderComponentDiffuse(baseShader));
	baseShader->addComponent(new ShaderComponentTexture(baseShader));
	baseShader->compileShader();

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

	playerCam = new OrthographicCamera(-192/3, 192/3, -108/3, 108/3, -100, 100);
	childTransform->addChild(playerCam);
	
	cameras.push_back(playerCam);

	activeCamera = debugCam;

	childTransform->addChild(box2dDebug, false);
	box2dDebug->drawing = true;
	box2dWorld->b2world->SetDebugDraw(box2dDebug);
	box2dDebug->AppendFlags(b2Draw::e_shapeBit);
	box2dDebug->AppendFlags(b2Draw::e_centerOfMassBit);
	box2dDebug->AppendFlags(b2Draw::e_jointBit);

	uiLayer.addMouseIndicator();

	SpriteSheet * dudeSpriteSheet = new SpriteSheet(MY_ResourceManager::scenario->getTexture("DUDE-ANIMATION")->texture);
	for(unsigned long int i = 0; i < NUM_SIZES; ++i){
		std::stringstream ss;
		ss << "IDLE_" << (i+1);
		dudeSpriteSheet->addAnimation(ss.str(), i*4, i*4, 16, 16, 1);
		ss.str(""); // clear
		ss << "RUN_" << (i+1);
		dudeSpriteSheet->addAnimation(ss.str(), i*4, i*4+3, 16, 16, 0.16f);
	}

	player = new Box2DSprite(box2dWorld, MY_ResourceManager::scenario->getTextureSampler("DUDE")->textureSampler, b2_dynamicBody, baseShader);
	player->createFixture();
	player->mesh->scaleModeMag = player->mesh->scaleModeMin = GL_NEAREST;
	childTransform->addChild(player);
	player->setSpriteSheet(dudeSpriteSheet, "IDLE_1");

	ground = new Box2DSprite(box2dWorld, MY_ResourceManager::scenario->getTextureSampler("DEFAULT")->textureSampler, b2_staticBody, baseShader);
	ground->createFixture();
	childTransform->addChild(ground);
	ground->setTranslationPhysical(0, -ground->getCorrectedHeight()*0.5f, 0);

	playerSize = 1;
}

MY_Scene::~MY_Scene(){
	deleteChildTransform();
	baseShader->safeDelete();
	characterShader->safeDelete();
	textShader->safeDelete();

	screenSurface->safeDelete();
	//screenSurfaceShader->safeDelete();
	screenFBO->safeDelete();
}


void MY_Scene::update(Step * _step){
	glm::uvec2 sd = sweet::getScreenDimensions();

	glm::vec3 playerPos = player->meshTransform->getWorldPos();
	glm::vec3 groundPos = ground->meshTransform->getWorldPos();
	ground->setTranslationPhysical(playerPos.x, groundPos.y, groundPos.z);
	box2dWorld->update(_step);


	glm::vec3 camPos = playerCam->getWorldPos();
	playerCam->firstParent()->translate(camPos.x + (playerPos.x - camPos.x)*0.1f, camPos.y + (playerPos.y - camPos.y)*0.1f, camPos.z, false);
	
	if(keyboard->keyJustDown(GLFW_KEY_F12)){
		game->toggleFullScreen();
	}

	if(keyboard->keyJustDown(GLFW_KEY_1)){
		cycleCamera();
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





	// player controls
	glm::vec2 f(0);
	float m = player->body->GetMass();
	if (keyboard->keyJustDown(GLFW_KEY_W)){
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
	}
	player->applyLinearImpulseToCenter(m*f.x, m*f.y);
	
	std::stringstream anim;
	if(f.x < 0){
		player->childTransform->scale(-1, 1, 1, false);
		anim << "RUN_";
	}else if(f.x > 0){
		player->childTransform->scale(1, 1, 1, false);
		anim << "RUN_";
	}else{
		anim << "IDLE_";
	}
	anim << playerSize;
	player->setCurrentAnimation(anim.str());

	uiLayer.resize(0, sd.x, 0, sd.y);
	Scene::update(_step);
	uiLayer.update(_step);
}

void MY_Scene::render(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions){
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