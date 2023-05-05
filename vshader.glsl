#version 410

in vec4 vPosition;
in vec4 vColor;
in vec4 vColorID;
out vec4 color;

uniform bool DrawEdges;
uniform bool DrawID;
uniform mat4 Projection;

layout(std140) uniform ModelViews {
    mat4 modelViews[SUB_CUBE_COUNT];
};

const vec4 edge_color = vec4(0.0, 0.0, 0.0, 1.0);

void main()
{
    gl_Position = Projection * modelViews[gl_VertexID / 36] * vPosition;
    color = DrawEdges ? edge_color : (DrawID ? vColorID : vColor);
}
