#version 420 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 squareVertices;

// Output data ; will be interpolated for each fragment.
out VS_OUT 
{
    vec2 UV;
	float dist;
} vs_out;

uniform vec3 vertexPosition_worldspace;
uniform vec3 cameraPos;
uniform mat4 projection;
uniform mat4 view; // Model-View-Projection matrix, but without the Model (the position is in BillboardPos; the orientation depends on the camera)
uniform float viewportAspect;

void main()
{
	float size = 0.04;
	mat4 VP = projection * view;

	gl_Position = VP * vec4(vertexPosition_worldspace, 1.0f); // Get the screen-space position of the particle's center
	gl_Position /= gl_Position.w; // Here we have to do the perspective division ourselves.
	gl_Position.xy += squareVertices.xy * vec2(size, size * viewportAspect); // Move the vertex in directly screen space. No need for CameraUp/Right_worlspace here.
	
	// Or, if BillboardSize is in pixels : 
	// Same thing, just use (ScreenSizeInPixels / BillboardSizeInPixels) instead of BillboardSizeInScreenPercentage.
	vs_out.dist = length(vertexPosition_worldspace - cameraPos);

	// UV of the vertex. No special space for this one.
	vs_out.UV = squareVertices.xy + vec2(0.5, 0.5);
}