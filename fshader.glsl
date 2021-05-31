#version 120

uniform sampler2D texture;
uniform vec3 color;
uniform vec3 badColor;
uniform bool differ_hue;
uniform bool plane;
varying vec3 v_normal;

void main()
{
    vec3 N = normalize(v_normal);
    float factor = (N.x + N.z + N.y + 3.0) / 6.0;
    if(!differ_hue) factor = 1.0;

    if (gl_FrontFacing){
        gl_FragColor = vec4(color*factor, 1.0);
     } else {
        gl_FragColor = vec4(badColor*factor, 1.0);
     }

     if(plane)
     	gl_FragColor = vec4(0.5, 0.2, 0.9, 0.4);

}
