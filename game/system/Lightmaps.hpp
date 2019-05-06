#pragma once

#include <entityx\System.h>

#include "other\Line.hpp"

#include <vector>

class Lightmaps : public entityx::System<Lightmaps>, public entityx::Receiver<Lightmaps> {
	std::vector<Line> _testingLines;

public:
	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void buildLightmaps(entityx::EntityManager & entities);
};