
/*

details information about where textures in aacc_2v2_ui_elements.png are located.
this time, if i ever need a resize, it can be much, much easier
should i do these as defines or as constexpr? i dont want a #width define, but a namespace would let me,,,
ugh

please use the define with the:
    name: var name
    x, y, w, h are the texture coords, in PIXEL quantities

*/

namespace UI {

    constexpr float size = 1024.0f;

    #define MAKEUILABEL(name, x, y, w, h) constexpr Rect name(Point(x, y), w, h)

    MAKEUILABEL(test, 0, 0, 0.5, 0.5);



};

