#include "scene_app.h"
#include <system/platform.h>
#include <graphics/sprite_renderer.h>
#include <graphics/font.h>
#include <system/debug_log.h>
#include <graphics/renderer_3d.h>
#include <graphics/mesh.h>
#include <maths/math_utils.h>
#include <input/sony_controller_input_manager.h>
#include <graphics/sprite.h>
#include "load_texture.h"

SceneApp::SceneApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	renderer_3d_(NULL),
	primitive_builder_(NULL),
	input_manager_(NULL),
	font_(NULL),
	world_(NULL),
	player_body_(NULL),
	crossButton(NULL),
	squareButton(NULL),
	circleButton(NULL),
	triangleButton(NULL)
{
}


void SceneApp::Init()
{
	sprite_renderer_ = gef::SpriteRenderer::Create(platform_);
	InitFont();

	// initialise input manager
	input_manager_ = gef::InputManager::Create(platform_);

	gameState = INIT;
	IntervalInit();
}

void SceneApp::CleanUp()
{
	delete input_manager_;
	input_manager_ = NULL;

	CleanUpFont();

	delete sprite_renderer_;
	sprite_renderer_ = NULL;
}

bool SceneApp::Update(float frame_time)
{
	fps_ = 1.0f / frame_time;


	input_manager_->Update();

	switch (gameState)
	{
	case INIT:
		IntervalUpdate(frame_time);
		break;
	case MENU:
		FrontendUpdate(frame_time);
		break;
	case OPTIONS:
		OptionsUpdate(frame_time);
		break;
	case CREDITS:
		CreditsUpdate(frame_time);
		break;
	case LEVEL1:
		GameUpdate(frame_time);
		break;
	case LEVEL2:
		GameUpdate(frame_time);
		break;
	case LOSE:
		IntervalUpdate(frame_time);
		break;
	case WIN:
		IntervalUpdate(frame_time);
		break;
	case EXIT:
		return IntervalUpdate(frame_time);
		break;
	default:
		break;
	}

	return true;
}

void SceneApp::Render()
{
	switch (gameState)
	{
	case SceneApp::INIT:
		IntervalRender();
		break;
	case SceneApp::MENU:
		FrontendRender();
		break;
	case SceneApp::OPTIONS:
		OptionsRender();
		break;
	case SceneApp::CREDITS:
		CreditsRender();
		break;
	case SceneApp::LEVEL1:
		GameRender();
		break;
	case SceneApp::LEVEL2:
		GameRender();
		break;
	case SceneApp::LOSE:
		IntervalRender();
		break;
	case SceneApp::WIN:
		IntervalRender();
		break;
	case SceneApp::EXIT:
		IntervalRender();
		break;
	default:
		break;
	}
}

void SceneApp::InitPlayer()
{
	// setup the mesh for the player
	player_.set_mesh(primitive_builder_->GetDefaultSphereMesh());

	// create a physics body for the player
	b2BodyDef player_body_def;
	player_body_def.type = b2_dynamicBody;
	player_body_def.position = b2Vec2(0.0f, 4.0f);

	player_body_ = world_->CreateBody(&player_body_def);

	// create the shape for the player
	b2CircleShape player_shape;
	player_shape.m_radius = 0.5f;

	// create the fixture
	b2FixtureDef player_fixture_def;
	player_fixture_def.shape = &player_shape;
	player_fixture_def.density = 1.0f;
	player_fixture_def.restitution = 0.8f;
	player_fixture_def.friction = 0.2f;

	// create the fixture on the rigid body
	player_body_->CreateFixture(&player_fixture_def);

	// update visuals from simulation data
	player_.UpdateFromSimulation(player_body_);

	// create a connection between the rigid body and GameObject
	player_body_->SetUserData(&player_);
}

