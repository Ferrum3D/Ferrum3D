#pragma once
#include <FeCore/Math/Color.h>

namespace FE
{
    //! @brief Named colors from CSS3 specification
    struct Colors
    {
        //=========================================================================================
        // Pink colors
        const static Color4F kMediumVioletRed; //!< CSS spec: rgb(199, 21,  133)
        const static Color4F kDeepPink;        //!< CSS spec: rgb(255, 20,  147)
        const static Color4F kPaleVioletRed;   //!< CSS spec: rgb(219, 112, 147)
        const static Color4F kHotPink;         //!< CSS spec: rgb(255, 105, 180)
        const static Color4F kLightPink;       //!< CSS spec: rgb(255, 182, 193)
        const static Color4F kPink;            //!< CSS spec: rgb(255, 192, 203)

        //=========================================================================================
        // Red colors
        const static Color4F kDarkRed;     //!< CSS spec: rgb(139, 0,   0)
        const static Color4F kRed;         //!< CSS spec: rgb(255, 0,   0)
        const static Color4F kFirebrick;   //!< CSS spec: rgb(178, 34,  34)
        const static Color4F kCrimson;     //!< CSS spec: rgb(220, 20,  60)
        const static Color4F kIndianRed;   //!< CSS spec: rgb(205, 92,  92)
        const static Color4F kLightCoral;  //!< CSS spec: rgb(240, 128, 128)
        const static Color4F kSalmon;      //!< CSS spec: rgb(250, 128, 114)
        const static Color4F kDarkSalmon;  //!< CSS spec: rgb(233, 150, 122)
        const static Color4F kLightSalmon; //!< CSS spec: rgb(255, 160, 122)

        //=========================================================================================
        // Orange colors
        const static Color4F kOrangeRed;  //!< CSS spec: rgb(255, 69,  0)
        const static Color4F kTomato;     //!< CSS spec: rgb(255, 99,  71)
        const static Color4F kDarkOrange; //!< CSS spec: rgb(255, 140, 0)
        const static Color4F kCoral;      //!< CSS spec: rgb(255, 127, 80)
        const static Color4F kOrange;     //!< CSS spec: rgb(255, 165, 0)

        //=========================================================================================
        // Yellow colors
        const static Color4F kDarkKhaki;            //!< CSS spec: rgb(189, 183, 107)
        const static Color4F kGold;                 //!< CSS spec: rgb(255, 215, 0)
        const static Color4F kKhaki;                //!< CSS spec: rgb(240, 230, 140)
        const static Color4F kPeachPuff;            //!< CSS spec: rgb(255, 218, 185)
        const static Color4F kYellow;               //!< CSS spec: rgb(255, 255, 0)
        const static Color4F kPaleGoldenrod;        //!< CSS spec: rgb(238, 232, 170)
        const static Color4F kMoccasin;             //!< CSS spec: rgb(255, 228, 181)
        const static Color4F kPapayaWhip;           //!< CSS spec: rgb(255, 239, 213)
        const static Color4F kLightGoldenrodYellow; //!< CSS spec: rgb(250, 250, 210)
        const static Color4F kLemonChiffon;         //!< CSS spec: rgb(255, 250, 205)
        const static Color4F kLightYellow;          //!< CSS spec: rgb(255, 255, 224)

        //=========================================================================================
        // Brown colors
        const static Color4F kMaroon;         //!< CSS spec: rgb(128, 0,   0)
        const static Color4F kBrown;          //!< CSS spec: rgb(165, 42,  42)
        const static Color4F kSaddleBrown;    //!< CSS spec: rgb(139, 69,  19)
        const static Color4F kSienna;         //!< CSS spec: rgb(160, 82,  45)
        const static Color4F kChocolate;      //!< CSS spec: rgb(210, 105, 30)
        const static Color4F kDarkGoldenrod;  //!< CSS spec: rgb(184, 134, 11)
        const static Color4F kPeru;           //!< CSS spec: rgb(205, 133, 63)
        const static Color4F kRosyBrown;      //!< CSS spec: rgb(188, 143, 143)
        const static Color4F kGoldenrod;      //!< CSS spec: rgb(218, 165, 32)
        const static Color4F kSandyBrown;     //!< CSS spec: rgb(244, 164, 96)
        const static Color4F kTan;            //!< CSS spec: rgb(210, 180, 140)
        const static Color4F kBurlywood;      //!< CSS spec: rgb(222, 184, 135)
        const static Color4F kWheat;          //!< CSS spec: rgb(245, 222, 179)
        const static Color4F kNavajoWhite;    //!< CSS spec: rgb(255, 222, 173)
        const static Color4F kBisque;         //!< CSS spec: rgb(255, 228, 196)
        const static Color4F kBlanchedAlmond; //!< CSS spec: rgb(255, 235, 205)
        const static Color4F kCornsilk;       //!< CSS spec: rgb(255, 248, 220)

