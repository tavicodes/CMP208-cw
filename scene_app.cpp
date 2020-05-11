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

	audio_manager_ = gef::AudioManager::Create();

	LoadScores();

	spaceBG = CreateTextureFromPNG("spacedust.png", platform_);
	simpleBG = CreateTextureFromPNG("simplebg.png", platform_);

	soundFX[0] = audio_manager_->LoadSample("highSFX.ogg", platform_);
	soundFX[1] = audio_manager_->LoadSample("mediumSFX.ogg", platform_);
	soundFX[2] = audio_manager_->LoadSample("lowSFX.ogg", platform_);

	for (int i = 0; i < 26; i++)
	{
		alph.push_back(i + 'A');
	}
	for (int i = 0; i < 10; i++)
	{
		alph.push_back(i + '0');
	}

	soundVol = 7;
	musicVol = 7;

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
	case PAUSE:
		GameUpdate(frame_time);
		break;
	case GAMEOVER:
	case NEWSCORE:
	case LEADERBOARD:
	case EXIT:
		return IntervalUpdate(frame_time);
		break;
	default:
		break;
	}

	gef::VolumeInfo vol;
	for (int i = 0; i < 3; i++)
	{
		audio_manager_->GetSampleVoiceVolumeInfo(i, vol);
		vol.volume = soundVol*10;
		audio_manager_->SetSampleVoiceVolumeInfo(i, vol);
	}
	audio_manager_->GetMusicVolumeInfo(vol);
	vol.volume = musicVol*10;
	audio_manager_->SetMusicVolumeInfo(vol);

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
	case SceneApp::PAUSE:
		GameRender();
		break;
	case SceneApp::GAMEOVER:
	case SceneApp::NEWSCORE:
	case SceneApp::LEADERBOARD:
	case SceneApp::EXIT:
		IntervalRender();
		break;
	default:
		break;
	}
}

void SceneApp::InitBall()
{
	int vecPos = ball_vec_.size();
	// setup the mesh for the ball
	ball_vec_.push_back(new Ball);
	ball_vec_[vecPos]->set_mesh(primitive_builder_->GetDefaultSphereMesh());

	// create a physics body for the ball
	b2BodyDef ball_body_def;
	ball_body_def.type = b2_dynamicBody;
	ball_body_def.position = b2Vec2(4.5f, 4.0f);

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
	ball_fixture_def.filter.categoryBits = BALL;

	// create the fixture on the rigid body
	ball_body_vec_[vecPos]->CreateFixture(&ball_fixture_def);

	// update visuals from simulation data
	ball_vec_[vecPos]->UpdateFromSimulation(ball_body_vec_[vecPos]);

	// create a connection between the rigid body and GameObject
	ball_body_vec_[vecPos]->SetUserData(ball_vec_[vecPos]);
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
	fixture_def.filter.categoryBits = BOARD;
	fixture_def.filter.maskBits = BALL;
	
	// create the fixture on the rigid body
	board_body_->CreateFixture(&fixture_def);
	// update visuals from simulation data
	board_.UpdateFromSimulation(board_body_);

	// create a connection between the rigid body and GameObject
	board_body_->SetUserData(&board_);
}

void SceneApp::InitBarriers()
{
	int barrierCount = 5;

	// barrier dimensions
	gef::Vector4 barrier_half_dimensions(0.4f, 0.3f, 1.0f);
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
		barrier_body_def.position = b2Vec2((-3.0f + i*1.5f), (1.5f + (i % 2) * 1.7f));
		barrier_body_vec_.push_back(world_->CreateBody(&barrier_body_def));
	}

	// create the shape for the barrier
	b2PolygonShape shape;
	shape.SetAsBox(barrier_half_dimensions.x(), barrier_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.density = 1.0f;
	fixture_def.filter.categoryBits = BARRIER;
	fixture_def.filter.maskBits = BALL;

	for (int i = 0; i < barrierCount; i++)
	{
		// create the fixture on the rigid body
		barrier_body_vec_[i]->CreateFixture(&fixture_def);

		// update visuals from simulation data
		barrier_vec_[i]->UpdateFromSimulation(barrier_body_vec_[i]);

		// create a connection between the rigid body and GameObject
		barrier_body_vec_[i]->SetUserData(barrier_vec_[i]);
	}
}

