#version 330 core

out vec4 frag_color;

// In this shader, we want to draw a checkboard where the size of each tile is (size x size).
// The color of the top-left most tile should be "colors[0]" and the 2 tiles adjacent to it
// should have the color "colors[1]".

//TODO: (Req 1) Finish this shader.

uniform int size = 32;
uniform vec3 colors[2];

void main(){
     // Calculate the position of the fragment in the grid
    vec2 gridPosition = floor(vec2(gl_FragCoord.xy) / size);

    // Determine which color the fragment should have based on its position
    vec3 fragmentColor = colors[int(mod((gridPosition.x + gridPosition.y) , 2))];

    frag_color = vec4(fragmentColor, 1.0);
}