        //=========================================================================================
        // Green colors
        const static Color4F kDarkGreen;         //!< CSS spec: rgb(0,   100, 0)
        const static Color4F kGreen;             //!< CSS spec: rgb(0,   128, 0)
        const static Color4F kDarkOliveGreen;    //!< CSS spec: rgb(85,  107, 47)
        const static Color4F kForestGreen;       //!< CSS spec: rgb(34,  139, 34)
        const static Color4F kSeaGreen;          //!< CSS spec: rgb(46,  139, 87)
        const static Color4F kOlive;             //!< CSS spec: rgb(128, 128, 0)
        const static Color4F kOliveDrab;         //!< CSS spec: rgb(107, 142, 35)
        const static Color4F kMediumSeaGreen;    //!< CSS spec: rgb(60,  179, 113)
        const static Color4F kLimeGreen;         //!< CSS spec: rgb(50,  205, 50)
        const static Color4F kLime;              //!< CSS spec: rgb(0,   255, 0)
        const static Color4F kSpringGreen;       //!< CSS spec: rgb(0,   255, 127)
        const static Color4F kMediumSpringGreen; //!< CSS spec: rgb(0,   250, 154)
        const static Color4F kDarkSeaGreen;      //!< CSS spec: rgb(143, 188, 143)
        const static Color4F kMediumAquamarine;  //!< CSS spec: rgb(102, 205, 170)
        const static Color4F kYellowGreen;       //!< CSS spec: rgb(154, 205, 50)
        const static Color4F kLawnGreen;         //!< CSS spec: rgb(124, 252, 0)
        const static Color4F kChartreuse;        //!< CSS spec: rgb(127, 255, 0)
        const static Color4F kLightGreen;        //!< CSS spec: rgb(144, 238, 144)
        const static Color4F kGreenYellow;       //!< CSS spec: rgb(173, 255, 47)
        const static Color4F kPaleGreen;         //!< CSS spec: rgb(152, 251, 152)

        //=========================================================================================
        // Cyan colors
        const static Color4F kTeal;            //!< CSS spec: rgb(0,   128, 128)
        const static Color4F kDarkCyan;        //!< CSS spec: rgb(0,   139, 139)
        const static Color4F kLightSeaGreen;   //!< CSS spec: rgb(32,  178, 170)
        const static Color4F kCadetBlue;       //!< CSS spec: rgb(95,  158, 160)
        const static Color4F kDarkTurquoise;   //!< CSS spec: rgb(0,   206, 209)
        const static Color4F kMediumTurquoise; //!< CSS spec: rgb(72,  209, 204)
        const static Color4F kTurquoise;       //!< CSS spec: rgb(64,  224, 208)
        const static Color4F kAqua;            //!< CSS spec: rgb(0,   255, 255)
        const static Color4F kCyan;            //!< CSS spec: rgb(0,   255, 255)
        const static Color4F kAquamarine;      //!< CSS spec: rgb(127, 255, 212)
        const static Color4F kPaleTurquoise;   //!< CSS spec: rgb(175, 238, 238)
        const static Color4F kLightCyan;       //!< CSS spec: rgb(224, 255, 255)

        //=========================================================================================
        // Blue colors
        const static Color4F kNavy;           //!< CSS spec: rgb(0,   0,   128)
        const static Color4F kDarkBlue;       //!< CSS spec: rgb(0,   0,   139)
        const static Color4F kMediumBlue;     //!< CSS spec: rgb(0,   0,   205)
        const static Color4F kBlue;           //!< CSS spec: rgb(0,   0,   255)
        const static Color4F kMidnightBlue;   //!< CSS spec: rgb(25,  25,  112)
        const static Color4F kRoyalBlue;      //!< CSS spec: rgb(65,  105, 225)
        const static Color4F kSteelBlue;      //!< CSS spec: rgb(70,  130, 180)
        const static Color4F kDodgerBlue;     //!< CSS spec: rgb(30,  144, 255)
        const static Color4F kDeepSkyBlue;    //!< CSS spec: rgb(0,   191, 255)
        const static Color4F kCornflowerBlue; //!< CSS spec: rgb(100, 149, 237)
        const static Color4F kSkyBlue;        //!< CSS spec: rgb(135, 206, 235)
        const static Color4F kLightSkyBlue;   //!< CSS spec: rgb(135, 206, 250)
        const static Color4F kLightSteelBlue; //!< CSS spec: rgb(176, 196, 222)
        const static Color4F kLightBlue;      //!< CSS spec: rgb(173, 216, 230)
        const static Color4F kPowderBlue;     //!< CSS spec: rgb(176, 224, 230)