void SceneApp::InitBumpers()
{
	// bumper dimensions
	float bumper_radius = 1.5f;
	std::vector<gef::Vector4*> bumper_centre_vec;
	bumper_centre_vec.push_back(new gef::Vector4(0.f, 13.5f, 0.0f));
	bumper_centre_vec.push_back(new gef::Vector4(-4.5f, 10.f, 0.0f));
	bumper_centre_vec.push_back(new gef::Vector4(4.5f, 10.f, 0.0f));

	for (int i = 0; i < bumper_centre_vec.size(); i++)
	{
		// setup the mesh for the bumper
		bumper_mesh_ = primitive_builder_->CreateSphereMesh(bumper_radius, 40, 20);
		bumper_vec_.push_back(new GameObject);
		bumper_vec_[i]->set_mesh(bumper_mesh_);
		bumper_vec_[i]->set_type(BUMPER);
	}

	// create a physics body for the bumper
	b2BodyDef bumper_body_def;
	bumper_body_def.type = b2_staticBody;

	for (int i = 0; i < bumper_centre_vec.size(); i++)
	{
		bumper_body_def.position = b2Vec2(bumper_centre_vec[i]->x(), bumper_centre_vec[i]->y());
		bumper_body_vec_.push_back(world_->CreateBody(&bumper_body_def));
	}

	// create the shape for the bumper
	b2CircleShape shape;
	shape.m_radius = bumper_radius;

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.density = 1.0f;
	fixture_def.restitution = 1.2f;
	fixture_def.filter.categoryBits = BUMPER;
	fixture_def.filter.maskBits = BALL;

	for (int i = 0; i < bumper_centre_vec.size(); i++)
	{
		// create the fixture on the rigid body
		bumper_body_vec_[i]->CreateFixture(&fixture_def);

		// update visuals from simulation data
		bumper_vec_[i]->UpdateFromSimulation(bumper_body_vec_[i]);

		// create a connection between the rigid body and GameObject
		bumper_body_vec_[i]->SetUserData(bumper_vec_[i]);
	}
}

void SceneApp::InitFlipperBumpers()
{
	// bumper dimensions
	float bumper_radius = 2.5f;
	std::vector<gef::Vector4*> bumper_centre_vec;
	bumper_centre_vec.push_back(new gef::Vector4(-8.f, -19.3f, 0.0f));
	bumper_centre_vec.push_back(new gef::Vector4(8.f, -19.3f, 0.0f));

	for (int i = 0; i < bumper_centre_vec.size(); i++)
	{
		// setup the mesh for the bumper
		bumper_mesh_ = primitive_builder_->CreateSphereMesh(bumper_radius, 40, 20);
		bumper_vec_.push_back(new GameObject);
		bumper_vec_[bumper_vec_.size() - 1]->set_mesh(bumper_mesh_);
		bumper_vec_[bumper_vec_.size() - 1]->set_type(BUMPER);
	}

	// create a physics body for the bumper
	b2BodyDef bumper_body_def;
	bumper_body_def.type = b2_staticBody;

	for (int i = 0; i < bumper_centre_vec.size(); i++)
	{
		bumper_body_def.position = b2Vec2(bumper_centre_vec[i]->x(), bumper_centre_vec[i]->y());
		bumper_body_vec_.push_back(world_->CreateBody(&bumper_body_def));
	}

	// create the shape for the bumper
	b2CircleShape shape;
	shape.m_radius = bumper_radius;

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.density = 1.0f;
	fixture_def.restitution = 0.4f;
	fixture_def.filter.categoryBits = BUMPER;
	fixture_def.filter.maskBits = BALL;

	for (int i = bumper_vec_.size() - bumper_centre_vec.size(); i < bumper_vec_.size(); i++)
	{
		// create the fixture on the rigid body
		bumper_body_vec_[i]->CreateFixture(&fixture_def);

		// update visuals from simulation data
		bumper_vec_[i]->UpdateFromSimulation(bumper_body_vec_[i]);

		// create a connection between the rigid body and GameObject
		bumper_body_vec_[i]->SetUserData(bumper_vec_[i]);
	}
}

void SceneApp::InitFlippers()
{
	// flipper dimensions
	gef::Vector4 flipper_half_dimensions(2.05f, 0.3f, 0.5f);
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

	flipper_def.position = b2Vec2(-3.05f, -19.5f);
	flipper_body_vec_.push_back(world_->CreateBody(&flipper_def));

	flipper_pin_def.position = flipper_def.position - b2Vec2(1.8f, 0);
	flipper_pin_body_vec_.push_back(world_->CreateBody(&flipper_pin_def));

	flipper_joint_def.bodyA = flipper_pin_body_vec_[0];
	flipper_joint_def.bodyB = flipper_body_vec_[0];
	flipper_joint_def.localAnchorB.Set(-1.75f, 0);
	flipper_joint_def.lowerAngle = gef::DegToRad(-30.f);
	flipper_joint_def.upperAngle = gef::DegToRad(30.f);
	flipper_joint_def.motorSpeed = -500.f;
	flipper_joint_vec_.push_back((b2RevoluteJoint*)world_->CreateJoint(&flipper_joint_def));

	// flipper two

	flipper_def.position = b2Vec2(3.05f, -19.5f);
	flipper_body_vec_.push_back(world_->CreateBody(&flipper_def));

	flipper_pin_def.position = flipper_def.position + b2Vec2(1.8f, 0);
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
	fixture_def.filter.categoryBits = FLIPPER;
	fixture_def.filter.maskBits = BALL;

	for (int i = 0; i < 2; i++)
	{
		// create the fixture on the rigid body
		flipper_body_vec_[i]->CreateFixture(&fixture_def);

		// update visuals from simulation data
		flipper_vec_[i]->UpdateFromSimulation(flipper_body_vec_[i]);

		// create a connection between the rigid body and GameObject
		flipper_body_vec_[i]->SetUserData(flipper_vec_[i]);
	}

}

