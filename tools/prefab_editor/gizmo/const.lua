local m = {
    SELECT                  = 0,
    MOVE                    = 1,
    ROTATE                  = 2,
    SCALE                   = 3,
    DIR_X                   = {1, 0, 0},
    DIR_Y                   = {0, 1, 0},
    DIR_Z                   = {0, 0, 1},
    COLOR_X                 = {7, 0, 0, 1},
    COLOR_Y                 = {0, 7, 0, 1},
    COLOR_Z                 = {0, 0, 7, 1},
    COLOR_X_ALPHA           = {7, 0, 0, 0.5},
    COLOR_Y_ALPHA           = {0, 7, 0, 0.5},
    COLOR_Z_ALPHA           = {0, 0, 7, 0.5},
    COLOR_GRAY              = {3.5, 3.5, 3.5, 1},
    COLOR_GRAY_ALPHA        = {3.5, 3.5, 3.5, 0.5},
    HIGHTLIGHT_COLOR        = {7, 7, 0, 1},
    HIGHTLIGHT_COLOR_ALPHA  = {7, 7, 0, 0.5},
    RIGHT_TOP               = 0,
    RIGHT_BOTTOM            = 1,
    LEFT_BOTTOM             = 2,
    LEFT_TOP                = 3,
    AXIS_CUBE_SCALE         = 2.5,
    AXIS_LEN                = 0.2,
    UNIFORM_ROT_AXIS_LEN    = 0.25,
    MOVE_PLANE_SCALE        = 0.08,
    MOVE_PLANE_OFFSET       = 0.04,
    MOVE_PLANE_HIT_RADIUS   = 0.22,
    ROTATE_SLICES           = 72,
    ROTATE_HIT_RADIUS       = 0.02,
    MOVE_HIT_RADIUS_PIXEL   = 10
}
return m