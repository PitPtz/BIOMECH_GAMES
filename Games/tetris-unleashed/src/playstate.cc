// Copyright [2015] <Chafic Najjar>

#include "src/playstate.h"

#include <random>

#include "src/game_engine.h"
#include "src/tetromino.h"
#include "src/board.h"
#include "src/utilities.h"

// This will prevent linker errors in case the same names are used
// in other files.
namespace {
    std::random_device rd;
    std::mt19937 gen(rd());
}

PlayState PlayState::m_playstate;

void PlayState::init(GameEngine* game) {
    // Game objects.
    board        = new Board();
    tetro        = new Tetromino(rand()%7);       // Current tetromino.
    next_tetro   = new Tetromino(rand()%7);       // Next tetromino.

    ready_4_input = true;
    ready_4_input_rotate = true;
    game_starttime = SDL_GetTicks();
    dt_lastInput = SDL_GetTicks() + 5000;

    // Music.
    music_engine = irrklang::createIrrKlangDevice();
    music_engine->play2D("resources/sounds/Dubmood-Tetris.ogg", true);

    // Texture.
    block_texture = load_texture("resources/sprites/block.bmp", game->renderer);

    // Fonts.
    TTF_Init();
    white = { 255, 255, 255 };
    font_pause = TTF_OpenFont("resources/fonts/bitwise.ttf", 16);
    font_tetris = TTF_OpenFont("resources/fonts/bitwise.ttf", 16);
    font_score_text = TTF_OpenFont("resources/fonts/bitwise.ttf", 20);
    font_score = TTF_OpenFont("resources/fonts/bitwise.ttf", 20);
    font_new_game = TTF_OpenFont("resources/fonts/bitwise.ttf", 20);
    font_quit = TTF_OpenFont("resources/fonts/bitwise.ttf", 20);
    font_game_over = TTF_OpenFont("resources/fonts/bitwise.ttf", 16);

    font_image_pause = render_text("Pause",
            white, font_pause, game->renderer);
    font_image_tetris = render_text("Tetris Unleashed!",
            white, font_tetris, game->renderer);
    font_image_score_text = render_text("Score: ",
            white, font_score_text, game->renderer);
    font_image_score = render_text(std::to_string(board->get_score()),
        white, font_score, game->renderer);
    font_image_new_game = render_text("New game",
            white, font_new_game, game->renderer);
    font_image_quit = render_text("Quit",
            white, font_quit, game->renderer);
    font_image_game_over = render_text("Game over!",
            white, font_game_over, game->renderer);

    // Frame rate.
    acceleration    = 0.005f;
    this_time       = 0;
    last_time       = 0;
    time_till_drop  = 0.3f;
    time_counter    = 0.0f;

    // Buttons status.
    newgamedown     = false;
    newgameup       = false;
    quitdown        = false;
    quitup          = false;

    // Buttons coordinates.
    newgamex1       = GAME_OFFSET+board->WIDTH+board->BLOCK_WIDTH;
    newgamex2       = GAME_OFFSET+board->WIDTH+8*board->BLOCK_WIDTH;
    newgamey1       = board->HEIGHT-4*board->BLOCK_HEIGHT;
    newgamey2       = board->HEIGHT-6*board->BLOCK_HEIGHT;

    paused          = false;
    game_over       = false;
    exit            = false;

    // At the start of the game:
    // x position of (0, 0) block of tetro is int(15/2) = 7
    // which is the exact horizontal middle of board.
    // y position of (0, 0) block of tetro is 0 which is the top of the board.
    tetro->set_position(static_cast<int>(board->COLS/2), 0);

    // Position next_tetro at the upper right of the window,
    // outside of the board.
    next_tetro->set_position(board->COLS+5, static_cast<int>(0.3*board->ROWS));
}