void SceneApp::InitLoseTrigger()
{
	lose_trigger_.set_type(LOSETRIGGER);
	// lose trigger dimensions
	gef::Vector4 lt_half_dimensions(8.5f, 0.2f, 0.5f);

	// setup the mesh for the board
	lose_trigger_mesh_ = primitive_builder_->CreateBoxMesh(lt_half_dimensions);
	lose_trigger_.set_mesh(lose_trigger_mesh_);

	// create a physics body
	b2BodyDef body_def;
	body_def.type = b2_staticBody;
	body_def.position = b2Vec2(0.0f, -25.5f);

	lose_trigger_body_ = world_->CreateBody(&body_def);

	// create the shape
	b2PolygonShape shape;
	shape.SetAsBox(lt_half_dimensions.x(), lt_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.filter.categoryBits = LOSETRIGGER;
	fixture_def.filter.maskBits = BALL;

	// create the fixture on the rigid body
	lose_trigger_body_->CreateFixture(&fixture_def);

	// update visuals from simulation data
	lose_trigger_.UpdateFromSimulation(lose_trigger_body_);

	// create a connection between the rigid body and GameObject
	lose_trigger_body_->SetUserData(&lose_trigger_);
}

bool SceneApp::CheckBarriers()
{
	for (int barrierCount = 0; barrierCount < barrier_vec_.size(); barrierCount++)
	{
		if (!barrier_vec_[barrierCount]->get_hit())
		{
			return false;
		}
	}
	return true;
}

void SceneApp::LoadScores()
{
	std::ifstream scoresFile("scores.txt");

	if (!scoresFile.good())
	{
		scoresFile.close();
		std::ofstream newScoresFile("scores.txt");
		newScoresFile << "AAA,0";
		newScoresFile.close();
		ResetScores();
	}
	else
	{
		std::string line, val;
		bool pair = true;

		// read data, line by line
		while (scoresFile.good())
		{
			std::getline(scoresFile, line);
			// Create a stringstream of the current line
			std::stringstream ss(line);

			std::pair<std::string, unsigned int> temp;

			// read every column data of a row and 
			// store it in a string variable
			// then fill the pair variable
			while (std::getline(ss, val, ','))
			{
				if (pair)
				{
					temp.first = val;
				}
				else
				{
					temp.second = std::stoi(val);
				}

				pair = !pair;
			}

			scores.push_back(temp);
		}

		scoresFile.close();
	}
}

void SceneApp::SaveScores()
{
	std::ofstream scoresFile("scores.txt", std::ofstream::trunc);
	scoresFile << scores[0].first << "," << std::to_string(scores[0].second);

	int endOfFile = scores.size();
	if (endOfFile > 10) endOfFile = 10;

	for (int i = 1; i < endOfFile; i++)
	{
		scoresFile << std::endl << scores[i].first << "," << std::to_string(scores[i].second);
	}

	scoresFile.close();
}

void SceneApp::ResetScores()
{
	scores.clear();
	std::pair<std::string, unsigned int> tempPair;
	std::string name = "AAA";
	tempPair.first = name;
	for (int i = 10; i > 0; i--)
	{
		tempPair.second = i * 100;
		scores.push_back(tempPair);
	}
	SaveScores();
}

bool SceneApp::CheckHighScore()
{
	for (scorePos = scores.begin(); scorePos != scores.end(); ++scorePos)
	{
		if (points > scorePos->second)
		{
			return true;
		}
	}
	return false;
}

void SceneApp::RenderScores()
{
	int scale0, scale1, scale2,
		height0, height1, height2;
	UInt32 col0, col1, col2;
	int	temp00, temp10, temp20,
		temp01, temp11, temp21;

	scale0 = 3.5f;
	scale1 = 3.5f;
	scale2 = 3.5f;
	height0 = 50.f;
	height1 = 50.f;
	height2 = 50.f;
	col0 = 0xffffffff;
	col1 = 0xffffffff;
	col2 = 0xffffffff;

	switch (charSelected)
	{
	case 0:
		scale0 = 5.f;
		col0 = 0xff025aad;
		height0 = 80.f;
		break;
	case 1:
		scale1 = 5.f;
		col1 = 0xff025aad;
		height1 = 80.f;
		break;
	case 2:
		scale2 = 5.f;
		col2 = 0xff025aad;
		height2 = 80.f;
		break;
	default:
		break;
	}


	if (char0 == 0)
	{
		temp00 = 35;
		temp01 = char0 + 1;
	}
	else if (char0 == 35)
	{
		temp00 = char0 - 1;
		temp01 = 0;
	}
	else
	{
		temp00 = char0-1;
		temp01 = char0+1;
	}

	if (char1 == 0)
	{
		temp10 = 35;
		temp11 = char1+1;
	}
	else if (char1 == 35)
	{
		temp10 = char1-1;
		temp11 = 0;
	}
	else
	{
		temp10 = char1-1;
		temp11 = char1+1;
	}

	if (char2 == 0)
	{
		temp20 = 35;
		temp21 = char2+1;
	}
	else if (char2 == 35)
	{
		temp20 = char2-1;
		temp21 = 0;
	}
	else
	{
		temp20 = char2-1;
		temp21 = char2+1;
	}

	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f - 80.f, platform_.height() * 0.5f - 120.0f, -0.99f),
		1.5f,
		0xffffffff,
		gef::TJ_CENTRE,
		"%c", alph[temp00]);
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f - 120.0f, -0.99f),
		1.5f,
		0xffffffff,
		gef::TJ_CENTRE,
		"%c", alph[temp10]);
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f + 80.f, platform_.height() * 0.5f - 120.0f, -0.99f),
		1.5f,
		0xffffffff,
		gef::TJ_CENTRE,
		"%c", alph[temp20]);

	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f - 80.f, platform_.height() * 0.5f - height0, -0.99f),
		scale0,
		col0,
		gef::TJ_CENTRE,
		"%c", alph[char0]);
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f - height1, -0.99f),
		scale1,
		col1,
		gef::TJ_CENTRE,
		"%c", alph[char1]);
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f + 80.f, platform_.height() * 0.5f - height2, -0.99f),
		scale2,
		col2,
		gef::TJ_CENTRE,
		"%c", alph[char2]);

	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f - 80.f, platform_.height() * 0.5f + 60.f, -0.99f),
		1.5f,
		0xffffffff,
		gef::TJ_CENTRE,
		"%c", alph[temp01]);
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f + 60.f, -0.99f),
		1.5f,
		0xffffffff,
		gef::TJ_CENTRE,
		"%c", alph[temp11]);
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f + 80.f, platform_.height() * 0.5f + 60.f, -0.99f),
		1.5f,
		0xffffffff,
		gef::TJ_CENTRE,
		"%c", alph[temp21]);
}

