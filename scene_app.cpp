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
	crossButton(NULL),
	squareButton(NULL),
	circleButton(NULL),
	triangleButton(NULL)
{
	lives = 3;
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
	case INGAME:
		GameUpdate(frame_time);
		break;
	case PAUSE:
		GameUpdate(frame_time);
		break;
	case GAMEOVER:
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
	case SceneApp::INGAME:
		GameRender();
		break;
	case SceneApp::PAUSE:
		GameRender();
		break;
	case SceneApp::GAMEOVER:
		IntervalRender();
		break;
	case SceneApp::EXIT:
		IntervalRender();
		break;
	default:
		break;
	}
}

void SceneApp::InitBall()
{
	// setup the mesh for the ball
	ball_vec_.push_back(new Ball);
	ball_vec_[0]->set_mesh(primitive_builder_->GetDefaultSphereMesh());

	// create a physics body for the ball
	b2BodyDef ball_body_def;
	ball_body_def.type = b2_dynamicBody;
	ball_body_def.position = b2Vec2(-2.0f, 4.0f);

	ball_body_vec_.push_back(world_->CreateBody(&ball_body_def));

	// create the shape for the ball
	b2CircleShape ball_shape;
	ball_shape.m_radius = 0.5f;

	// create the fixture
	b2FixtureDef ball_fixture_def;
	ball_fixture_def.shape = &ball_shape;
	ball_fixture_def.density = 0.7f;
	ball_fixture_def.restitution = 0.5f;
	ball_fixture_def.friction = 0.2f;

	// create the fixture on the rigid body
	ball_body_vec_[0]->CreateFixture(&ball_fixture_def);

	// update visuals from simulation data
	ball_vec_[0]->UpdateFromSimulation(ball_body_vec_[0]);

	// create a connection between the rigid body and GameObject
	ball_body_vec_[0]->SetUserData(&ball_vec_[0]);
}

void SceneApp::InitBoard()
{
	board_.set_type(BOARD);
	// load the assets in from the .scn
	const char* scene_asset_filename = "pinballFrame.scn";
	scene_assets_ = LoadSceneAssets(platform_, scene_asset_filename);
	if (scene_assets_)
	{
		board_.set_mesh(GetMeshFromSceneAssets(scene_assets_));
		gef::DebugOut("Scene file loaded!\n");
	}
	else
	{
		gef::DebugOut("Scene file %s failed to load\n", scene_asset_filename);
	}

	// create a physics body
	b2BodyDef body_def;
	body_def.type = b2_staticBody;

	board_body_ = world_->CreateBody(&body_def);

	// create the shape
	b2Vec2 frameVertices[19];
	frameVertices[0].Set(8.227069, -29.047207);
	frameVertices[1].Set(8.230977, 10.862797);
	frameVertices[2].Set(8.072821, 12.521641);
	frameVertices[3].Set(7.604436, 14.116737);
	frameVertices[4].Set(6.843820, 15.586790);
	frameVertices[5].Set(5.820206, 16.875298);
	frameVertices[6].Set(4.572926, 17.932747);
	frameVertices[7].Set(3.149917, 18.718502);
	frameVertices[8].Set(1.605863, 19.202366);
	frameVertices[9].Set(0.000113, 19.365749);
	frameVertices[10].Set(-1.605649, 19.202366);
	frameVertices[11].Set(-3.149703, 18.718498);
	frameVertices[12].Set(-4.572711, 17.932739);
	frameVertices[13].Set(-5.819987, 16.875290);
	frameVertices[14].Set(-6.843601, 15.586779);
	frameVertices[15].Set(-7.604213, 14.116732);
	frameVertices[16].Set(-8.072598, 12.521635);
	frameVertices[17].Set(-8.230750, 10.862789);
	frameVertices[18].Set(-8.228932, -29.047207);

	b2ChainShape shape;
	shape.CreateChain(frameVertices, 19);

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	
	// create the fixture on the rigid body
	board_body_->CreateFixture(&fixture_def);
	// update visuals from simulation data
	board_.UpdateFromSimulation(board_body_);

	// create a connection between the rigid body and GameObject
	board_body_->SetUserData(&board_);
}

