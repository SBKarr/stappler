/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2015 Chukong Technologies Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#ifndef __BASE_CCTYPES_H__
#define __BASE_CCTYPES_H__

#include <string>

#include "math/CCGeometry.h"
#include "math/CCMath.h"
#include "base/CCRef.h"
#include "platform/CCGL.h"

#include "SPLayout.h"
/**
 * @addtogroup base
 * @{
 */

NS_CC_BEGIN

using Color4B = stappler::layout::Color4B;
using Color4F = stappler::layout::Color4F;
using Color3B = stappler::layout::Color3B;

/** A vertex composed of 2 floats: x, y
 @since v3.0
 */
// struct Vertex2F
// {
//     Vertex2F(float _x, float _y) :x(_x), y(_y) {}

//     Vertex2F(): x(0.f), y(0.f) {}

//     GLfloat x;
//     GLfloat y;
// };


/** A vertex composed of 2 floats: x, y
 @since v3.0
 */
// struct Vertex3F
// {
//     Vertex3F(float _x, float _y, float _z)
//         : x(_x)
//         , y(_y)
//         , z(_z)
//     {}

//     Vertex3F(): x(0.f), y(0.f), z(0.f) {}

//     GLfloat x;
//     GLfloat y;
//     GLfloat z;
// };

/** @struct Tex2F
 * A TEXCOORD composed of 2 floats: u, y
 * @since v3.0
 */
struct CC_DLL Tex2F {
    Tex2F(float _u, float _v): u(_u), v(_v) {}

    Tex2F(): u(0.f), v(0.f) {}

    GLfloat u;
    GLfloat v;
};

/** @struct PointSprite
 * Vec2 Sprite component.
 */
struct CC_DLL PointSprite
{
    Vec2   pos;        // 8 bytes
    Color4B    color;      // 4 bytes
    GLfloat    size;       // 4 bytes
};

/** @struct Quad2
 * A 2D Quad. 4 * 2 floats.
 */
struct CC_DLL Quad2
{
    Vec2        tl;
    Vec2        tr;
    Vec2        bl;
    Vec2        br;
};

/** @struct Quad3
 * A 3D Quad. 4 * 3 floats.
 */
struct CC_DLL Quad3 {
    Vec3        bl;
    Vec3        br;
    Vec3        tl;
    Vec3        tr;
};

/** @struct V2F_C4B_T2F
 * A Vec2 with a vertex point, a tex coord point and a color 4B.
 */
struct V2F_C4B_T2F
{
    /// vertices (2F)
    Vec2       vertices;
    /// colors (4B)
    Color4B        colors;
    /// tex coords (2F)
    Tex2F          texCoords;
};

/** @struct V2F_C4B_PF
 *
 */
struct V2F_C4B_PF
{
    /// vertices (2F)
    Vec2       vertices;
    /// colors (4B)
    Color4B        colors;
    /// pointsize
    float      pointSize;
};

/** @struct V2F_C4F_T2F
 * A Vec2 with a vertex point, a tex coord point and a color 4F.
 */
struct CC_DLL V2F_C4F_T2F
{
    /// vertices (2F)
    Vec2       vertices;
    /// colors (4F)
    Color4F        colors;
    /// tex coords (2F)
    Tex2F          texCoords;
};

/** @struct V3F_C4B_T2F
 * A Vec2 with a vertex point, a tex coord point and a color 4B.
 */
struct CC_DLL V3F_C4B_T2F
{
    /// vertices (3F)
    Vec3     vertices;            // 12 bytes

    /// colors (4B)
    Color4B      colors;              // 4 bytes

    // tex coords (2F)
    Tex2F        texCoords;           // 8 bytes
};

/** @struct V3F_T2F
 * A Vec2 with a vertex point, a tex coord point.
 */
struct CC_DLL V3F_T2F
{
    /// vertices (2F)
    Vec3       vertices;
    /// tex coords (2F)
    Tex2F          texCoords;
};

/** @struct V2F_C4B_T2F_Triangle
 * A Triangle of V2F_C4B_T2F.
 */
