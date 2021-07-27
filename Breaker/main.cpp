#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif
#ifdef __APPLE__
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_ttf.h>
#endif

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include "brick.h"

using std::ifstream;

const char* convertToString(std::string _prefix, int _value) {
	std::ostringstream oss;
	oss << _prefix << _value;
	std::string cstr = oss.str();

	return cstr.c_str();
}

const char* convertToString(std::string _prefix, int _value, std::string _suffix) {
	std::ostringstream oss;
	oss << _prefix << _value << _suffix;
	std::string cstr = oss.str();

	return cstr.c_str();
}

bool loadBaseGraphics(SDL_PixelFormat* _pixel_format, SDL_Surface* &_bat, SDL_Surface* &_ball, SDL_Surface* _bricks[]) {
	// First try to load the graphics if any can't be found, exit with false to indicate a failure.
	if (!(_bat = SDL_LoadBMP("/usr/share/breaker/media/bat.bmp"))) return false;
	if (!(_ball = SDL_LoadBMP("/usr/share/breaker/media/ball.bmp"))) return false;
	char filename[] = "/usr/share/breaker/media/brick .bmp";
	for (int i = 0; i < 9; i++) {
		filename[30] = 49 + i;
		if (!(_bricks[i] = SDL_LoadBMP(filename))) return false;
	}

	// I've used pure red - RGB(255,0,0) - as the alpha mask for the sprites so I need to tell SDL to use it.
	Uint32 color_key = SDL_MapRGB(_pixel_format, 255, 0, 0);
    SDL_SetColorKey(_bat, SDL_RLEACCEL | SDL_SRCCOLORKEY, color_key);
    SDL_SetColorKey(_ball, SDL_RLEACCEL | SDL_SRCCOLORKEY, color_key);
    for (int i = 0; i < 9; i++) {
    	SDL_SetColorKey(_bricks[i], SDL_RLEACCEL | SDL_SRCCOLORKEY, color_key);
    }

    // Return with a true value because we've loaded all the graphics successfully.
    return true;
}

bool loadLevel(int _level_no, SDL_Surface* &level_text, TTF_Font* _font, SDL_Surface* _brick_designs[], unsigned int &_brick_count, Brick** &_layout, unsigned int &_total_bricks) {
	const char* filename = convertToString("/usr/share/breaker/levels/level",_level_no,".txt");
	// Open the level file.
	std::ifstream level_file(filename, ifstream::in);
	// Get the level title
	char title[255];
	level_file.getline(title,255);

	// Create a surface for the level title ext
	SDL_Color text_foreground = {255, 255, 255};
	SDL_Color text_background = {0,0,64};
	level_text = TTF_RenderText_Shaded(_font, title, text_foreground, text_background);

	// Get the number of brick lines there are.
	char bls[3];
	level_file.getline(bls,3);
	int brick_lines = atoi(bls);
	char brick_data[brick_lines][14];

	_total_bricks = 0;
	for (int i = 0; i < brick_lines; i++) {
		level_file.getline(brick_data[i],14);
		for (int j = 0; j < 13; j++) {
			char brick_type = brick_data[i][j];
			if (brick_type>='1' && brick_type <='9') {
				_total_bricks++;
				if (brick_type>='1' && brick_type <='8') _brick_count++;
			}
		}
	}

	// Set the origin for the top-left brick.
	int top = 8;
	int left = 8;

	// If _layout is not NULL delete any objects it references.
	if (_layout != NULL) delete _layout;

	// Create an array of brick objects to hold the level layout
	_layout = new Brick*[_total_bricks];
	int brick_index = 0;
	for (int i = 0; i < brick_lines && i < 11; i++) {
		for (int j = 0; j < 13; j++) {
			int brick_type = (brick_data[i][j]) - 49;
			if (brick_type >=0 && brick_type <=8) {
				if (brick_type < 7) {
					_layout[brick_index++] = new Brick(_brick_designs[brick_type], left, top, (8 - (brick_type+1)) * 10);
				} else if (brick_type == 7) {
					_layout[brick_index++] = new Brick(_brick_designs[brick_type], left, top, 100, 3);
				} else if (brick_type == 8) {
					_layout[brick_index++] = new Brick(_brick_designs[brick_type], left, top, 0, -1);
				}
			}
			left += 48;
		}
		left = 8;
		top += 32;
	}

	// Get the numeric flag that indicates whether or not there is a level after this one.
	level_file.getline(bls,2);
	bool another_level = (bls[0] == '1');

	// Close the level file.
	level_file.close();

	// Return true or false to the calling method indicating the existance of another level after this one.
	return another_level;
}

