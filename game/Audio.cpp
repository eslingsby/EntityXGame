#include "Audio.hpp"

Audio::Audio(){ }

Audio::~Audio(){ }

void Audio::configure(entityx::EventManager & events){
	events.subscribe<ContactEvent>(*this);
	events.subscribe<CollidingEvent>(*this);
}

void Audio::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){

}

void Audio::receive(const CollidingEvent & collidingEvent){

}

void Audio::receive(const ContactEvent & contactEvent){

}