void SceneApp::RenderLeaderboard()
{
	for (int i = 0; i < scores.size(); i++)
	{
		std::string tempStr = std::string(8 - std::to_string(scores[i].second).length(), '0') + std::to_string(scores[i].second);

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(40.f + ((double)sin(leaderboardSway + i * 1.2f) * 20.f), 80.f + (i * 60.f), -0.99f),
			3.f,
			0xffffffff,
			gef::TJ_LEFT,
			"%s", scores[i].first.c_str());
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4((platform_.width() - 40.f) + ((double)sin(leaderboardSway + i * 1.2f) * 20.f), 80.f + (i * 60.f), -0.99f),
			3.f,
			0xffffffff,
			gef::TJ_RIGHT,
			"%s", tempStr.c_str());
	}
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

void SceneApp::DrawBG()
{
	gef::Sprite background;
	if (gameState == INGAME || gameState == PAUSE || gameState == GAMEOVER)
	{
		background.set_texture(spaceBG);
	}
	else
	{
		background.set_texture(simpleBG);
	}
	background.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f, 1.f));
	background.set_height(platform_.height());
	background.set_width(platform_.width());
	// draw 3d geometry
	sprite_renderer_->DrawSprite(background);
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

	if (contact_count == ball_vec_.size() - 1)
	{
		contacted = false;
	}

	for (int contact_num = 0; contact_num<contact_count; ++contact_num)
	{
		if (contact->IsTouching())
		{
			// get the colliding bodies
			b2Body* bodyA = contact->GetFixtureA()->GetBody();
			b2Body* bodyB = contact->GetFixtureB()->GetBody();
			b2Filter filterA = contact->GetFixtureA()->GetFilterData();
			b2Filter filterB = contact->GetFixtureB()->GetFilterData();

			// DO COLLISION RESPONSE HERE
			Barrier* barrier = NULL;

			int sfx = std::rand() / ((RAND_MAX + 1) / 3);

			switch (filterA.categoryBits)
			{
			case BARRIER:
				barrier = (Barrier*)bodyA->GetUserData();
				barrier->set_hit(true);
				filterA.categoryBits = HITBARRIER;
				filterA.maskBits = NULL;
				bodyA->GetFixtureList()->SetFilterData(filterA);

				if (!contacted)
				{
					audio_manager_->PlaySample(soundFX[sfx]);
					points += 25;
					contacted = true;
				}
				break;
			case LOSETRIGGER:
				LostLife((Ball*)bodyB->GetUserData());
				contact = world_->GetContactList();
				contact_count = world_->GetContactCount();
				break;
			case FLIPPER:
				if (CheckBarriers())
				{
					for (int barrierCount = 0; barrierCount < barrier_vec_.size(); barrierCount++)
					{
						barrier_vec_[barrierCount]->set_hit(false);
						b2Filter filter = barrier_body_vec_[barrierCount]->GetFixtureList()->GetFilterData();
						filter.categoryBits = BARRIER;
						filter.maskBits = BALL;
						barrier_body_vec_[barrierCount]->GetFixtureList()->SetFilterData(filter);
					}
					InitBall();
				}
				
				if (!contacted)
				{
					audio_manager_->PlaySample(soundFX[sfx]);
					points += 10;
					contacted = true;
				}
				break;
			case BUMPER:
				
				if (!contacted)
				{
					audio_manager_->PlaySample(soundFX[sfx]);
					points += 15;
					contacted = true;
				}
				break;
			default:
				break;
			}

			switch (filterB.categoryBits)
			{
			case BARRIER:
				barrier = (Barrier*)bodyB->GetUserData();
				barrier->set_hit(true);
				filterB.categoryBits = HITBARRIER;
				filterB.maskBits = NULL;
				bodyB->GetFixtureList()->SetFilterData(filterB);
				
				if (!contacted)
				{
					audio_manager_->PlaySample(soundFX[sfx]);
					points += 25;
					contacted = true;
				}
				break;
			case LOSETRIGGER:
				LostLife((Ball*)bodyA->GetUserData());
				contact = world_->GetContactList();
				contact_count = world_->GetContactCount();
				break;
			case FLIPPER:
				if (CheckBarriers())
				{
					for (int barrierCount = 0; barrierCount < barrier_vec_.size(); barrierCount++)
					{
						barrier_vec_[barrierCount]->set_hit(false);
						b2Filter filter = barrier_body_vec_[barrierCount]->GetFixtureList()->GetFilterData();
						filter.categoryBits = BARRIER;
						filter.maskBits = BALL;
						barrier_body_vec_[barrierCount]->GetFixtureList()->SetFilterData(filter);
					}
					InitBall();
				}
				
				if (!contacted)
				{
					audio_manager_->PlaySample(soundFX[sfx]);
					points += 10;
					contacted = true;
				}
				break;
			case BUMPER:
				
				if (!contacted)
				{
					audio_manager_->PlaySample(soundFX[sfx]);
					points += 15;
					contacted = true;
				}
				break;
			default:
				break;
			}
		}

		if (contact != NULL)
		{			
			// Get next contact point
			contact = contact->GetNext();
		}
	}
}

