const char fragment_shader[] =
	"#version 330                                            \n"

	"in vec2 fragTexCoord;                                   \n"
	"in vec4 fragColor;                                      \n"
	"out vec4 finalColor;                                    \n"

	"uniform sampler2D texture0;                             \n"
	"uniform vec4 colDiffuse;                                \n"

	"void main()                                             \n"
	"{                                                       \n"
	"    vec4 texelColor = texture(texture0, fragTexCoord);  \n"
	"    finalColor = texelColor*colDiffuse*fragColor;		 \n"
    "    float tmp = finalColor.b;                           \n"
    "    finalColor.b = finalColor.r;                        \n"
    "    finalColor.r = tmp;                                 \n"
	"}                                                       \n";