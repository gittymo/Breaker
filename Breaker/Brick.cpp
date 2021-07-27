#include "brick.h"

Brick::Brick(SDL_Surface* _sprite, int _xpos, int _ypos, int _score) {
	sprite = _sprite;
	xpos = _xpos;
	ypos = _ypos;
	score = _score;
	hits = 1;
	destroyed = false;
	defineRect();
}

Brick::Brick(SDL_Surface* _sprite, int _xpos, int _ypos, int _score, int _hits) {
	sprite = _sprite;
	xpos = _xpos;
	ypos = _ypos;
	score = _score;
	hits = _hits;
	destroyed = false;
	defineRect();
}

bool Brick::isVisible() {
	return !destroyed;
}

int Brick::getScore() {
	return score;
}

void Brick::registerHit() {
	if (hits > 0) hits--;
	if (hits == 0) destroyed = true;
}

void Brick::defineRect() {
	rect = new SDL_Rect();
	rect->x = xpos;
	rect->y = ypos;
	rect->w = sprite->w;
	rect->h = sprite->h;
}

SDL_Rect* Brick::getRect() {
	return rect;
}

SDL_Surface* Brick::getSprite() {
	return sprite;
}
