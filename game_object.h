#ifndef _GAME_OBJECT_H
#define _GAME_OBJECT_H

#include <graphics/mesh_instance.h>
#include <box2d/Box2D.h>

enum OBJECT_TYPE
{
	BALL = 0x0001,
	FLIPPER = 0x0002,
	BARRIER = 0x0004,
	BUMPER = 0x0008,
	LOSETRIGGER = 0x0020,
	BOARD = 0x0010
};

class GameObject : public gef::MeshInstance
{
public:
	void UpdateFromSimulation(const b2Body* body);
	void MyCollisionResponse();

	inline void set_type(OBJECT_TYPE type) { type_ = type; }
	inline OBJECT_TYPE type() { return type_; }
private:
	OBJECT_TYPE type_;
};

class Ball : public GameObject
{
public:
	Ball();
};

class Flipper : public GameObject
{
public:
	Flipper();

	void set_left(bool option) { left = option; }
	bool get_left() { return left; }
private:
	bool left;
};

class Barrier : public GameObject
{
public:
	Barrier();

	void set_hit(bool option) { hit = option; }
	bool get_hit() { return hit; }
private:
	bool hit;
};

#endif // _GAME_OBJECT_H