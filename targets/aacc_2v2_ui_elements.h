
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

    #define MAKEUILABEL(name, x, y, w, h) constexpr Rect name(Point((x) / size, (y) / size), (w) / size, (h) / size)

    MAKEUILABEL(P0TopBar, 0, 0, 322, 82);
    MAKEUILABEL(P1TopBar, 0, 85, 322, 82);

    MAKEUILABEL(P0Meter, 325,   0, 565-325, 50);
    MAKEUILABEL(P1Meter, 325, 223, 565-325, 50);



};