void SceneApp::LostLife(Ball* dead_ball)
{
	for (int i = 0; i < ball_body_vec_.size(); i++)
	{
		if (ball_body_vec_[i]->GetUserData() == dead_ball)
		{
			world_->DestroyBody(ball_body_vec_[i]);
			ball_body_vec_.erase(ball_body_vec_.begin() + i);
		}
	}
	for (int i = 0; i < ball_vec_.size(); i++)
	{
		if (ball_vec_[i] == dead_ball)
		{
			delete dead_ball;
			ball_vec_.erase(ball_vec_.begin() + i);
		}
	}
	if (ball_vec_.empty())
	{
		if (lives > 0)
		{
			lives--;
			InitBall();
		}
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
	delete squareButton;
	squareButton = NULL;
	delete circleButton;
	circleButton = NULL;
	delete triangleButton;
	triangleButton = NULL;
}

void SceneApp::FrontendUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);
	switch (controller->buttons_pressed())
	{
	case (gef_SONY_CTRL_CROSS):
		FrontendRelease();
		gameState = INGAME;
		GameInit();
		break;
	case (gef_SONY_CTRL_CIRCLE):
		FrontendRelease();
		gameState = EXIT;
		IntervalInit();
		break;
	case (gef_SONY_CTRL_TRIANGLE):
		gameState = OPTIONS;
		OptionsInit();
		break;
	case (gef_SONY_CTRL_SQUARE):
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

	DrawBG();

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
	button.set_texture(crossButton);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	button.set_texture(triangleButton);
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

	audio_manager_->StopMusic();
	audio_manager_->UnloadMusic();
	int sfx = std::rand() / ((RAND_MAX + 1) / 3);
	switch (sfx)
	{
	case 0:
		musicSFX = audio_manager_->LoadMusic("Beauty-Flow.wav", platform_);
		break;
	case 1:
		musicSFX = audio_manager_->LoadMusic("EDM-Detection-Mode.wav", platform_);
		break;
	case 2:
		musicSFX = audio_manager_->LoadMusic("Inspired.wav", platform_);
		break;
	default:
		break;
	}
	audio_manager_->PlayMusic();

	SetupLights();
	contacted = false;
	lives = 3;
	points = 0;
	optSelected = 0;

	// initialise the physics world
	b2Vec2 gravity(0.0f, -5.f);
	world_ = new b2World(gravity);

	InitBall();
	InitBoard();
	InitBarriers();
	InitBumpers();
	InitFlipperBumpers();
	InitFlippers();
	InitLoseTrigger();
}

