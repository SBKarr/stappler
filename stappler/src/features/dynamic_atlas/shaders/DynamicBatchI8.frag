
const char* DynamicBatchI8_frag = STRINGIFY(

\n#ifdef GL_ES\n
precision lowp float;
\n#endif\n

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

void main() {
    gl_FragColor = vec4( v_fragmentColor.rgb,// RGB from uniform\n
        v_fragmentColor.a * (1.0 - texture2D(CC_Texture0, v_texCoord).r) // A from texture & uniform\n
    );
}

);
