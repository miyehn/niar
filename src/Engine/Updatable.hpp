#pragma once
#include "Utils/lib.h"

struct Updatable {
	std::string name = "[unnamed updatable]";
	virtual void update(float time_elapsed) = 0;
	virtual bool handle_event(SDL_Event event) = 0;
	
	// enabled status
	bool enabled;
	virtual void enable() { enabled = true; }
	virtual void disable() { enabled = false; }

};