void SceneApp::GameRelease()
{
	contacted = NULL;
	lives = NULL;
	optSelected = NULL;
	optSelected = NULL;

	// destroy the ball objects and clear the vector
	for (auto ball_obj : ball_vec_)
	{
		delete ball_obj;
	}
	ball_vec_.clear();
	ball_body_vec_.clear();

	// destroy the bumper objects and clear the vector
	for (auto bumper_obj : bumper_vec_)
	{
		delete bumper_obj;
	}
	bumper_vec_.clear();
	bumper_body_vec_.clear();

	// destroy the barrier objects and clear the vector
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

	float flipperSpeed = 1000.f;

	if (controller->buttons_pressed() > 0)
	{
		gef::DebugOut("%i\n", controller->buttons_pressed());
	}

	if (gameState == INGAME)
	{
		switch (controller->buttons_released())
		{
		case (gef_SONY_CTRL_SQUARE):
		case (gef_SONY_CTRL_L1):
			for (int i = 0; i < flipper_vec_.size(); i++)
			{
				if (flipper_vec_[i]->get_left())
				{
					flipper_joint_vec_[i]->SetMotorSpeed(-flipperSpeed);
				}
			}
			break;
		case (gef_SONY_CTRL_CIRCLE):
		case (gef_SONY_CTRL_R1):
			for (int i = 0; i < flipper_vec_.size(); i++)
			{
				if (!flipper_vec_[i]->get_left())
				{
					flipper_joint_vec_[i]->SetMotorSpeed(flipperSpeed);
				}
			}
			break;
		case (40960):
		case (3072):
			for (int i = 0; i < flipper_vec_.size(); i++)
			{
				if (flipper_vec_[i]->get_left())
				{
					flipper_joint_vec_[i]->SetMotorSpeed(-flipperSpeed);
				}
				else
				{
					flipper_joint_vec_[i]->SetMotorSpeed(flipperSpeed);
				}
			}
		default:
			break;
		}
		switch (controller->buttons_pressed())
		{
		case (gef_SONY_CTRL_SELECT):
			gameState = PAUSE;
			return;
			break;
		case (gef_SONY_CTRL_SQUARE):
		case (gef_SONY_CTRL_L1):
			for (int i = 0; i < flipper_vec_.size(); i++)
			{
				if (flipper_vec_[i]->get_left())
				{
					flipper_joint_vec_[i]->SetMotorSpeed(flipperSpeed);
				}
			}
			break;
		case (gef_SONY_CTRL_CIRCLE):
		case (gef_SONY_CTRL_R1):
			for (int i = 0; i < flipper_vec_.size(); i++)
			{
				if (!flipper_vec_[i]->get_left())
				{
					flipper_joint_vec_[i]->SetMotorSpeed(-flipperSpeed);
				}
			}
			break;
		case (40960):
		case (3072):
			for (int i = 0; i < flipper_vec_.size(); i++)
			{
				if (flipper_vec_[i]->get_left())
				{
					flipper_joint_vec_[i]->SetMotorSpeed(flipperSpeed);
				}
				else
				{
					flipper_joint_vec_[i]->SetMotorSpeed(-flipperSpeed);
				}
			}
		default:
			break;
		}
	}
	else
	{
		switch (controller->buttons_pressed())
		{
		case (gef_SONY_CTRL_DOWN):
			if (optSelected < 3)
			{
				optSelected++;
			}
			break;
		case (gef_SONY_CTRL_UP):
			if (optSelected > 0)
			{
				optSelected--;
			}
			break;
		case (gef_SONY_CTRL_LEFT):
			if (optSelected == 1 && soundVol > 0)
			{
				soundVol--;
			}
			else if (optSelected == 2 && musicVol > 0)
			{
				musicVol--;
			}
			break;
		case (gef_SONY_CTRL_RIGHT):
			if (optSelected == 1 && soundVol < 10)
			{
				soundVol++;
			}
			else if (optSelected == 2 && musicVol < 10)
			{
				musicVol++;
			}
			break;
		case (gef_SONY_CTRL_CROSS):
			if (optSelected == 0)
			{
				gameState = INGAME;
				return;
			}
			else if (optSelected == 3)
			{
				GameRelease();
				gameState = GAMEOVER;
				IntervalInit();
				return;
			}
			break;
		case (gef_SONY_CTRL_SELECT):
			gameState = INGAME;
			return;
			break;
		default:
			break;
		}
	}

	if (gameState == INGAME)
	{
		UpdateSimulation(frame_time);
	}
	if (lives == 0)
	{
		GameRelease();
		gameState = GAMEOVER;
		IntervalInit();
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

	renderer_3d_->set_override_material(&primitive_builder_->red_material());
	for (int bumperCount = 0; bumperCount < bumper_vec_.size(); bumperCount++)
	{
		renderer_3d_->DrawMesh(*bumper_vec_[bumperCount]);
	}
	renderer_3d_->set_override_material(NULL);

	for (int barrierCount = 0; barrierCount < barrier_vec_.size(); barrierCount++)
	{
		if (!barrier_vec_[barrierCount]->get_hit())
		{
			renderer_3d_->DrawMesh(*barrier_vec_[barrierCount]);
		}
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

	DrawBG();

	if (gameState == INGAME)
	{
		font_->RenderText(sprite_renderer_, 
			gef::Vector4(50.f, 10.f, -0.9f), 
			1.0f, 0xffffffff, gef::TJ_LEFT, 
			"Lives: %i", lives);
		// display score
		font_->RenderText(sprite_renderer_, gef::Vector4(400.f, 10.f, -0.9f), 1.0f, 0xffffffff, gef::TJ_CENTRE, "SCORE: %i", points);
	}
	else
	{
		UInt32 highlight0 = 0xffffffff, highlight1 = 0xffffffff, highlight2 = 0xffffffff, highlight3 = 0xffffffff;
		switch (optSelected)
		{
		case 0:
			highlight0 = 0xff025aad;
			break;
		case 1:
			highlight1 = 0xff025aad;
			break;
		case 2:
			highlight2 = 0xff025aad;
			break;
		case 3:
			highlight3 = 0xff025aad;
			break;
		default:
			break;
		}

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.4f - 60.f, -0.99f),
			1.0f,
			highlight0,
			gef::TJ_CENTRE,
			"Resume");
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.35f, platform_.height() * 0.4f - 20.f, -0.99f),
			1.0f,
			highlight1,
			gef::TJ_RIGHT,
			"Sound Volume: ");
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.35f, platform_.height() * 0.4f + 20.f, -0.99f),
			1.0f,
			highlight2,
			gef::TJ_RIGHT,
			"Music Volume: ");
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.4f + 60.f, -0.99f),
			1.0f,
			highlight3,
			gef::TJ_CENTRE,
			"Quit");

		gef::Sprite soundSquare;
		soundSquare.set_height(20.f);
		soundSquare.set_width(20.f);

		for (int i = 0; i < soundVol; i++)
		{
			soundSquare.set_position(gef::Vector4(platform_.width() * 0.37f + i * 25.f, platform_.height() * 0.4f - 5.f, -0.99f));
			sprite_renderer_->DrawSprite(soundSquare);
		}
		for (int i = 0; i < musicVol; i++)
		{
			soundSquare.set_position(gef::Vector4(platform_.width() * 0.37f + i * 25.f, platform_.height() * 0.4f + 35.f, -0.99f));
			sprite_renderer_->DrawSprite(soundSquare);
		}

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
	charSelected = 0;
	char0 = 0;
	char1 = 0;
	char2 = 0;
	leaderboardSway = 0;

	logo = CreateTextureFromPNG("logo.png", platform_);
	crossButton = CreateTextureFromPNG("playstation-cross-dark-icon.png", platform_);
}

