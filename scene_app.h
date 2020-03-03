#ifndef _SCENE_APP_H
#define _SCENE_APP_H

#include <system/application.h>
#include <maths/vector2.h>
#include "primitive_builder.h"
#include <graphics/mesh_instance.h>
#include <input/input_manager.h>
#include <box2d/Box2D.h>
#include "game_object.h"


// FRAMEWORK FORWARD DECLARATIONS
namespace gef
{
	class Platform;
	class SpriteRenderer;
	class Font;
	class InputManager;
	class Renderer3D;
}

class SceneApp : public gef::Application
{
public:
	SceneApp(gef::Platform& platform);
	void Init();
	void CleanUp();
	bool Update(float frame_time);
	void Render();
private:
	void InitPlayer();
	void InitGround();
	void InitFont();
	void CleanUpFont();
	void DrawHUD();
	void SetupLights();
	void UpdateSimulation(float frame_time);
    
	gef::SpriteRenderer* sprite_renderer_;
	gef::Font* font_;
	gef::InputManager* input_manager_;

	enum GAMESTATE { INIT, MENU, OPTIONS, CREDITS, LEVEL1, LEVEL2, LOSE, WIN, EXIT };
	GAMESTATE gameState;

	//
	// FRONTEND DECLARATIONS
	//
	gef::Texture* crossButton;
	gef::Texture* squareButton;
	gef::Texture* circleButton;
	gef::Texture* triangleButton;

	//
	// INTERVAL DECLARATIONS
	//
	float timer;

	//
	// GAME DECLARATIONS
	//
	gef::Renderer3D* renderer_3d_;
	PrimitiveBuilder* primitive_builder_;

	// create the physics world
	b2World* world_;

	// player variables
	Player player_;
	b2Body* player_body_;

	// ground variables
	gef::Mesh* ground_mesh_;
	GameObject ground_;
	b2Body* ground_body_;

	// audio variables
	int sfx_id_;
	int sfx_voice_id_;

	float fps_;

	void FrontendInit();
	void FrontendRelease();
	void FrontendUpdate(float frame_time);
	void FrontendRender();

	void GameInit();
	void GameRelease();
	void GameUpdate(float frame_time);
	void GameRender();

	void IntervalInit();
	void IntervalRelease();
	bool IntervalUpdate(float frame_time);
	void IntervalRender();

	void OptionsInit();
	void OptionsRelease();
	void OptionsUpdate(float frame_time);
	void OptionsRender();

	void CreditsInit();
	void CreditsRelease();
	void CreditsUpdate(float frame_time);
	void CreditsRender();
};

#endif // _SCENE_APP_H
