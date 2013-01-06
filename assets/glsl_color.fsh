#ifdef GL_ES
precision mediump float;
#endif

varying vec4 vColor;

void main() 
{
    bvec4 is0 = equal(vColor, vec4(0, 0, 0, 0));
    bool isBlackColor = all(is0);
    if (isBlackColor) {
        discard;
    }
    gl_FragColor = vColor;
}