void SceneApp::InitGround()
{
	// ground dimensions
	gef::Vector4 ground_half_dimensions(5.0f, 0.5f, 0.5f);

	// setup the mesh for the ground
	ground_mesh_ = primitive_builder_->CreateBoxMesh(ground_half_dimensions);
	ground_.set_mesh(ground_mesh_);

	// create a physics body
	b2BodyDef body_def;
	body_def.type = b2_staticBody;
	body_def.position = b2Vec2(0.0f, 0.0f);
	body_def.angle = 0.8f;

	ground_body_ = world_->CreateBody(&body_def);

	// create the shape
	b2PolygonShape shape;
	shape.SetAsBox(ground_half_dimensions.x(), ground_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;

	// create the fixture on the rigid body
	ground_body_->CreateFixture(&fixture_def);

	// update visuals from simulation data
	ground_.UpdateFromSimulation(ground_body_);
}


void SceneApp::InitFont()
{
	font_ = new gef::Font(platform_);
	font_->Load("comic_sans");
}

void SceneApp::CleanUpFont()
{
	delete font_;
	font_ = NULL;
}

void SceneApp::DrawHUD()
{
	if(font_)
	{
		// display frame rate
		font_->RenderText(sprite_renderer_, gef::Vector4(850.0f, 510.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "FPS: %.1f", fps_);
	}
}

void SceneApp::SetupLights()
{
	// grab the data for the default shader used for rendering 3D geometry
	gef::Default3DShaderData& default_shader_data = renderer_3d_->default_shader_data();

	// set the ambient light
	default_shader_data.set_ambient_light_colour(gef::Colour(0.25f, 0.25f, 0.25f, 1.0f));

	// add a point light that is almost white, but with a blue tinge
	// the position of the light is set far away so it acts light a directional light
	gef::PointLight default_point_light;
	default_point_light.set_colour(gef::Colour(0.7f, 0.7f, 1.0f, 1.0f));
	default_point_light.set_position(gef::Vector4(-500.0f, 400.0f, 700.0f));
	default_shader_data.AddPointLight(default_point_light);
}

void SceneApp::UpdateSimulation(float frame_time)
{
	// update physics world
	float timeStep = 1.0f / 60.0f;

	int32 velocityIterations = 6;
	int32 positionIterations = 2;

	world_->Step(timeStep, velocityIterations, positionIterations);

	// update object visuals from simulation data
	player_.UpdateFromSimulation(player_body_);

	// don't have to update the ground visuals as it is static

	// collision detection
	// get the head of the contact list
	b2Contact* contact = world_->GetContactList();
	// get contact count
	int contact_count = world_->GetContactCount();

	for (int contact_num = 0; contact_num<contact_count; ++contact_num)
	{
		if (contact->IsTouching())
		{
			// get the colliding bodies
			b2Body* bodyA = contact->GetFixtureA()->GetBody();
			b2Body* bodyB = contact->GetFixtureB()->GetBody();

			// DO COLLISION RESPONSE HERE
			Player* player = NULL;

			GameObject* gameObjectA = NULL;
			GameObject* gameObjectB = NULL;

			gameObjectA = (GameObject*)bodyA->GetUserData();
			gameObjectB = (GameObject*)bodyB->GetUserData();

			if (gameObjectA)
			{
				if (gameObjectA->type() == PLAYER)
				{
					player = (Player*)bodyA->GetUserData();
				}
			}

			if (gameObjectB)
			{
				if (gameObjectB->type() == PLAYER)
				{
					player = (Player*)bodyB->GetUserData();
				}
			}

			if (player)
			{
				player->DecrementHealth();
			}
		}

		// Get next contact point
		contact = contact->GetNext();
	}
}

void SceneApp::FrontendInit()
{
	crossButton = CreateTextureFromPNG("playstation-cross-dark-icon.png", platform_);
	squareButton = CreateTextureFromPNG("playstation-square-dark-icon.png", platform_);
	circleButton = CreateTextureFromPNG("playstation-circle-dark-icon.png", platform_);
	triangleButton = CreateTextureFromPNG("playstation-triangle-dark-icon.png", platform_);
}

void SceneApp::FrontendRelease()
{
	delete crossButton;
	crossButton = NULL;
}

void SceneApp::FrontendUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);
	switch (controller->buttons_pressed())
	{
	case (1 << 12):
		FrontendRelease();
		gameState = LEVEL1;
		GameInit();
		break;
	case (1<<13):
		FrontendRelease();
		gameState = EXIT;
		IntervalInit();
		break;
	case (1<<14):
		gameState = OPTIONS;
		OptionsInit();
		break;
	case (1<<15):
		gameState = CREDITS;
		CreditsInit();
		break;
	default:
		break;
	}
}

