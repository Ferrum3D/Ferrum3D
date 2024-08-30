#pragma once
#include <FeCore/Math/Color.h>

namespace FE
{
    //! @brief Named colors from CSS3 specification
    struct Colors
    {
        //=========================================================================================
        // Pink colors
        const static Color MediumVioletRed; //!< CSS spec: rgb(199, 21,  133)
        const static Color DeepPink;        //!< CSS spec: rgb(255, 20,  147)
        const static Color PaleVioletRed;   //!< CSS spec: rgb(219, 112, 147)
        const static Color HotPink;         //!< CSS spec: rgb(255, 105, 180)
        const static Color LightPink;       //!< CSS spec: rgb(255, 182, 193)
        const static Color Pink;            //!< CSS spec: rgb(255, 192, 203)

        //=========================================================================================
        // Red colors
        const static Color DarkRed;     //!< CSS spec: rgb(139, 0,   0)
        const static Color Red;         //!< CSS spec: rgb(255, 0,   0)
        const static Color Firebrick;   //!< CSS spec: rgb(178, 34,  34)
        const static Color Crimson;     //!< CSS spec: rgb(220, 20,  60)
        const static Color IndianRed;   //!< CSS spec: rgb(205, 92,  92)
        const static Color LightCoral;  //!< CSS spec: rgb(240, 128, 128)
        const static Color Salmon;      //!< CSS spec: rgb(250, 128, 114)
        const static Color DarkSalmon;  //!< CSS spec: rgb(233, 150, 122)
        const static Color LightSalmon; //!< CSS spec: rgb(255, 160, 122)

        //=========================================================================================
        // Orange colors
        const static Color OrangeRed;  //!< CSS spec: rgb(255, 69,  0)
        const static Color Tomato;     //!< CSS spec: rgb(255, 99,  71)
        const static Color DarkOrange; //!< CSS spec: rgb(255, 140, 0)
        const static Color Coral;      //!< CSS spec: rgb(255, 127, 80)
        const static Color Orange;     //!< CSS spec: rgb(255, 165, 0)

        //=========================================================================================
        // Yellow colors
        const static Color DarkKhaki;            //!< CSS spec: rgb(189, 183, 107)
        const static Color Gold;                 //!< CSS spec: rgb(255, 215, 0)
        const static Color Khaki;                //!< CSS spec: rgb(240, 230, 140)
        const static Color PeachPuff;            //!< CSS spec: rgb(255, 218, 185)
        const static Color Yellow;               //!< CSS spec: rgb(255, 255, 0)
        const static Color PaleGoldenrod;        //!< CSS spec: rgb(238, 232, 170)
        const static Color Moccasin;             //!< CSS spec: rgb(255, 228, 181)
        const static Color PapayaWhip;           //!< CSS spec: rgb(255, 239, 213)
        const static Color LightGoldenrodYellow; //!< CSS spec: rgb(250, 250, 210)
        const static Color LemonChiffon;         //!< CSS spec: rgb(255, 250, 205)
        const static Color LightYellow;          //!< CSS spec: rgb(255, 255, 224)

        //=========================================================================================
        // Brown colors
        const static Color Maroon;         //!< CSS spec: rgb(128, 0,   0)
        const static Color Brown;          //!< CSS spec: rgb(165, 42,  42)
        const static Color SaddleBrown;    //!< CSS spec: rgb(139, 69,  19)
        const static Color Sienna;         //!< CSS spec: rgb(160, 82,  45)
        const static Color Chocolate;      //!< CSS spec: rgb(210, 105, 30)
        const static Color DarkGoldenrod;  //!< CSS spec: rgb(184, 134, 11)
        const static Color Peru;           //!< CSS spec: rgb(205, 133, 63)
        const static Color RosyBrown;      //!< CSS spec: rgb(188, 143, 143)
        const static Color Goldenrod;      //!< CSS spec: rgb(218, 165, 32)
        const static Color SandyBrown;     //!< CSS spec: rgb(244, 164, 96)
        const static Color Tan;            //!< CSS spec: rgb(210, 180, 140)
        const static Color Burlywood;      //!< CSS spec: rgb(222, 184, 135)
        const static Color Wheat;          //!< CSS spec: rgb(245, 222, 179)
        const static Color NavajoWhite;    //!< CSS spec: rgb(255, 222, 173)
        const static Color Bisque;         //!< CSS spec: rgb(255, 228, 196)
        const static Color BlanchedAlmond; //!< CSS spec: rgb(255, 235, 205)
        const static Color Cornsilk;       //!< CSS spec: rgb(255, 248, 220)