void SceneApp::IntervalRelease()
{
	points = NULL;
	timer = NULL;
	charSelected = NULL;
	char0 = NULL;
	char1 = NULL;
	char2 = NULL;
	leaderboardSway = NULL;

	delete crossButton;
	crossButton = NULL;
}

bool SceneApp::IntervalUpdate(float frame_time)
{
	timer += frame_time;

	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);

	switch (gameState)
	{
	case INIT:
		if (timer > 3)
		{
			IntervalRelease();
			gameState = MENU;
			FrontendInit();
		}
		break;
	case SceneApp::GAMEOVER:
		if (controller->buttons_pressed() == gef_SONY_CTRL_CROSS)
		{
			if (CheckHighScore())
			{
				gameState = NEWSCORE;
			}
			else
			{
				gameState = LEADERBOARD;
			}
		}
		break;
	case SceneApp::NEWSCORE:
		switch (controller->buttons_pressed())
		{
		case (gef_SONY_CTRL_CROSS):
			if (charSelected < 2)
			{
				charSelected++;
			}
			else
			{
				std::string tempStr;
				tempStr.push_back(alph[char0]);
				tempStr.push_back(alph[char1]);
				tempStr.push_back(alph[char2]);

				std::pair<std::string, unsigned int> temp;
				temp.first = tempStr;
				temp.second = points;
				scores.insert(scorePos, temp);

				SaveScores();
				timer = 0;
				gameState = LEADERBOARD;
			}
			break;
		case (gef_SONY_CTRL_RIGHT):
			if (charSelected < 2)
			{
				charSelected++;
			}
			break;
		case (gef_SONY_CTRL_LEFT):
		case (gef_SONY_CTRL_CIRCLE):
			if (charSelected > 0)
			{
				charSelected--;
			}
			break;
		case (gef_SONY_CTRL_UP):
			switch (charSelected)
			{
			case 0:
				if (char0 > 0)	char0--;
				else char0 = 35;
				break;
			case 1:
				if (char1 > 0) char1--;
				else char1 = 35;
				break;
			case 2:
				if (char2 > 0) char2--;
				else char2 = 35;
				break;
			default:
				break;
			}
			break;
		case (gef_SONY_CTRL_DOWN):
			switch (charSelected)
			{
			case 0:
				if (char0 < 35)	char0++;
				else char0 = 0;
				break;
			case 1:
				if (char1 < 35) char1++;
				else char1 = 0;
				break;
			case 2:
				if (char2 < 35) char2++;
				else char2 = 0;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case SceneApp::LEADERBOARD:
		leaderboardSway += frame_time * 1.4f;
		if (controller->buttons_pressed() == gef_SONY_CTRL_CROSS)
		{
			IntervalRelease();
			gameState = MENU;
			FrontendInit();
		}
		break;
	case SceneApp::EXIT:
		if (timer > 1)
		{
			SaveScores();
			IntervalRelease();
			return false;
		}
		break;
	default:
		break;
	}

	return true;
}

void SceneApp::IntervalRender()
{
	sprite_renderer_->Begin();

	DrawBG();

	gef::Sprite button;
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
		button.set_texture(logo);
		button.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f, -0.99f));
		button.set_height(202.f);
		button.set_width(342.f);
		sprite_renderer_->DrawSprite(button);
		break;
	case SceneApp::GAMEOVER:
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width()*0.5f, platform_.height()*0.4f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Game Over!");
		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.4f + 30.f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Your score: %i", points);

		button.set_texture(crossButton);
		button.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.8f, -0.99f));
		button.set_height(40.0f);
		button.set_width(40.0f);
		sprite_renderer_->DrawSprite(button);

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.8f + 30.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Press to continue");
		break;
	case SceneApp::NEWSCORE:
		RenderScores();

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.3f - 10.f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"New high score!");

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.3f + 20.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Enter your name below using the dpad:");

		button.set_texture(crossButton);
		button.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.8f, -0.99f));
		button.set_height(40.0f);
		button.set_width(40.0f);
		sprite_renderer_->DrawSprite(button);

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.8f + 25.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Press to confirm");
		break;
	case SceneApp::LEADERBOARD:
		RenderLeaderboard();

		button.set_texture(crossButton);
		button.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.8f, -0.99f));
		button.set_height(40.0f);
		button.set_width(40.0f);
		sprite_renderer_->DrawSprite(button);

		font_->RenderText(
			sprite_renderer_,
			gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.8f + 25.0f, -0.99f),
			1.0f,
			0xffffffff,
			gef::TJ_CENTRE,
			"Press to continue");

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
	circleButton = CreateTextureFromPNG("playstation-circle-dark-icon.png", platform_);
	optSelected = 0;
}

