#include "system\Audio.hpp"

#include "component\Name.hpp"

Audio::Audio(){ }

Audio::~Audio(){ }

void Audio::configure(entityx::EventManager & events){
	events.subscribe<ContactEvent>(*this);
	events.subscribe<CollidingEvent>(*this);
}

void Audio::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){

}

void Audio::receive(const CollidingEvent & collidingEvent){
	entityx::Entity entity = collidingEvent.contactEvent.firstEntity;

	if (!entity.has_component<Name>() || entity.component<Name>()->name != "test")
		return;

	if (collidingEvent.colliding)
		printf("Colliding!\n");
	else
		printf("Not colliding...\n");
}

void Audio::receive(const ContactEvent & contactEvent){

}