void SceneApp::FrontendRender()
{
	sprite_renderer_->Begin();

	// render "PRESS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"PRESS");

	// Render buttons
	gef::Sprite button;
	button.set_texture(triangleButton);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	button.set_texture(crossButton);
	button.set_position(gef::Vector4(platform_.width()*0.05f, platform_.height()*0.1f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	button.set_texture(squareButton);
	button.set_position(gef::Vector4(platform_.width()*0.95f, platform_.height()*0.1f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	button.set_texture(circleButton);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.85f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	// render "TO START" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f + 32.0f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"TO START");

	// render "OPTIONS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.05f + 82.f, platform_.height()*0.1f - 16.f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"OPTIONS");

	// render "CREDITS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.95f - 80.f, platform_.height()*0.1f - 16.f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"CREDITS");

	// render "EXIT" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.85f + 32.f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"EXIT");
	
	DrawHUD();
	sprite_renderer_->End();
}

void SceneApp::GameInit()
{
	// create the renderer for draw 3D geometry
	renderer_3d_ = gef::Renderer3D::Create(platform_);

	// initialise primitive builder to make create some 3D geometry easier
	primitive_builder_ = new PrimitiveBuilder(platform_);


	SetupLights();

	// initialise the physics world
	b2Vec2 gravity(0.0f, -9.81f);
	world_ = new b2World(gravity);

	InitPlayer();
	InitGround();
}

void SceneApp::GameRelease()
{
	// destroying the physics world also destroys all the objects within it
	delete world_;
	world_ = NULL;

	delete ground_mesh_;
	ground_mesh_ = NULL;

	delete primitive_builder_;
	primitive_builder_ = NULL;

	delete renderer_3d_;
	renderer_3d_ = NULL;

}

void SceneApp::GameUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);
	switch (controller->buttons_pressed())
	{
	case (1 << 14):
		if (gameState == LEVEL1)
		{
			gameState = LEVEL2;
			GameInit();
		}
		else
		{
			GameRelease();
			gameState = WIN;
			IntervalInit();
		}
		return;
		break;
	case (1 << 15):
		GameRelease();
		gameState = LOSE;
		IntervalInit();
		return;
		break;
	default:
		break;
	}

	UpdateSimulation(frame_time);
}

void SceneApp::GameRender()
{
	// setup camera

	// projection
	float fov = gef::DegToRad(45.0f);
	float aspect_ratio = (float)platform_.width() / (float)platform_.height();
	gef::Matrix44 projection_matrix;
	projection_matrix = platform_.PerspectiveProjectionFov(fov, aspect_ratio, 0.1f, 100.0f);
	renderer_3d_->set_projection_matrix(projection_matrix);

	// view
	gef::Vector4 camera_eye(-2.0f, 2.0f, 10.0f);
	gef::Vector4 camera_lookat(0.0f, 0.0f, 0.0f);
	gef::Vector4 camera_up(0.0f, 1.0f, 0.0f);
	gef::Matrix44 view_matrix;
	view_matrix.LookAt(camera_eye, camera_lookat, camera_up);
	renderer_3d_->set_view_matrix(view_matrix);


	// draw 3d geometry
	renderer_3d_->Begin();

	// draw ground
	renderer_3d_->DrawMesh(ground_);

	// draw player
	renderer_3d_->set_override_material(&primitive_builder_->red_material());
	renderer_3d_->DrawMesh(player_);
	renderer_3d_->set_override_material(NULL);

	renderer_3d_->End();

	// start drawing sprites, but don't clear the frame buffer
	sprite_renderer_->Begin(false);

	if (gameState == LEVEL1)
	{

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.7f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Level 1");
	}
	else
	{

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.7f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Level 2");
	}

	DrawHUD();
	sprite_renderer_->End();
}

void SceneApp::IntervalInit()
{
	timer = 0;
}

void SceneApp::IntervalRelease()
{
	timer = NULL;
}

bool SceneApp::IntervalUpdate(float frame_time)
{
	timer += frame_time;

	if (timer > 1)
	{
		if (gameState == EXIT)
		{
			IntervalRelease();
			return false;
		}
		else
		{
			IntervalRelease();
			gameState = MENU;
			FrontendInit();
		}
	}

	return true;
}

void SceneApp::IntervalRender()
{
	sprite_renderer_->Begin();

	// render text
	switch (gameState)
	{
	case SceneApp::INIT:
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Initialising Game");
		break;
	case SceneApp::LOSE:
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"You Lost!");
		break;
	case SceneApp::WIN:
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"You Won!");
		break;
	case SceneApp::EXIT:
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Exiting Game");
		break;
	default:
		break;
	}
	
	DrawHUD();
	sprite_renderer_->End();
}

void SceneApp::OptionsInit()
{
}

void SceneApp::OptionsRelease()
{
}

void SceneApp::OptionsUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);
	switch (controller->buttons_pressed())
	{
	case (1 << 14):
		gameState = MENU;
		FrontendInit();
		break;
	default:
		break;
	}
}

void SceneApp::OptionsRender()
{
	sprite_renderer_->Begin();

	// render "OPTIONS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"OPTIONS");

	// Render buttons
	gef::Sprite button;
	button.set_texture(crossButton);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	// render "TO MENU" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f + 32.0f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"TO MENU");

	sprite_renderer_->End();
}

void SceneApp::CreditsInit()
{
}

void SceneApp::CreditsRelease()
{
}

void SceneApp::CreditsUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);
	switch (controller->buttons_pressed())
	{
	case (1 << 14):
		gameState = MENU;
		FrontendInit();
		break;
	default:
		break;
	}
}

void SceneApp::CreditsRender()
{
	sprite_renderer_->Begin();

	// render "CREDITS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"CREDITS");

	// Render buttons
	gef::Sprite button;
	button.set_texture(crossButton);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	// render "TO MENU" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f + 32.0f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"TO MENU");

	sprite_renderer_->End();
}
