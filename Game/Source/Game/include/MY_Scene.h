#pragma once

#include <Scene.h>
#include <UILayer.h>
#include <Flag.h>

class PerspectiveCamera;
class MousePerspectiveCamera;

class Box2DWorld;
class Box2DDebugDrawer;
class Box2DMeshEntity;
class Box2DSprite;
class MeshEntity;

class ShaderComponentHsv;

class Shader;
class RenderSurface;
class StandardFrameBuffer;
class Material;
class Sprite;

class PointLight;

class ComponentShaderText;

#define NUM_SIZES 11
#define DISTANCE 200

class MY_Scene : public Scene{
public:
	Shader * screenSurfaceShader;
	RenderSurface * screenSurface;
	StandardFrameBuffer * screenFBO;
	
	ComponentShaderBase * baseShader;
	ComponentShaderBase * worldspaceShader;
	
	OrthographicCamera * playerCam;
	
	PointLight * light;
	bool moving, giveUp;

	Box2DWorld * box2dWorld;
	Box2DDebugDrawer * box2dDebug;

	virtual void update(Step * _step) override;
	virtual void render(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions) override;
	
	virtual void load() override;
	virtual void unload() override;

	UILayer uiLayer;
	Sprite * xButton;
	Sprite * zButton;
	Flag * flag;
	NodeUI * images;

	MY_Scene(Game * _game);
	~MY_Scene();


	unsigned long int playerSize;
	bool lowered;
	Box2DSprite * player;
	Box2DSprite * ground;
};