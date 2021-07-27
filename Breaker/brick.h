#ifndef BRICK_H_INCLUDED
#define BRICK_H_INCLUDED

#include <SDL.h>
class Brick {
	public:
		Brick(SDL_Surface *, int, int, int);
		Brick(SDL_Surface *, int, int, int, int);
		bool isVisible();
		int getScore();
		SDL_Rect* getRect();
		void registerHit();
		SDL_Surface* getSprite();
	private:
		SDL_Surface* sprite;
		SDL_Rect* rect;
		int xpos, ypos;
		int hits, score;
		bool destroyed;

		void defineRect();
};
#endif // BRICK_H_INCLUDED
