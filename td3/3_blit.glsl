#if defined(VERTEX)
uniform mat4 Projection;

in vec3 VertexPosition;
in vec3 VertexTexCoord;

out vec2 uv;

void main(void)
{	
	uv = VertexTexCoord.xy;
	gl_Position = Projection * vec4(VertexPosition, 1.0);
}

#endif

#if defined(FRAGMENT)

in vec2 uv;

uniform sampler2D Texture1;

out vec4  Color;

void main(void)
{
	vec3 color = texture(Texture1, uv).rgb;
	Color = vec4(color, 1.0);
}

#endif
