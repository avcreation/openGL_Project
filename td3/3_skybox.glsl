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
	mat4 roatationX = mat4(1, 0, 0, 0,
						   0,  cos(-3.14/2),   -sin(-3.14/2),    0,
							 0, sin(-3.14/2), cos(-3.14/2), 0,
							    0,    0,   0,    1);
	uv = VertexTexCoord;
	normal = vec3(Object * vec4(VertexNormal, 1.0));; 
	position = vec3(Object * vec4(VertexPosition, 1.0));
	vec4 tmp = Projection * View * roatationX * Object * vec4(position, 1.0);
	gl_Position = tmp.xyww;
}

#endif

#if defined(FRAGMENT)

in vec2 uv;

uniform sampler2D Box;


out vec4  Color;

void main(void)
{
	vec3 diffuse = texture(Box, uv).rgb;
	Color = vec4(diffuse, 0.5);
	//Color = vec4(0,1,1,1);
}

#endif