        //=========================================================================================
        // Green colors
        const static Color DarkGreen;         //!< CSS spec: rgb(0,   100, 0)
        const static Color Green;             //!< CSS spec: rgb(0,   128, 0)
        const static Color DarkOliveGreen;    //!< CSS spec: rgb(85,  107, 47)
        const static Color ForestGreen;       //!< CSS spec: rgb(34,  139, 34)
        const static Color SeaGreen;          //!< CSS spec: rgb(46,  139, 87)
        const static Color Olive;             //!< CSS spec: rgb(128, 128, 0)
        const static Color OliveDrab;         //!< CSS spec: rgb(107, 142, 35)
        const static Color MediumSeaGreen;    //!< CSS spec: rgb(60,  179, 113)
        const static Color LimeGreen;         //!< CSS spec: rgb(50,  205, 50)
        const static Color Lime;              //!< CSS spec: rgb(0,   255, 0)
        const static Color SpringGreen;       //!< CSS spec: rgb(0,   255, 127)
        const static Color MediumSpringGreen; //!< CSS spec: rgb(0,   250, 154)
        const static Color DarkSeaGreen;      //!< CSS spec: rgb(143, 188, 143)
        const static Color MediumAquamarine;  //!< CSS spec: rgb(102, 205, 170)
        const static Color YellowGreen;       //!< CSS spec: rgb(154, 205, 50)
        const static Color LawnGreen;         //!< CSS spec: rgb(124, 252, 0)
        const static Color Chartreuse;        //!< CSS spec: rgb(127, 255, 0)
        const static Color LightGreen;        //!< CSS spec: rgb(144, 238, 144)
        const static Color GreenYellow;       //!< CSS spec: rgb(173, 255, 47)
        const static Color PaleGreen;         //!< CSS spec: rgb(152, 251, 152)

        //=========================================================================================
        // Cyan colors
        const static Color Teal;            //!< CSS spec: rgb(0,   128, 128)
        const static Color DarkCyan;        //!< CSS spec: rgb(0,   139, 139)
        const static Color LightSeaGreen;   //!< CSS spec: rgb(32,  178, 170)
        const static Color CadetBlue;       //!< CSS spec: rgb(95,  158, 160)
        const static Color DarkTurquoise;   //!< CSS spec: rgb(0,   206, 209)
        const static Color MediumTurquoise; //!< CSS spec: rgb(72,  209, 204)
        const static Color Turquoise;       //!< CSS spec: rgb(64,  224, 208)
        const static Color Aqua;            //!< CSS spec: rgb(0,   255, 255)
        const static Color Cyan;            //!< CSS spec: rgb(0,   255, 255)
        const static Color Aquamarine;      //!< CSS spec: rgb(127, 255, 212)
        const static Color PaleTurquoise;   //!< CSS spec: rgb(175, 238, 238)
        const static Color LightCyan;       //!< CSS spec: rgb(224, 255, 255)

        //=========================================================================================
        // Blue colors
        const static Color Navy;           //!< CSS spec: rgb(0,   0,   128)
        const static Color DarkBlue;       //!< CSS spec: rgb(0,   0,   139)
        const static Color MediumBlue;     //!< CSS spec: rgb(0,   0,   205)
        const static Color Blue;           //!< CSS spec: rgb(0,   0,   255)
        const static Color MidnightBlue;   //!< CSS spec: rgb(25,  25,  112)
        const static Color RoyalBlue;      //!< CSS spec: rgb(65,  105, 225)
        const static Color SteelBlue;      //!< CSS spec: rgb(70,  130, 180)
        const static Color DodgerBlue;     //!< CSS spec: rgb(30,  144, 255)
        const static Color DeepSkyBlue;    //!< CSS spec: rgb(0,   191, 255)
        const static Color CornflowerBlue; //!< CSS spec: rgb(100, 149, 237)
        const static Color SkyBlue;        //!< CSS spec: rgb(135, 206, 235)
        const static Color LightSkyBlue;   //!< CSS spec: rgb(135, 206, 250)
        const static Color LightSteelBlue; //!< CSS spec: rgb(176, 196, 222)
        const static Color LightBlue;      //!< CSS spec: rgb(173, 216, 230)
        const static Color PowderBlue;     //!< CSS spec: rgb(176, 224, 230)

