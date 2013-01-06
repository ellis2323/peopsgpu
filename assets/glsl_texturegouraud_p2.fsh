#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D tex0;

varying vec2 vTexCoord0;
varying vec4 vColor;


void main()
{
    vec4 color = texture2D(tex0, vTexCoord0, 0.);
    if (color.a>0.9||color.a==0.) {
        discard;
    }
    color.xyz *= vColor.xyz;
    gl_FragColor = color; 
}

