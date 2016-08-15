
const char* DrawNodeA8_vert = STRINGIFY(

attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec4 a_color;

\n#ifdef GL_ES\n
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
\n#else\n
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
\n#endif\n

void main() {
    gl_Position = CC_PMatrix * a_position;
    v_fragmentColor = a_color;
    v_texCoord = a_texCoord;
}

);