void SceneApp::InitBarriers()
{
	int barrierCount = 4;

	// barrier dimensions
	gef::Vector4 barrier_half_dimensions(0.4f, 0.5f, 1.0f);
	// setup the mesh for the barrier
	barrier_mesh_ = primitive_builder_->CreateBoxMesh(barrier_half_dimensions);

	for (int i = 0; i < barrierCount; i++)
	{
		barrier_vec_.push_back(new Barrier);
		barrier_vec_[i]->set_mesh(barrier_mesh_);
	}

	// create a physics body for the barrier
	b2BodyDef barrier_body_def;
	barrier_body_def.type = b2_kinematicBody;

	for (int i = 0; i < barrierCount; i++)
	{
		barrier_body_def.position = b2Vec2(-2.0f + i*0.5f, 4.0f);
		barrier_body_vec_.push_back(world_->CreateBody(&barrier_body_def));
	}


	// create the shape for the barrier
	b2PolygonShape shape;
	shape.SetAsBox(barrier_half_dimensions.x(), barrier_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.density = 1.0f;

	for (int i = 0; i < 2; i++)
	{
		// create the fixture on the rigid body
		barrier_body_vec_[i]->CreateFixture(&fixture_def);

		// update visuals from simulation data
		barrier_vec_[i]->UpdateFromSimulation(barrier_body_vec_[i]);

		// create a connection between the rigid body and GameObject
		barrier_body_vec_[i]->SetUserData(&barrier_vec_[i]);
	}
}

void SceneApp::InitFlippers()
{
	// flipper dimensions
	gef::Vector4 flipper_half_dimensions(3.0f, 0.5f, 0.5f);
	// setup the mesh for the flipper
	flipper_mesh_ = primitive_builder_->CreateBoxMesh(flipper_half_dimensions);

	flipper_vec_.push_back(new Flipper);
	flipper_vec_[0]->set_mesh(flipper_mesh_);
	flipper_vec_[0]->set_left(true);

	flipper_vec_.push_back(new Flipper);
	flipper_vec_[1]->set_mesh(flipper_mesh_);
	flipper_vec_[1]->set_left(false);

	// create a physics body
	b2BodyDef flipper_def;
	flipper_def.type = b2_dynamicBody;
	b2BodyDef flipper_pin_def;
	flipper_pin_def.type = b2_staticBody;

	// create physics joint
	b2RevoluteJointDef flipper_joint_def;
	flipper_joint_def.localAnchorA.Set(0, 0);
	flipper_joint_def.collideConnected = false;
	flipper_joint_def.enableLimit = true;
	flipper_joint_def.enableMotor = true;
	flipper_joint_def.maxMotorTorque = 1000;

	// flipper one

	flipper_def.position = b2Vec2(-3.f, -7.0f);
	flipper_body_vec_.push_back(world_->CreateBody(&flipper_def));

	flipper_pin_def.position = flipper_def.position - b2Vec2(2.5f, 0);
	flipper_pin_body_vec_.push_back(world_->CreateBody(&flipper_pin_def));

	flipper_joint_def.bodyA = flipper_pin_body_vec_[0];
	flipper_joint_def.bodyB = flipper_body_vec_[0];
	flipper_joint_def.localAnchorB.Set(-1.75f, 0);
	flipper_joint_def.lowerAngle = gef::DegToRad(-30.f);
	flipper_joint_def.upperAngle = gef::DegToRad(30.f);
	flipper_joint_def.motorSpeed = -500.f;
	flipper_joint_vec_.push_back((b2RevoluteJoint*)world_->CreateJoint(&flipper_joint_def));

	// flipper two

	flipper_def.position = b2Vec2(3.f, -7.0f);
	flipper_body_vec_.push_back(world_->CreateBody(&flipper_def));

	flipper_pin_def.position = flipper_def.position + b2Vec2(2.5f, 0);
	flipper_pin_body_vec_.push_back(world_->CreateBody(&flipper_pin_def));

	flipper_joint_def.bodyA= flipper_pin_body_vec_[1];
	flipper_joint_def.bodyB = flipper_body_vec_[1];
	flipper_joint_def.localAnchorB.Set(1.75f, 0);
	flipper_joint_def.lowerAngle = gef::DegToRad(-30.f);
	flipper_joint_def.upperAngle = gef::DegToRad(30.f);
	flipper_joint_def.motorSpeed = 500.f;
	flipper_joint_vec_.push_back((b2RevoluteJoint*)world_->CreateJoint(&flipper_joint_def));

	// create the shape
	b2PolygonShape shape;
	shape.SetAsBox(flipper_half_dimensions.x(), flipper_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.density = 1.0f;
	fixture_def.restitution = 0.f;

	for (int i = 0; i < 2; i++)
	{
		// create the fixture on the rigid body
		flipper_body_vec_[i]->CreateFixture(&fixture_def);

		// update visuals from simulation data
		flipper_vec_[i]->UpdateFromSimulation(flipper_body_vec_[i]);

		// create a connection between the rigid body and GameObject
		flipper_body_vec_[i]->SetUserData(&flipper_vec_[i]);
	}

}

void SceneApp::InitLoseTrigger()
{
	lose_trigger_.set_type(LOSETRIGGER);
	// kill trigger dimensions
	gef::Vector4 lt_half_dimensions(10.0f, 0.2f, 0.5f);

	// setup the mesh for the board
	lose_trigger_mesh_ = primitive_builder_->CreateBoxMesh(lt_half_dimensions);
	lose_trigger_.set_mesh(lose_trigger_mesh_);

	// create a physics body
	b2BodyDef body_def;
	body_def.type = b2_staticBody;
	body_def.position = b2Vec2(0.0f, -15.0f);

	lose_trigger_body_ = world_->CreateBody(&body_def);

	// create the shape
	b2PolygonShape shape;
	shape.SetAsBox(lt_half_dimensions.x(), lt_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;

	// create the fixture on the rigid body
	lose_trigger_body_->CreateFixture(&fixture_def);

	// update visuals from simulation data
	lose_trigger_.UpdateFromSimulation(lose_trigger_body_);

	// create a connection between the rigid body and GameObject
	lose_trigger_body_->SetUserData(&lose_trigger_);
}

gef::Scene* SceneApp::LoadSceneAssets(gef::Platform& platform, const char* filename)
{
	gef::Scene* scene = new gef::Scene();

	if (scene->ReadSceneFromFile(platform, filename))
	{
		// if scene file loads successful
		// create material and mesh resources from the scene data
		scene->CreateMaterials(platform);
		scene->CreateMeshes(platform);
	}
	else
	{
		delete scene;
		scene = NULL;
	}

	return scene;
}

gef::Mesh* SceneApp::GetMeshFromSceneAssets(gef::Scene* scene)
{
	gef::Mesh* mesh = NULL;

	// if the scene data contains at least one mesh
	// return the first mesh
	if (scene && scene->meshes.size() > 0)
		mesh = scene->meshes.front();

	return mesh;
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
		font_->RenderText(sprite_renderer_, gef::Vector4(700.f, 10.f, -0.9f), 1.0f, 0xffffffff, gef::TJ_RIGHT, "FPS: %.1f", fps_);		
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
	for (int ballCount = 0; ballCount < ball_vec_.size(); ballCount++)
	{
		ball_vec_[ballCount]->UpdateFromSimulation(ball_body_vec_[ballCount]);
	}

	for (int flipperCount = 0; flipperCount < flipper_vec_.size(); flipperCount++)
	{
		flipper_vec_[flipperCount]->UpdateFromSimulation(flipper_body_vec_[flipperCount]);
	}

	// don't have to update the board visuals as it is static

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
			Ball* ball = NULL;

			GameObject* gameObjectA = NULL;
			GameObject* gameObjectB = NULL;

			gameObjectA = (GameObject*)bodyA->GetUserData();
			gameObjectB = (GameObject*)bodyB->GetUserData();

			/*if (gameObjectA)
			{
				if (gameObjectA->type() == BALL)
				{
					ball = (Ball*)bodyA->GetUserData();
				}
			}

			if (gameObjectB)
			{
				if (gameObjectB->type() == BALL)
				{
					ball = (Ball*)bodyB->GetUserData();
				}
			}*/

			if (gameObjectA != NULL && gameObjectB != NULL)
			{
				if (gameObjectA->type() == LOSETRIGGER || gameObjectB->type() == LOSETRIGGER)
				{
					lives--;
				}
				else if (gameObjectA->type() == BALL && gameObjectB->type() == BOARD)
				{
					gef::Matrix44 ball_transform = gameObjectA->transform();
					gef::Vector4 ball_pos = ball_transform.GetTranslation();
					gef::DebugOut("Collision Location: %f, %f", ball_pos.x(), ball_pos.y());
				}
				else if (gameObjectA->type() == BOARD && gameObjectB->type() == BALL)
				{
					gef::Matrix44 ball_transform = gameObjectB->transform();
					gef::Vector4 ball_pos = ball_transform.GetTranslation();
					gef::DebugOut("Collision Location: %f, %f", ball_pos.x(), ball_pos.y());
				}
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
		gameState = INGAME;
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
	b2Vec2 gravity(0.0f, -5.f);
	world_ = new b2World(gravity);

	InitBall();
	InitBoard();
	InitBarriers();
	InitFlippers();
	InitLoseTrigger();
}

void SceneApp::GameRelease()
{
	// destroy the ball objects and clear the vector
	for (auto ball_obj : ball_vec_)
	{
		delete ball_obj;
	}
	ball_vec_.clear();
	ball_body_vec_.clear();

	// destroy the ball objects and clear the vector
	for (auto barrier_obj : barrier_vec_)
	{
		delete barrier_obj;
	}
	barrier_vec_.clear();
	barrier_body_vec_.clear();

	// destroy the flipper objects and clear the vector
	for (int jointCount = 0; jointCount < flipper_joint_vec_.size(); jointCount++)
	{
		world_->DestroyJoint(flipper_joint_vec_[jointCount]);
	}
	flipper_joint_vec_.clear();
	for (auto flipper_obj : flipper_vec_)
	{
		delete flipper_obj;
	}
	flipper_vec_.clear();
	flipper_body_vec_.clear();
	flipper_pin_body_vec_.clear();

	// destroying the physics world also destroys all the objects within it
	delete world_;
	world_ = NULL;

	delete scene_assets_;
	scene_assets_ = NULL;

	delete primitive_builder_;
	primitive_builder_ = NULL;

	delete renderer_3d_;
	renderer_3d_ = NULL;

}

void SceneApp::GameUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);

	switch (controller->buttons_released())
	{
	case (gef_SONY_CTRL_SQUARE):
		for (int i = 0; i < flipper_vec_.size(); i++)
		{
			if (flipper_vec_[i]->get_left())
			{
				flipper_joint_vec_[i]->SetMotorSpeed(-500.f);
			}
		}
		break;
	case (gef_SONY_CTRL_CIRCLE):
		for (int i = 0; i < flipper_vec_.size(); i++)
		{
			if (!flipper_vec_[i]->get_left())
			{
				flipper_joint_vec_[i]->SetMotorSpeed(500.f);
			}
		}
		break;
	case (40960):
		for (int i = 0; i < flipper_vec_.size(); i++)
		{
			if (flipper_vec_[i]->get_left())
			{
				flipper_joint_vec_[i]->SetMotorSpeed(-500.f);
			}
			else
			{
				flipper_joint_vec_[i]->SetMotorSpeed(500.f);
			}
		}
	default:
		break;
	}
	switch (controller->buttons_pressed())
	{
	case (gef_SONY_CTRL_SELECT):
		if (gameState == INGAME)
		{
			gameState = PAUSE;
		}
		else
		{
			gameState = INGAME;
		}
		return;
		break;
	case (gef_SONY_CTRL_TRIANGLE):
		GameRelease();
		gameState = GAMEOVER;
		IntervalInit();
		return;
		break;
	case (gef_SONY_CTRL_CROSS):
		GameRelease();
		GameInit();
		return;
		break;
	case (gef_SONY_CTRL_SQUARE):
		for (int i = 0; i < flipper_vec_.size(); i++)
		{
			if (flipper_vec_[i]->get_left())
			{
				flipper_joint_vec_[i]->SetMotorSpeed(500.f);
			}
		}
		break;
	case (gef_SONY_CTRL_CIRCLE):
		for (int i = 0; i < flipper_vec_.size(); i++)
		{
			if (!flipper_vec_[i]->get_left())
			{
				flipper_joint_vec_[i]->SetMotorSpeed(-500.f);
			}
		}
		break;
	case (40960):
		for (int i = 0; i < flipper_vec_.size(); i++)
		{
			if (flipper_vec_[i]->get_left())
			{
				flipper_joint_vec_[i]->SetMotorSpeed(500.f);
			}
			else
			{
				flipper_joint_vec_[i]->SetMotorSpeed(-500.f);
			}
		}
	default:
		break;
	}

	if (gameState == INGAME)
	{
		UpdateSimulation(frame_time);
	}
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
	gef::Vector4 camera_eye(0.0f, -35.0f, 30.0f);
	gef::Vector4 camera_lookat(0.0f, -10.0f, 0.0f);
	gef::Vector4 camera_up(0.0f, 1.0f, 0.0f);
	gef::Matrix44 view_matrix;
	view_matrix.LookAt(camera_eye, camera_lookat, camera_up);
	renderer_3d_->set_view_matrix(view_matrix);
	
	// draw 3d geometry
	renderer_3d_->Begin();

	// draw board
	renderer_3d_->DrawMesh(board_);
	renderer_3d_->DrawMesh(lose_trigger_);

	// draw flippers
	for (int flipperCount = 0; flipperCount < flipper_vec_.size(); flipperCount++)
	{
		renderer_3d_->DrawMesh(*flipper_vec_[flipperCount]);
	}

	for (int barrierCount = 0; barrierCount < barrier_vec_.size(); barrierCount++)
	{
		renderer_3d_->DrawMesh(*barrier_vec_[barrierCount]);
	}

	// draw ball
	renderer_3d_->set_override_material(&primitive_builder_->green_material());
	for (int ballCount = 0; ballCount < ball_vec_.size(); ballCount++)
	{
		renderer_3d_->DrawMesh(*ball_vec_[ballCount]);
	}
	renderer_3d_->set_override_material(NULL);

	renderer_3d_->End();

	// start drawing sprites, but don't clear the frame buffer
	sprite_renderer_->Begin(false);

	if (gameState == INGAME)
	{
		font_->RenderText(sprite_renderer_, 
			gef::Vector4(50.f, 10.f, -0.9f), 
			1.0f, 0xffffffff, gef::TJ_LEFT, 
			"Lives: %i", lives);
	}
	else
	{

		font_->RenderText(sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.7f, -0.99f),
			1.0f, 0xffffffff, gef::TJ_CENTRE,
			"Paused");
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
	case SceneApp::GAMEOVER:
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Game Over! \n\nYour score: ");
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
