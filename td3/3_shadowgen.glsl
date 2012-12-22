#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Object;
uniform float Time;
uniform vec3 translation;

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec2 VertexTexCoord;

out vec2 uv;
out vec3 normal;
out vec3 position;

void main(void)
{	
	mat3 roatationTime = mat3(cos(Time), 0, sin(Time),
						    0,    1,   0,
						 -sin(Time), 0, cos(Time));
	uv = VertexTexCoord;
	normal = vec3(Object * vec4(VertexNormal, 1.0));
	vec3 pos = VertexPosition*roatationTime + translation;
	position = vec3(Object * vec4(pos, 1.0));
	position.y += 0.1*sin(Time); 
	
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