void SceneApp::OptionsRelease()
{
	delete circleButton;
	circleButton = NULL;
	optSelected = NULL;
}

void SceneApp::OptionsUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);
	switch (controller->buttons_pressed())
	{
	case (gef_SONY_CTRL_CIRCLE):
		gameState = MENU;
		FrontendInit();
		break;
	case (gef_SONY_CTRL_DOWN):
		if (optSelected < 3)
		{
			optSelected++;
		}
		break;
	case (gef_SONY_CTRL_UP):
		if (optSelected > 0)
		{
			optSelected--;
		}
		break;
	case (gef_SONY_CTRL_LEFT):
		if (optSelected == 0 && soundVol > 0)
		{
			soundVol--;
		}
		else if (optSelected == 1 && musicVol > 0)
		{
			musicVol--;
		}
		break;
	case (gef_SONY_CTRL_RIGHT):
		if (optSelected == 0 && soundVol < 10)
		{
			soundVol++;
		}
		else if (optSelected == 1 && musicVol < 10)
		{
			musicVol++;
		}
		break;
	case (gef_SONY_CTRL_CROSS):
		if (optSelected == 2)
		{
			ResetScores();
		}
		else if (optSelected == 3)
		{
			IntervalInit();
			gameState = LEADERBOARD;
			OptionsRelease();
		}
		break;
	default:
		break;
	}
}

void SceneApp::OptionsRender()
{
	UInt32 highlight0 = 0xffffffff, highlight1 = 0xffffffff, highlight2 = 0xffffffff, highlight3 = 0xffffffff;
	switch (optSelected)
	{
	case 0:
		highlight0 = 0xff025aad;
		break;
	case 1:
		highlight1 = 0xff025aad;
		break;
	case 2:
		highlight2 = 0xff025aad;
		break;
	case 3:
		highlight3 = 0xff025aad;
		break;
	default:
		break;
	}
	sprite_renderer_->Begin();

	DrawBG();

	// render "OPTIONS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.3f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"OPTIONS");

	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.35f, platform_.height() * 0.4f - 20.f, -0.99f),
		1.0f,
		highlight0,
		gef::TJ_RIGHT,
		"Sound Volume: ");
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.35f, platform_.height() * 0.4f + 20.f, -0.99f),
		1.0f,
		highlight1,
		gef::TJ_RIGHT,
		"Music Volume: ");
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f - 20.0f, -0.99f),
		1.0f,
		highlight2,
		gef::TJ_CENTRE,
		"Reset Leaderboard");
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f + 20.0f, -0.99f),
		1.0f,
		highlight3,
		gef::TJ_CENTRE,
		"Show Leaderboard");

	gef::Sprite soundSquare;
	soundSquare.set_height(20.f);
	soundSquare.set_width(20.f);

	for (int i = 0; i < soundVol; i++)
	{
		soundSquare.set_position(gef::Vector4(platform_.width() * 0.37f + i * 25.f, platform_.height() * 0.4f - 5.f, -0.99f));
		sprite_renderer_->DrawSprite(soundSquare);
	}
	for (int i = 0; i < musicVol; i++)
	{
		soundSquare.set_position(gef::Vector4(platform_.width() * 0.37f + i * 25.f, platform_.height() * 0.4f + 35.f, -0.99f));
		sprite_renderer_->DrawSprite(soundSquare);
	}

	// Render buttons
	gef::Sprite button;
	button.set_texture(circleButton);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.8f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	// render "TO MENU" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.8f + 32.0f, -0.99f),
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

	DrawBG();

	// render "CREDITS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.3f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"CREDITS");
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f - 20.f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"Sound effects from ZapSplat.com");
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f + 20.f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"Music from Kevin Macleod: incompetech.filmmusic.io");

	// Render buttons
	gef::Sprite button;
	button.set_texture(crossButton);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.8f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	// render "TO MENU" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.8f + 32.0f, -0.99f),
		1.0f,
		0xffffffff,
		gef::TJ_CENTRE,
		"TO MENU");

	sprite_renderer_->End();
}
