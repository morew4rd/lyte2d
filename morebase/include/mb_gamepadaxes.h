/* (c) mg */

#ifndef MB_GAMEPADAXES_H_INCLUDED
#define MB_GAMEPADAXES_H_INCLUDED

// Same as GLFW3

typedef enum M_GamepadAxis {
    M_GAMEPADAXIS_LEFT_X       =0,
    M_GAMEPADAXIS_LEFT_Y       =1,
    M_GAMEPADAXIS_RIGHT_X      =2,
    M_GAMEPADAXIS_RIGHT_Y      =3,
    M_GAMEPADAXIS_LEFT_TRIGGER =4,
    M_GAMEPADAXIS_RIGHT_TRIGGER=5,
    M_NUM_GAMEPADAXISES        =6,
} M_GamepadAxis;

#endif  /* MB_GAMEPADAXES_H_INCLUDED */
