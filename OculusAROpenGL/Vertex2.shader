#version 150
 
// Input vertex data, different for all executions of this shader.
in vec3 position;
in vec2 uvcoord;
 
// Output data ; will be interpolated for each fragment.
out vec2 UV;
 
// Values that stay constant for the whole mesh.
uniform mat4 MVPMatrix;
 
void main(){
 
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  MVPMatrix * vec4(position,1);
 
    // UV of the vertex. No special space for this one.
    UV = uvcoord;
}