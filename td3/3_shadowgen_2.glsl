#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Object;

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec2 VertexTexCoord;

out vec2 uv;
out vec3 normal;
out vec3 position;

void main(void)
{	
	uv = VertexTexCoord;
	normal = vec3(Object * vec4(VertexNormal, 1.0));; 
	position = vec3(Object * vec4(VertexPosition, 1.0));
	position.y += (gl_InstanceID * 1.5); 
	position.x += (gl_InstanceID * 1.5); 
	gl_Position = Projection * View * Object * vec4(position, 1.0);
}

#endif

#if defined(FRAGMENT)

out vec4  Color;

in vec3 normal;

void main(void)
{

}

#endif
