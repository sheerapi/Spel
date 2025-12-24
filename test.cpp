// build (example):
// g++ main.cpp -O2 -std=c++17 -lglfw -ldl -lGL -o quad1000
//
// Requires: GLFW, GLAD (or swap GLAD for GLEW, etc.)

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include <cstdio>
#include <cstdlib>

static void glfw_error_callback(int code, const char* msg)
{
	std::fprintf(stderr, "GLFW error %d: %s\n", code, msg);
}

static GLuint compile_shader(GLenum type, const char* src)
{
	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, &src, nullptr);
	glCompileShader(s);

	GLint ok = 0;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		GLint len = 0;
		glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
		char* log = (char*)std::malloc((size_t)len + 1);
		glGetShaderInfoLog(s, len, nullptr, log);
		std::fprintf(stderr, "Shader compile failed:\n%s\n", log);
		std::free(log);
		glDeleteShader(s);
		return 0;
	}
	return s;
}

static GLuint link_program(GLuint vs, GLuint fs)
{
	GLuint p = glCreateProgram();
	glAttachShader(p, vs);
	glAttachShader(p, fs);
	glLinkProgram(p);

	GLint ok = 0;
	glGetProgramiv(p, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		GLint len = 0;
		glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
		char* log = (char*)std::malloc((size_t)len + 1);
		glGetProgramInfoLog(p, len, nullptr, log);
		std::fprintf(stderr, "Program link failed:\n%s\n", log);
		std::free(log);
		glDeleteProgram(p);
		return 0;
	}
	return p;
}

int main()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* win = glfwCreateWindow(800, 600, "Indexed Quad x1000", nullptr, nullptr);
	if (!win)
	{
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(win);

	// vsync on (optional)
	glfwSwapInterval(1);

	glewInit();

	// Simple shaders
	const char* vs_src = R"GLSL(
        #version 330 core
        layout(location=0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )GLSL";

	const char* fs_src = R"GLSL(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )GLSL";

	GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
	GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
	if (!vs || !fs)
		return 1;

	GLuint prog = link_program(vs, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);
	if (!prog)
		return 1;

	// Indexed quad
	// 4 vertices, 6 indices
	float verts[] = {
		// x, y
		-0.5f, -0.5f, // 0
		0.5f,  -0.5f, // 1
		0.5f,  0.5f,  // 2
		-0.5f, 0.5f	  // 3
	};

	unsigned int indices[] = {0, 1, 2, 2, 3, 0};

	GLuint vao = 0, vbo = 0, ebo = 0;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, // location
						  2, // vec2
						  GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	glBindVertexArray(0);

	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();

		int w = 0, h = 0;
		glfwGetFramebufferSize(win, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(prog);
		glBindVertexArray(vao);

		// draw the same indexed quad 1000 times
		for (int i = 0; i < 1000; ++i)
		{
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
		}

		glBindVertexArray(0);

		glfwSwapBuffers(win);
	}

	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(prog);

	glfwDestroyWindow(win);
	glfwTerminate();
	return 0;
}