        //=========================================================================================
        // Purple, violet, and magenta colors
        const static Color4F kIndigo;          //!< CSS spec: rgb(75,  0,   130)
        const static Color4F kPurple;          //!< CSS spec: rgb(128, 0,   128)
        const static Color4F kDarkMagenta;     //!< CSS spec: rgb(139, 0,   139)
        const static Color4F kDarkViolet;      //!< CSS spec: rgb(148, 0,   211)
        const static Color4F kDarkSlateBlue;   //!< CSS spec: rgb(72,  61,  139)
        const static Color4F kBlueViolet;      //!< CSS spec: rgb(138, 43,  226)
        const static Color4F kDarkOrchid;      //!< CSS spec: rgb(153, 50,  204)
        const static Color4F kFuchsia;         //!< CSS spec: rgb(255, 0,   255)
        const static Color4F kMagenta;         //!< CSS spec: rgb(255, 0,   255)
        const static Color4F kSlateBlue;       //!< CSS spec: rgb(106, 90,  205)
        const static Color4F kMediumSlateBlue; //!< CSS spec: rgb(123, 104, 238)
        const static Color4F kMediumOrchid;    //!< CSS spec: rgb(186, 85,  211)
        const static Color4F kMediumPurple;    //!< CSS spec: rgb(147, 112, 219)
        const static Color4F kOrchid;          //!< CSS spec: rgb(218, 112, 214)
        const static Color4F kViolet;          //!< CSS spec: rgb(238, 130, 238)
        const static Color4F kPlum;            //!< CSS spec: rgb(221, 160, 221)
        const static Color4F kThistle;         //!< CSS spec: rgb(216, 191, 216)
        const static Color4F kLavender;        //!< CSS spec: rgb(230, 230, 250)

        //=========================================================================================
        // White colors
        const static Color4F kMistyRose;     //!< CSS spec: rgb(255, 228, 225)
        const static Color4F kAntiqueWhite;  //!< CSS spec: rgb(250, 235, 215)
        const static Color4F kLinen;         //!< CSS spec: rgb(250, 240, 230)
        const static Color4F kBeige;         //!< CSS spec: rgb(245, 245, 220)
        const static Color4F kWhiteSmoke;    //!< CSS spec: rgb(245, 245, 245)
        const static Color4F kLavenderBlush; //!< CSS spec: rgb(255, 240, 245)
        const static Color4F kOldLace;       //!< CSS spec: rgb(253, 245, 230)
        const static Color4F kAliceBlue;     //!< CSS spec: rgb(240, 248, 255)
        const static Color4F kSeashell;      //!< CSS spec: rgb(255, 245, 238)
        const static Color4F kGhostWhite;    //!< CSS spec: rgb(248, 248, 255)
        const static Color4F kHoneydew;      //!< CSS spec: rgb(240, 255, 240)
        const static Color4F kFloralWhite;   //!< CSS spec: rgb(255, 250, 240)
        const static Color4F kAzure;         //!< CSS spec: rgb(240, 255, 255)
        const static Color4F kMintCream;     //!< CSS spec: rgb(245, 255, 250)
        const static Color4F kSnow;          //!< CSS spec: rgb(255, 250, 250)
        const static Color4F kIvory;         //!< CSS spec: rgb(255, 255, 240)
        const static Color4F kWhite;         //!< CSS spec: rgb(255, 255, 255)

        //=========================================================================================
        // Gray and black colors
        const static Color4F kBlack;          //!< CSS spec: rgb(0,   0,   0)
        const static Color4F kDarkSlateGray;  //!< CSS spec: rgb(47,  79,  79)
        const static Color4F kDimGray;        //!< CSS spec: rgb(105, 105, 105)
        const static Color4F kSlateGray;      //!< CSS spec: rgb(112, 128, 144)
        const static Color4F kGray;           //!< CSS spec: rgb(128, 128, 128)
        const static Color4F kLightSlateGray; //!< CSS spec: rgb(119, 136, 153)
        const static Color4F kDarkGray;       //!< CSS spec: rgb(169, 169, 169)
        const static Color4F kSilver;         //!< CSS spec: rgb(192, 192, 192)
        const static Color4F kLightGray;      //!< CSS spec: rgb(211, 211, 211)
        const static Color4F kGainsboro;      //!< CSS spec: rgb(220, 220, 220)
    };
} // namespace FE
