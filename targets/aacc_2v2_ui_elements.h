/*

details information about where textures in aacc_2v2_ui_elements.png are located.
this time, if i ever need a resize, it can be much, much easier
should i do these as defines or as constexpr? i dont want a #width define, but a namespace would let me,,,
ugh

please use the define with the:
    name: var name
    x, y, w, h are the texture coords, in PIXEL quantities

eternal thanks to melissa meant it for this. god is it a lot

*/

namespace UI {

    constexpr float size = 2048.0f;

    #define MAKEUILABEL(name, x, y, w, h) constexpr Rect name(Point(x / size, y / size), w / size, h / size)

    // P0,P2 refers to Left Side Players
    // P1,P3 refers to Right Side Players

    //Top Bars Include Health, Guard Bars for both teammates.
    MAKEUILABEL(P0TopBar,   0,  0, 322, 82);
    MAKEUILABEL(P1TopBar,   0, 84, 322, 82);
    
    //Bottom Bars Include Magic Circuit bars for both teammates.
    MAKEUILABEL(P0BottomBar, 328,  0, 240, 51);
    MAKEUILABEL(P1BottomBar, 326, 55, 240, 51);

    //Broken Magic Circuits, and Guard Bars for each player.
    //Use P0 for P3, and P1 for P2 (so if both players are broken there's a big more visual distinction)
    MAKEUILABEL(P0BrokenCircuit, 0, 168, 198, 12);
    MAKEUILABEL(P1BrokenCircuit, 0, 186, 198, 12);
    
    MAKEUILABEL(P0BrokenGuard, 0, 204, 122, 16);
    MAKEUILABEL(P2BrokenGuard, 0, 226, 122, 16);
    MAKEUILABEL(P1BrokenGuard, 0, 248, 122, 16);
    MAKEUILABEL(P3BrokenGuard, 0, 270, 122, 16);

    // Shade Textures? Not sure what to call these. Feel free to change - Melissa
    MAKEUILABEL(P0HealthShade,    0, 292, 186, 10);
    MAKEUILABEL(P0RedHealthShade, 0, 308, 186, 10);
    MAKEUILABEL(P1HealthShade,    0, 324, 186, 10);
    MAKEUILABEL(P1RedHealthShade, 0, 340, 186, 10);
       // Should be player agnostic, use for everyone.
    MAKEUILABEL(CircuitShade,     0, 356, 192,  8);
    
    // Guard bars are Player specific. Do not substitute.
    MAKEUILABEL(P0GuardShadeBlue,  0, 370, 120, 16);
    MAKEUILABEL(P0GuardShadeWhite, 0, 392, 120, 16);
    MAKEUILABEL(P2GuardShadeBlue,  0, 414, 120, 16);
    MAKEUILABEL(P2GuardShadeWhite, 0, 436, 120, 16);
    MAKEUILABEL(P1GuardShadeBlue,  0, 458, 120, 16);
    MAKEUILABEL(P1GuardShadeWhite, 0, 480, 120, 16);
    MAKEUILABEL(P3GuardShadeBlue,  0, 502, 120, 16);
    MAKEUILABEL(P3GuardShadeWhite, 0, 524, 120, 16);
    
    // Character Select Portraits
    MAKEUILABEL(CSSion,  257*0, 605, 257, 121);
    MAKEUILABEL(CSArc,   257*1, 605, 257, 121);
    MAKEUILABEL(CSCiel,  257*2, 605, 257, 121);
    MAKEUILABEL(CSAki,   257*3, 605, 257, 121);
    MAKEUILABEL(CSKoha,  257*4, 605, 257, 121);
    MAKEUILABEL(CSHisui, 257*5, 605, 257, 121);
    MAKEUILABEL(CSTohno, 257*6, 605, 257, 121);
    MAKEUILABEL(CSMiya,  257*0, 726, 257, 121);
    MAKEUILABEL(CSWara,  257*1, 726, 257, 121);
    MAKEUILABEL(CSNero,  257*2, 726, 257, 121);
    MAKEUILABEL(CSVSion, 257*3, 726, 257, 121);
    MAKEUILABEL(CSWarc,  257*4, 726, 257, 121);
    MAKEUILABEL(CSVaki,  257*5, 726, 257, 121);
    MAKEUILABEL(CSMech,  257*6, 726, 257, 121);
    MAKEUILABEL(CSNanaya,257*0, 847, 257, 121);
    MAKEUILABEL(CSSats,  257*1, 847, 257, 121);
    MAKEUILABEL(CSLen,   257*2, 847, 257, 121);
    MAKEUILABEL(CSPCiel, 257*3, 847, 257, 121);
    MAKEUILABEL(CSNeco,  257*4, 847, 257, 121);
    MAKEUILABEL(CSAoko,  257*5, 847, 257, 121);
    MAKEUILABEL(CSWLen,  257*6, 847, 257, 121);
    MAKEUILABEL(CSNac,   257*0, 968, 257, 121);
    MAKEUILABEL(CSKouma, 257*1, 968, 257, 121);
    MAKEUILABEL(CSSei,   257*2, 968, 257, 121);
    MAKEUILABEL(CSRies,  257*3, 968, 257, 121);
    MAKEUILABEL(CSRoa,   257*4, 968, 257, 121);
    MAKEUILABEL(CSRyougi,257*5, 968, 257, 121);
    MAKEUILABEL(CSHime,  257*6, 968, 257, 121);
    
    // Player Chevrons
    MAKEUILABEL(P0Chevron,  0, 545, 17, 22);
    MAKEUILABEL(P1Chevron, 24, 545, 20, 22);
    MAKEUILABEL(P2Chevron, 51, 545, 21, 22);
    MAKEUILABEL(P3Chevron, 80, 545, 20, 22);
    
    // Moons Icons
    MAKEUILABEL(MoonCrescent, 574, 3,  40, 40);
    MAKEUILABEL(MoonFull,     615, 3,  40, 40);
    MAKEUILABEL(MoonHalf,     574, 43, 36, 36);
    MAKEUILABEL(MoonEclipse,  615, 44, 40, 40);
    
    // Round Icons
    MAKEUILABEL(RoundWinV,     120, 554, 25, 25);
    MAKEUILABEL(RoundWinS,     151, 554, 25, 25);
    MAKEUILABEL(RoundWinT,     182, 554, 25, 25);
    MAKEUILABEL(RoundWinBlank, 208, 554, 25, 25);
    
    // Character Resource Icons
    MAKEUILABEL(MaidsHeartFull,   0, 572, 15, 15);
    MAKEUILABEL(MaidsHeartEmpty, 21, 572, 15, 15);
    MAKEUILABEL(RoaLightning,    42, 572, 11, 16);
    MAKEUILABEL(SionBullet,      59, 572,  5, 14);
    
};