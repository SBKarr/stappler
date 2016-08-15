
const char* DynamicBatchAI88_frag = STRINGIFY(

\n#ifdef GL_ES\n
precision lowp float;
\n#endif\n

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

void main() {
	gl_FragColor = texture2D(CC_Texture0, v_texCoord) * v_fragmentColor;\n
	/*vec4 tex = texture2D(CC_Texture0, v_texCoord);
    gl_FragColor = vec4( tex.rgb,// RGB from uniform\n
        1.0 // A from texture & uniform\n
    );*/
}

);
