#include <GL/glew.h>

class LoadShader
{
public:
	static GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);
};