        //=========================================================================================
        // Purple, violet, and magenta colors
        const static Color Indigo;          //!< CSS spec: rgb(75,  0,   130)
        const static Color Purple;          //!< CSS spec: rgb(128, 0,   128)
        const static Color DarkMagenta;     //!< CSS spec: rgb(139, 0,   139)
        const static Color DarkViolet;      //!< CSS spec: rgb(148, 0,   211)
        const static Color DarkSlateBlue;   //!< CSS spec: rgb(72,  61,  139)
        const static Color BlueViolet;      //!< CSS spec: rgb(138, 43,  226)
        const static Color DarkOrchid;      //!< CSS spec: rgb(153, 50,  204)
        const static Color Fuchsia;         //!< CSS spec: rgb(255, 0,   255)
        const static Color Magenta;         //!< CSS spec: rgb(255, 0,   255)
        const static Color SlateBlue;       //!< CSS spec: rgb(106, 90,  205)
        const static Color MediumSlateBlue; //!< CSS spec: rgb(123, 104, 238)
        const static Color MediumOrchid;    //!< CSS spec: rgb(186, 85,  211)
        const static Color MediumPurple;    //!< CSS spec: rgb(147, 112, 219)
        const static Color Orchid;          //!< CSS spec: rgb(218, 112, 214)
        const static Color Violet;          //!< CSS spec: rgb(238, 130, 238)
        const static Color Plum;            //!< CSS spec: rgb(221, 160, 221)
        const static Color Thistle;         //!< CSS spec: rgb(216, 191, 216)
        const static Color Lavender;        //!< CSS spec: rgb(230, 230, 250)

        //=========================================================================================
        // White colors
        const static Color MistyRose;     //!< CSS spec: rgb(255, 228, 225)
        const static Color AntiqueWhite;  //!< CSS spec: rgb(250, 235, 215)
        const static Color Linen;         //!< CSS spec: rgb(250, 240, 230)
        const static Color Beige;         //!< CSS spec: rgb(245, 245, 220)
        const static Color WhiteSmoke;    //!< CSS spec: rgb(245, 245, 245)
        const static Color LavenderBlush; //!< CSS spec: rgb(255, 240, 245)
        const static Color OldLace;       //!< CSS spec: rgb(253, 245, 230)
        const static Color AliceBlue;     //!< CSS spec: rgb(240, 248, 255)
        const static Color Seashell;      //!< CSS spec: rgb(255, 245, 238)
        const static Color GhostWhite;    //!< CSS spec: rgb(248, 248, 255)
        const static Color Honeydew;      //!< CSS spec: rgb(240, 255, 240)
        const static Color FloralWhite;   //!< CSS spec: rgb(255, 250, 240)
        const static Color Azure;         //!< CSS spec: rgb(240, 255, 255)
        const static Color MintCream;     //!< CSS spec: rgb(245, 255, 250)
        const static Color Snow;          //!< CSS spec: rgb(255, 250, 250)
        const static Color Ivory;         //!< CSS spec: rgb(255, 255, 240)
        const static Color White;         //!< CSS spec: rgb(255, 255, 255)

        //=========================================================================================
        // Gray and black colors
        const static Color Black;          //!< CSS spec: rgb(0,   0,   0)
        const static Color DarkSlateGray;  //!< CSS spec: rgb(47,  79,  79)
        const static Color DimGray;        //!< CSS spec: rgb(105, 105, 105)
        const static Color SlateGray;      //!< CSS spec: rgb(112, 128, 144)
        const static Color Gray;           //!< CSS spec: rgb(128, 128, 128)
        const static Color LightSlateGray; //!< CSS spec: rgb(119, 136, 153)
        const static Color DarkGray;       //!< CSS spec: rgb(169, 169, 169)
        const static Color Silver;         //!< CSS spec: rgb(192, 192, 192)
        const static Color LightGray;      //!< CSS spec: rgb(211, 211, 211)
        const static Color Gainsboro;      //!< CSS spec: rgb(220, 220, 220)
    };
} // namespace FE