void PlayState::clean_up(GameEngine* game) {
    // Delete music engine.
    music_engine->drop();

    TTF_CloseFont(font_pause);
    TTF_CloseFont(font_tetris);
    TTF_CloseFont(font_score_text);
    TTF_CloseFont(font_score);
    TTF_CloseFont(font_new_game);
    TTF_CloseFont(font_quit);
    TTF_CloseFont(font_game_over);

    SDL_DestroyTexture(font_image_pause);
    SDL_DestroyTexture(font_image_tetris);
    SDL_DestroyTexture(font_image_score_text);
    SDL_DestroyTexture(font_image_score);
    SDL_DestroyTexture(font_image_new_game);
    SDL_DestroyTexture(font_image_quit);
    SDL_DestroyTexture(font_image_game_over);

    IMG_Quit();

    SDL_DestroyRenderer(game->renderer);
    SDL_DestroyWindow(game->window);
    SDL_Quit();
}

void PlayState::pause() {
    music_engine->setAllSoundsPaused(true);
    paused = true;
}

void PlayState::resume() {
    music_engine->setAllSoundsPaused(false);
    paused = false;
}

// Restarts game.
void PlayState::reset() {
    // Empty the board.
    for (int i = 0; i < board->ROWS; i++) {
        for (int j = 0; j < board->COLS; j++) {
            board->color[i][j] = -1;
        }
    }

    // Delete objects.
    delete [] board;
    delete [] tetro;
    delete [] next_tetro;

    // Recreate objects.
    board = new Board();
    tetro = new Tetromino(rand()%7);
    next_tetro = new Tetromino(rand()%7);
    tetro->set_position(static_cast<int>(board->COLS/2), 0);
    next_tetro->set_position(board->COLS+5, static_cast<int>(0.3*board->ROWS));

    // Restart music.
    music_engine->stopAllSounds();
    music_engine->play2D("resources/sounds/Dubmood-Tetris.ogg", true);

    game_over       = false;
    newgameup       = false;
    newgamedown     = false;

    paused = false;
}

