#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D tex0;

varying vec2 vTexCoord0;
varying vec4 vColor;


void main()
{
    vec4 color = texture2D(tex0, vTexCoord0, 0.);
    bvec4 is0 = equal(color, vec4(0, 0, 0, 0));
    bool isBlackColor = all(is0);
    if (isBlackColor) {
        discard;
    }
    color.xyz *= vColor.xyz;
//    color.xyz *= vColor.a;
    gl_FragColor = color; 
}