int main ( int argc, char** argv )
{
	// Basic player related game elements
	int score = 0; 				// Player score
	int lives = 3; 				// Player lives
	int level = 1; 				// Current level the player is trying to complete.
	unsigned int bricks_left=0;	// Number of bricks the player has left to destroy before the level is complete.
	unsigned int total_bricks=0;// Records the total number of bricks in the level (inc. indestructible)
	bool new_ball = true; 		// Indicates whether a new ball is waiting to be launched from the bat.
	SDL_Rect batrect;			// Controls the co-ordinates of the player's bat.
	SDL_Rect ballrect;			// Controls the co-ordinates of the player's ball.
	Brick** level_layout=NULL;	// Pointer to an array of Brick objects which describe the layout of the current level.
	/* bat_motion is used to determine the horizontal direction of mation for the bat.
		The player manipulated this value through the pressing the left and right
		cursor keys. */
	int bat_motion = 0;

	/* ball_motion_x and ball_motion_y determine the direction the ball travels. */
	int ball_motion_x = 4, ball_motion_y = -4;

	// Game graphics are referenced by these pointers.
	SDL_Surface *bat, *ball, *bricks[9], *background;

	// Score, level and lives text surfaces.
	SDL_Surface *score_text, *level_text, *lives_text;

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

	// Set the window title
	SDL_WM_SetCaption("Breaker v1.0a - by Morgan Evans", "Breaker");

	// Initialise SDL font library
	if (TTF_Init()==-1) {
		printf( "Unable to initialise TTF libary: %s\n", TTF_GetError());
		return 3;
	}

	// Font and colours to use for drawing fonts.
	TTF_Font* font = TTF_OpenFont("/usr/share/breaker/media/VeraMono.ttf", 14);
	if (!font) {
		printf("Failed to load font properly: %s\n", TTF_GetError());
		return 4;
	}
	SDL_Color text_foreground = {255, 255, 255};
	SDL_Color text_background = {0,0,64};

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window (640x480 is fine for this demo)
    SDL_Surface* screen = SDL_SetVideoMode(640, 480, 32,
                                           SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

	// Let's load the basic game graphics (bat, ball and brick sprites)
	if (!loadBaseGraphics(screen->format, bat, ball, bricks)) {
		printf("Unable to load base graphics.");
		return 2;
	};

    // program main loop
    bool user_quit = false;
    bool next_level_available = true;
    while (next_level_available && lives > 0 && !user_quit)
    {
    	// Place the player's bat centrally and to the bottom of the screen.
		batrect.x = (screen->w - bat->w) / 2;
		batrect.y = screen->h - 48;
		new_ball = true;

    	// Load the level data
    	bricks_left = 0; total_bricks = 0;
		next_level_available = loadLevel(level, level_text, font, bricks, bricks_left, level_layout, total_bricks);
    	while (bricks_left > 0 && !user_quit && lives > 0) {
			// message processing loop
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				// check for messages
				switch (event.type)
				{
				case SDL_QUIT:
					user_quit = true;
					break;
				case SDL_KEYUP:
					bat_motion = 0;
					break;
				case SDL_KEYDOWN:
					{
						switch (event.key.keysym.sym) {
							case SDLK_ESCAPE :
								user_quit = true;
								break;
							case SDLK_LEFT :
								bat_motion = -8;
								break;
							case SDLK_RIGHT :
								bat_motion = 8;
								break;
							case SDLK_SPACE :
								if (new_ball) {
									new_ball = false;
								} break;
							default : break;
						}
					} break;
				}
			} // end of message processing

			// If the bat is set to be in motion move it unless it's at the very edges of the screen.
			if (batrect.x > 0 && bat_motion < 0) batrect.x += bat_motion;
			if (batrect.x < screen->w - 96 && bat_motion > 0) batrect.x += bat_motion;

			// Move the ball.
			if (!new_ball) {
				ballrect.x += ball_motion_x;
				ballrect.y += ball_motion_y;
				if (ballrect.x <= 0) {
					ballrect.x = 0;
					ball_motion_x = -ball_motion_x;
				}

				if (ballrect.x >= screen->w - 16) {
					ballrect.x = screen->w - 16;
					ball_motion_x = -ball_motion_x;
				}

				if (ballrect.y <= 0) {
					ballrect.y = 0;
					ball_motion_y = -ball_motion_y;
				}

				if (ballrect.y >= screen->h - 16) {
					// Place the player's bat centrally and to the bottom of the screen.
					batrect.x = (screen->w - bat->w) / 2;
					batrect.y = screen->h - 48;
					new_ball = true;
					lives--;
				}
			} else {
				ballrect.x = batrect.x + ((batrect.w * 2) / 3);
				ballrect.y = batrect.y - ballrect.h;
				ball_motion_y = (8 + (level - 4) < 8) ? 8 + (level - 4) : 8;
			}

			if (ballrect.x >= batrect.x && ballrect.x + ballrect.w <= batrect.x + batrect.w &&
				ballrect.y + 16 >= batrect.y && ballrect.y + 16 <= batrect.y + (batrect.h / 2) && ball_motion_y > 0) {
					int half_bat = batrect.x + (batrect.w / 2);
					int half_ball = ballrect.x + (ballrect.w / 2);
					ball_motion_x = (half_ball - half_bat) / 6;
					if (ball_motion_x == 0) ball_motion_x = 1;
					ball_motion_y = -ball_motion_y;
			}

			// DRAWING STARTS HERE

			// clear screen
			SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));

			// Draw the bricks
			bool block_hit = false;
			for (unsigned int i = 0; i < total_bricks; i++) {
				Brick* temp_brick = (Brick*) level_layout[i];
				if (temp_brick->isVisible()) {
					SDL_BlitSurface(temp_brick->getSprite(), 0, screen, temp_brick->getRect());
					// Check to see if the ball has collided with the brick.
					SDL_Rect* temp_rect = temp_brick->getRect();
					if (ballrect.x + ballrect.w > temp_rect->x && ballrect.x < temp_rect->x + temp_rect->w
						&& ballrect.y + ballrect.h > temp_rect->y && ballrect.y < temp_rect->y + temp_rect->h && !block_hit) {
							if ((ballrect.x + ballrect.w) - ball_motion_x <= temp_rect->x ||
							ballrect.x - ball_motion_x >= temp_rect->x + temp_rect->w) {
								ball_motion_x = -ball_motion_x;
							} else if ((ballrect.y + ballrect.h) - ball_motion_y <= temp_rect->y ||
							ballrect.y - ball_motion_y >= temp_rect->y + temp_rect->h) {
								ball_motion_y = -ball_motion_y;
							} else {
								ball_motion_y = -ball_motion_y;
							}
						temp_brick->registerHit();
						block_hit = true;
						if (!temp_brick->isVisible()) {
							bricks_left--;
							score += temp_brick->getScore();
						}
					}
				}

			}

			// draw the bat and ball objects.
			SDL_BlitSurface(ball, 0, screen, &ballrect);
			SDL_BlitSurface(bat, 0, screen, &batrect);

			// Draw the score level name and lives at the bottom of the screen;
			const char* score_string = convertToString("Score: ", score);
			score_text = TTF_RenderText_Shaded(font, score_string, text_foreground, text_background);
			SDL_Rect score_rect;
			score_rect.x = 16;
			score_rect.y = screen->h - (score_text->h + 8);
			score_rect.w = score_text->w;
			score_rect.h = score_text->h;
			SDL_BlitSurface(score_text, 0, screen, &score_rect);

			const char* lives_string = convertToString("Lives: ", lives);
			lives_text= TTF_RenderText_Shaded(font, lives_string, text_foreground, text_background);
			SDL_Rect lives_rect;
			lives_rect.x = screen->w - (lives_text->w + 8);
			lives_rect.y = screen->h - (lives_text->h + 8);
			lives_rect.w = lives_text->w;
			lives_rect.h = lives_text->h;
			SDL_BlitSurface(lives_text, 0, screen, &lives_rect);

			SDL_Rect level_rect;
			level_rect.x = (screen->w - level_text->w) / 2;
			level_rect.y = screen->h - (level_text->h + 8);
			level_rect.w = level_text->w;
			level_rect.h = level_text->h;
			SDL_BlitSurface(level_text, 0, screen, &level_rect);

			// DRAWING ENDS HERE

			// finally, update the screen :)
			SDL_Flip(screen);

			// Free the score, lives and level text surfaces (because their content changes).
			SDL_FreeSurface(score_text);
			SDL_FreeSurface(lives_text);

			// Multitask nicely!
			SDL_Delay(20);
    	}

    	if (bricks_left == 0) {
    		level++;
    		SDL_FreeSurface(level_text);
    	}
    } // end main loop

    // free loaded bitmap
    SDL_FreeSurface(bat);
    SDL_FreeSurface(ball);

    for (int i = 0; i < 9; i++) {
    	SDL_FreeSurface(bricks[i]);
    }

	// free ttf font
	TTF_CloseFont(font);
    // all is well ;)
    printf("Exited cleanly\n");
    return 0;
}