// Handle player input.
void PlayState::input(GameEngine *game) {

    Sint16 js_axis_0, js_axis_1;

    // Checking for joystick state
    js_axis_0 = SDL_JoystickGetAxis(SDL_JoystickOpen(0), 0);
    js_axis_1 = SDL_JoystickGetAxis(SDL_JoystickOpen(0), 1);

    if(ready_4_input && js_axis_0 > 20000){
        tetro->movement = tetro->LEFT;
        tetro->shift = true;
        ready_4_input = false;
        dt_lastInput = SDL_GetTicks();
    }
    else if(ready_4_input && js_axis_0 < -20000){
        tetro->movement = tetro->RIGHT;
        tetro->shift = true;
        ready_4_input = false;
        dt_lastInput = SDL_GetTicks();

    }
    else if(ready_4_input_rotate && js_axis_1 > 20000){
        if (tetro->type != 2)
            tetro->rotate = true;
        ready_4_input_rotate = false;
    }
    else if(ready_4_input && js_axis_1 < -20000){
        tetro->speed_up = true;
        ready_4_input = false;
        dt_lastInput = SDL_GetTicks();
    }
    else if(abs(js_axis_0) < 10000 && abs(js_axis_1) < 10000){
        ready_4_input_rotate = true; 
    }
    else if(ready_4_input == false && SDL_GetTicks() - dt_lastInput > 250){
        ready_4_input = true; 
    }
    if(SDL_JoystickGetButton(SDL_JoystickOpen(0), 0) && game_starttime <= SDL_GetTicks()-2000){
        exit = true;
    }

    // Queuing events.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Clicking 'x' or pressing F4.
        if (event.type == SDL_QUIT) {
            exit = true;
        }

        // Key is pressed.
        if (event.type == SDL_KEYDOWN) {
            // Pause/Resume.
            if (event.key.keysym.sym == SDLK_p) {
                if (paused) {
                    resume();
                } else {
                    pause();
                }
            }

            if (!paused && !tetro->free_fall) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        exit = true;
                        break;
                    case SDLK_a: case SDLK_LEFT:
                        tetro->movement = tetro->LEFT;
                        tetro->shift = true;
                        break;
                    case SDLK_d: case SDLK_RIGHT:
                        tetro->movement = tetro->RIGHT;
                        tetro->shift = true;
                        break;
                    case SDLK_w: case SDLK_UP:
                        if (tetro->type != 2)  // type 3 is O-Block.
                            tetro->rotate = true;
                        break;
                    case SDLK_s: case SDLK_DOWN:
                        tetro->speed_up = true;
                        break;
                    case SDLK_SPACE:
                        tetro->free_fall = true;
                        break;
                    default:
                        break;
                }
            }
        }

        // Key is released.
        if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_s: case SDLK_DOWN:
                    tetro->speed_up = false;
                    break;
                default:
                    break;
            }
        }

        // Mouse moves.
        if (event.type == SDL_MOUSEMOTION) {
            // Outside of the board.
            if (event.motion.x > board->WIDTH + GAME_OFFSET)
                SDL_ShowCursor(1);  // Show cursor.

            // Inside the board.
            else
                SDL_ShowCursor(0);  // Don't show cursor.
        }

        // Mouse button clicked.
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            switch (event.button.button) {
                // Left mouse button clicked.
                case SDL_BUTTON_LEFT: {
                    int x = event.button.x;
                    int y = event.button.y;
                    if (x > newgamex1 &&
                        x < newgamex2) {
                        // And mouse cursor is on "New Game" button.
                        if (y > newgamey2 &&
                            y < newgamey1) {
                            newgamedown = true;
                        // And mouse cursor is on "Quit" button.
                        } else if (y > newgamey2+4*board->BLOCK_HEIGHT &&
                                   y < newgamey1+4*board->BLOCK_HEIGHT) {
                            quitdown = true;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        // Mouse button released.
        if (event.type == SDL_MOUSEBUTTONUP) {
            switch (event.button.button) {
                // Left mouse button released.
                case SDL_BUTTON_LEFT: {
                    int x = event.button.x;
                    int y = event.button.y;
                    if (x > newgamex1 && x < newgamex2) {
                        // And mouse cursor is on "New Game" button.
                        if (y > newgamey2 && y < newgamey1) {
                            newgameup = true;
                        // And mouse cursor is on "Quit" button.
                        } else if (y > newgamey2+4*board->BLOCK_HEIGHT &&
                                 y < newgamey1+4*board->BLOCK_HEIGHT) {
                            quitup = true;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void PlayState::release_tetromino() {
    Tetromino* new_tetro = new Tetromino(rand()%7);
    new_tetro->set_position(next_tetro->x, next_tetro->y);

    delete[] tetro;
    tetro = next_tetro;
    tetro->set_position(static_cast<int>(board->COLS/2), 0);

    next_tetro = new_tetro;

    tetro->drop();
}

// Update game values.
void PlayState::update(GameEngine* game) {
    // New Game button was pressed.
    if (newgameup && newgamedown) {
        reset();
    }

    // Quit button or 'x'/F4 was pressed.
    if ((quitup && quitdown) || exit) {
        game->quit();
    }

    if (game_over || paused) {
        return;
    }

    // Tetromino has landed.
    if (tetro->has_landed()) {
        tetro->free_fall = false;

        // Add fallen tetromino to the board and check if tetromino.
        // has crossed over the top border.
        if (!board->add(tetro)) {
            game_over = true;
            return;
        }

        // Drop stored tetromino and replace by newly-generated tetromino.
        release_tetromino();
    } else if (tetro->free_fall) {
        tetro->y++;  // Maximum speed.
    } else {  // Rotations and translations.
        // Rotation.
        if (tetro->rotate)
            tetro->rotate_left();

        // Update tetromino position on the x-axis.
        tetro->add_to_x(tetro->movement);

        // Assign the time required for tetromino to fall down one block.
        if (tetro->speed_up) {
            time_till_drop = 0.02f;  // 2x slower than free fall.
        } else {
            // Drop speed proportional to score.
            time_till_drop = 0.3f - board->get_score()*acceleration;
        }

        // Add time elapsed.
        time_counter += frame_rate(game, &last_time, &this_time);

        // time_till_drop = 0.3;
        // delta_time ~ 0.017 seconds.
        // Tetromino initially falls one block unit for
        // every 0.3/0.017 ~ 17 game loops.

        if (time_counter >= time_till_drop) {
            tetro->y++;  // Update tetromino position on the y-axis.
            time_counter = 0.0f;
        }
    }

    // Collision detection.
    // Check if tetromino is in an acceptable position,
    // if not, undo previous move(s).
    for (int i = 0; i < tetro->SIZE; i++) {
        // Coordinates of each block.
        int x = tetro->get_block_x(i);
        int y = tetro->get_block_y(i);

        // Block crosses wall after rotation and/or translation.
        if (x < 0 || x >= board->COLS) {
            // Because of rotation.
            if (tetro->rotate)
                tetro->rotate_right();  // Neutralize the left rotation.

            // Because of translation.
            if (tetro->shift)
                tetro->x -= tetro->movement;  // Neutralize shift.

            break;
        } else if (y >= board->ROWS) {  // Block touches ground.
            tetro->lands();
            // Change the value of Y so that block(s) of the (old)
            // tetromino is/are above the blue line.
            tetro->set_block_y(i, board->ROWS-1);
        } else if (y >= 0) {  // Block is on the board.
            // Block touched another block.
            if (board->color[y][x] != -1) {
                // Tetromino rotates and collides with a block.
                if (tetro->rotate || tetro->shift) {
                    if (tetro->rotate) {
                        tetro->rotate_right();  // Neutralize.
                    }
                    // Tetromino is shifted into another block.
                    if (tetro->shift) {
                        tetro->x -= tetro->movement;  // Neutralize.
                    }
                    break;
                } else {  // Block falls into another block.
                    tetro->y--;  // Neutralize: tetromino goes up.
                    tetro->lands();
                }
            }
        }
    }
    board->delete_full_rows();
    tetro->rotate = false;
    tetro->shift = false;
    tetro->movement = tetro->NONE;
}

// Render result.
void PlayState::render(GameEngine* game) {
    // Clear screen.
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 1);
    SDL_RenderClear(game->renderer);

    // Render "Tetris" text.
    int x = (next_tetro->x-3)*board->BLOCK_WIDTH;
    int y = GAME_OFFSET;

    render_texture(font_image_tetris, game->renderer, x, y);

    // Render "Pause" text if game is paused.
    if (paused)
        render_texture(font_image_pause, game->renderer, x, y+40);

    // Render score text.
    render_texture(font_image_score_text,
            game->renderer, x, y + board->BLOCK_WIDTH);

    // Render score.
    if (board->render_score) {
        font_image_score = render_text(std::to_string(board->get_score()),
                white, font_score, game->renderer);
        board->render_score = false;
    }
    render_texture(font_image_score,
            game->renderer, x + 60, y + board->BLOCK_WIDTH);

    int tetro_x, tetro_y;

    // Prepare textures.
    int iW, iH;
    SDL_QueryTexture(block_texture, nullptr, nullptr, &iW, &iH);

    SDL_Rect clips[7];
    for (int i = 0; i < 7; i++) {
        clips[i].x = 0;
        clips[i].y = i*24;
        clips[i].w = 20;
        clips[i].h = 20;
    }

    // Draw tetromino squares.
    for (int i = 0; i < tetro->SIZE; i++) {
        // Get new coordinates.
        tetro_x = tetro->get_block_x(i)*board->BLOCK_WIDTH + GAME_OFFSET;
        tetro_y = tetro->get_block_y(i)*board->BLOCK_HEIGHT + GAME_OFFSET;

        draw_block(game, tetro_x, tetro_y, tetro->type, clips);
    }

    // Draw shadow tetromino.
    int shadow_y[4];
    tetro->get_shadow(board, shadow_y);
    for (int i = 0; i < tetro->SIZE; i++) {
        if (shadow_y[i] < 0)
            break;
        int x = tetro->get_block_x(i)*board->BLOCK_WIDTH + GAME_OFFSET;
        int y = shadow_y[i]*board->BLOCK_WIDTH + GAME_OFFSET;

        // Draw block.
        SDL_SetRenderDrawColor(game->renderer, 180, 180, 180, 255);
        SDL_Rect shadow_block = {x, y, board->BLOCK_WIDTH, board->BLOCK_HEIGHT};
        SDL_RenderFillRect(game->renderer, &shadow_block);
    }

    if (!game_over) {
        // Draw next tetromino.
        for (int i = 0; i < next_tetro->SIZE; i++) {
            // Get new coordinates.
            tetro_x = next_tetro->get_block_x(i)*board->BLOCK_WIDTH;
            tetro_y = next_tetro->get_block_y(i)*board->BLOCK_HEIGHT;

            draw_block(game, tetro_x, tetro_y, next_tetro->type, clips);
        }
    }

    // This is the board. Non-active tetrominos live here.
    for (int i = 0; i < board->ROWS; i++)
        for (int j = 0; j < board->COLS; j++)
            if (board->color[i][j] != -1) {
                // Get new coordinates.
                tetro_x = j*board->BLOCK_WIDTH + GAME_OFFSET;
                tetro_y = i*board->BLOCK_HEIGHT + GAME_OFFSET;

                draw_block(game, tetro_x, tetro_y, board->color[i][j], clips);
            }

    // Box surrounding board.

    // Set color to white.
    SDL_SetRenderDrawColor(game->renderer, 180, 180, 180, 255);

    // Draw left border.
    SDL_RenderDrawLine(game->renderer,
        GAME_OFFSET, GAME_OFFSET, GAME_OFFSET, GAME_OFFSET+board->HEIGHT);

    // Draw right border.
    SDL_RenderDrawLine(game->renderer,
            GAME_OFFSET+board->WIDTH, GAME_OFFSET,
            GAME_OFFSET+board->WIDTH, GAME_OFFSET+board->HEIGHT);

    // Draw upper border.
    SDL_RenderDrawLine(game->renderer,
            GAME_OFFSET, GAME_OFFSET, GAME_OFFSET+board->WIDTH, GAME_OFFSET);

    // Draw bottom border.
    SDL_RenderDrawLine(game->renderer,
            GAME_OFFSET, GAME_OFFSET+board->HEIGHT,
            GAME_OFFSET+board->WIDTH, GAME_OFFSET+board->HEIGHT);

    // If game is over, display "Game Over!".
    if (game_over)
        render_texture(font_image_game_over,
                game->renderer, newgamex1,
                game->height-newgamey1+4*board->BLOCK_WIDTH);

    // Create "New Game" button.
    int blue[4] = {0, 0, 255, 255};
    create_button(game, newgamex1, newgamey2,
            7*board->BLOCK_WIDTH, 2*board->BLOCK_HEIGHT, blue);

    // Render "New Game" font.
    render_texture(font_image_new_game,
            game->renderer, newgamex1+10, newgamey2+10);

    // Create "Quit" button.
    int red[4] = {255, 0, 0, 255};
    create_button(game, newgamex1,
            newgamey2+4*board->BLOCK_HEIGHT, 7*board->BLOCK_WIDTH,
            2*board->BLOCK_HEIGHT, red);

    // Render "Quit" font.
    render_texture(font_image_quit,
            game->renderer, newgamex1+10, newgamey2+4*board->BLOCK_HEIGHT+10);

    // Swap buffers.
    SDL_RenderPresent(game->renderer);
}

// Create "New Game" and "Quit" buttons.
void PlayState::create_button(GameEngine* game,
        int x, int y, int width, int height, int color[]) {
    SDL_Rect rect = { x, y, width, height };
    SDL_SetRenderDrawColor(game->renderer,
            color[0], color[1], color[2], color[3]);
    SDL_RenderFillRect(game->renderer, &rect);
}

// Render Tetromino block.
void PlayState::draw_block(GameEngine* game,
        int x, int y, int k, SDL_Rect clips[]) {
    render_texture(block_texture, game->renderer, x, y, &clips[k]);
}

float PlayState::frame_rate(GameEngine* game, int* last_time, int* this_time) {
    // Get number of milliseconds since SDL_Init() of the previous frame.
    *last_time = *this_time;

    // Get number of milliseconds since SDL_Init().
    *this_time = SDL_GetTicks();

    // Variation of time between each game iteration.
    // Dividing by 1000 to convert in seconds.
    return ((*this_time - *last_time) / 1000.0f);
}