struct CC_DLL V2F_C4B_T2F_Triangle
{
	V2F_C4B_T2F a;
	V2F_C4B_T2F b;
	V2F_C4B_T2F c;
};

/** @struct V2F_C4B_T2F_Quad
 * A Quad of V2F_C4B_T2F.
 */
struct CC_DLL V2F_C4B_T2F_Quad
{
    /// bottom left
    V2F_C4B_T2F    bl;
    /// bottom right
    V2F_C4B_T2F    br;
    /// top left
    V2F_C4B_T2F    tl;
    /// top right
    V2F_C4B_T2F    tr;
};

/** @struct V3F_C4B_T2F_Quad
 * 4 Vertex3FTex2FColor4B.
 */
struct CC_DLL V3F_C4B_T2F_Quad
{
    /// top left
    V3F_C4B_T2F    tl;
    /// bottom left
    V3F_C4B_T2F    bl;
    /// top right
    V3F_C4B_T2F    tr;
    /// bottom right
    V3F_C4B_T2F    br;
};

/** @struct V2F_C4F_T2F_Quad
 * 4 Vertex2FTex2FColor4F Quad.
 */
struct CC_DLL V2F_C4F_T2F_Quad
{
    /// bottom left
    V2F_C4F_T2F    bl;
    /// bottom right
    V2F_C4F_T2F    br;
    /// top left
    V2F_C4F_T2F    tl;
    /// top right
    V2F_C4F_T2F    tr;
};

/** @struct V3F_T2F_Quad
 *
 */
struct CC_DLL V3F_T2F_Quad
{
    /// bottom left
    V3F_T2F    bl;
    /// bottom right
    V3F_T2F    br;
    /// top left
    V3F_T2F    tl;
    /// top right
    V3F_T2F    tr;
};

/** @struct BlendFunc
 * Blend Function used for textures.
 */
struct CC_DLL BlendFunc
{
    /** source blend function */
    GLenum src;
    /** destination blend function */
    GLenum dst;

    /** Blending disabled. Uses {GL_ONE, GL_ZERO} */
    static const BlendFunc DISABLE;
    /** Blending enabled for textures with Alpha premultiplied. Uses {GL_ONE, GL_ONE_MINUS_SRC_ALPHA} */
    static const BlendFunc ALPHA_PREMULTIPLIED;
    /** Blending enabled for textures with Alpha NON premultiplied. Uses {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA} */
    static const BlendFunc ALPHA_NON_PREMULTIPLIED;
    /** Enables Additive blending. Uses {GL_SRC_ALPHA, GL_ONE} */
    static const BlendFunc ADDITIVE;

    bool operator==(const BlendFunc &a) const
    {
        return src == a.src && dst == a.dst;
    }

    bool operator!=(const BlendFunc &a) const
    {
        return src != a.src || dst != a.dst;
    }

    bool operator<(const BlendFunc &a) const
    {
        return src < a.src || (src == a.src && dst < a.dst);
    }
};

/** @struct TextVAlignment
 * Vertical text alignment type.
 *
 * @note If any of these enums are edited and/or reordered, update Texture2D.m.
 */
enum class CC_DLL TextVAlignment
{
    TOP,
    CENTER,
    BOTTOM
};

/** @struct TextHAlignment
 * Horizontal text alignment type.
 *
 * @note If any of these enums are edited and/or reordered, update Texture2D.m.
 */
enum class CC_DLL TextHAlignment
{
    LEFT,
    CENTER,
    RIGHT
};

// Types for animation in particle systems

/** @struct T2F_Quad
 * Texture coordinates for a quad.
 */
struct CC_DLL T2F_Quad
{
    /// bottom left
    Tex2F    bl;
    /// bottom right
    Tex2F    br;
    /// top left
    Tex2F    tl;
    /// top right
    Tex2F    tr;
};

extern const std::string CC_DLL STD_STRING_EMPTY;
extern const ssize_t CC_DLL CC_INVALID_INDEX;

NS_CC_END
// end group
/// @}
#endif //__BASE_CCTYPES_H__
