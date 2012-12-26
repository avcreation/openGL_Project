#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Object;
uniform float Time;
uniform vec3 translation;
uniform float rotation;

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
	mat3 rotationMat = mat3(cos(rotation), 0, sin(rotation),
							    0,    1,   0,
							 -sin(rotation), 0, cos(rotation));
	uv = VertexTexCoord;
	normal = vec3(Object * vec4(VertexNormal*roatationTime, 1.0));
	vec3 pos = VertexPosition*roatationTime * rotationMat + translation;
	position = vec3(Object * vec4(pos, 1.0));
	position.y += 0.1*sin(Time); 
	
	gl_Position = Projection * View * Object * vec4(position, 1.0);
}

#endif

#if defined(FRAGMENT)
uniform vec3 CameraPosition;

in vec2 uv;
in vec3 position;
in vec3 normal;

uniform sampler2D Diffuse;
uniform sampler2D Spec;

out vec4  Color;
out vec4  Normal;

void main(void)
{
	vec3 diffuse = texture(Diffuse, uv).rgb;
	float spec = texture(Spec, uv).r;
	Color = vec4(diffuse, spec);
	Normal = vec4(normal, spec);
}

#endif
