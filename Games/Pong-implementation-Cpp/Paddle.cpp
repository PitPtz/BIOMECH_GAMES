#include "Paddle.h"

#define JOYSTICK_DEADZONE 1000
#define JOYSTICK_UPERLIM  20000.0

void Paddle::changePaddlePosition(bool sdlKeyboardStateUp, bool sdlKeyBoardStateDown, float deltaTime) {

    // Implementing movement up and down for corresponding key presses
    _mPaddleDirection = 0;
    if (sdlKeyboardStateUp)
        _mPaddleDirection -= 1;
    if (sdlKeyBoardStateDown)
        _mPaddleDirection += 1;

    // Implementing paddle movement and clipping position if paddle reaches window border
    if (static_cast<int>(_position.y) + _length / 2 > _windowLimitY) {
        _position.y = static_cast<float>(_windowLimitY) - static_cast<float>(_length) / 2;
    }
    else if (static_cast<int>(_position.y) - _length / 2 < 0) {
        _position.y = 0 + static_cast<float>(_length) / 2;
    }
    else
        _position.y += static_cast<float>(_mPaddleDirection) * _mPaddleMovingSpeed * deltaTime;
}

void Paddle::changePaddlePositionJoy(Sint16 js_state, float deltaTime) {

    // Implementing movement up and down for corresponding joystick state
    _mPaddleDirection = 0; // Resetting paddle direction
    if (abs(js_state) > JOYSTICK_DEADZONE){
        if (js_state > 0)
            _mPaddleDirection += std::max(-2.0, (js_state + JOYSTICK_DEADZONE) / JOYSTICK_UPERLIM);
        else if (js_state < 0)
            _mPaddleDirection += std::min(2.0, (js_state - JOYSTICK_DEADZONE) / JOYSTICK_UPERLIM);
    }

    // Implementing paddle movement and clipping position if paddle reaches window border
    if (static_cast<int>(_position.y) + _length / 2 > _windowLimitY) {
        _position.y = static_cast<float>(_windowLimitY) - static_cast<float>(_length) / 2;
    }
    else if (static_cast<int>(_position.y) - _length / 2 < 0) {
        _position.y = 0 + static_cast<float>(_length) / 2;
    }
    else
        _position.y += static_cast<float>(_mPaddleDirection) * _mPaddleMovingSpeed * deltaTime;
}

float* Paddle::getPaddleDirection() { return &_mPaddleDirection; }

float* Paddle::getPaddleMovingSpeed() { return &_mPaddleMovingSpeed; }
