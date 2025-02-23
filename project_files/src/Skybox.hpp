#define SKYBOX_HPP

#include <vector>
#include <string>
#include "glew.h"
#include "glm.hpp"

class Skybox {
public:
    // Constructor: takes a list of 6 image file paths for the cubemap
    Skybox(const std::vector<std::string>& faces);
    ~Skybox();
    // Draw the skybox given a view and projection matrix
    void Draw(const glm::mat4& view, const glm::mat4& projection);
private:
    GLuint cubemapTexture;
    GLuint skyboxVAO, skyboxVBO;
    GLuint shaderProgram;
    // Helper functions to load the cubemap and the skybox shader
    bool loadCubemap(const std::vector<std::string>& faces);
    bool loadShaders();
};