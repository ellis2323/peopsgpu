uniform mat4	uMvp;

attribute vec3 aPosition;
attribute vec4 aColor;

varying vec4 vColor;

void main() {
    vec4 position = vec4(aPosition.xyz, 1.);
    vColor = aColor;
    gl_PointSize = 1.;
    gl_Position = uMvp * position;  
}
