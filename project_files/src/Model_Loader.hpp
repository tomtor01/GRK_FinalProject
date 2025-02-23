//#include <string>
//#include <vector>
//#include <iostream>
//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//#include "Render_Utils.h"
//#include "Skybox.hpp"
//#include "Texture.h"  // For Core::LoadTexture
//
//namespace models
//{
//    // Global render contexts for our models.
//    Core::RenderContext shipContext;
//    Core::RenderContext sphereContext;
//    Core::RenderContext grassContext;
//}
//
//namespace textures
//{
//    // Global texture IDs.
//    GLuint earth;
//    GLuint clouds;
//    GLuint moon;
//    GLuint ship;
//    GLuint sun;
//    GLuint grass;
//    GLuint grid;
//    // For the skybox, we store a pointer to a Skybox instance.
//    Skybox* skybox;
//}
//
//namespace paths
//{
//    // Texture file paths.
//    std::string earth = "textures/earth.png";
//    std::string clouds = "textures/clouds.jpg";
//    std::string moon = "textures/moon.jpg";
//    std::string ship = "textures/spaceship.jpg";
//    std::string sun = "textures/sun.jpg";
//    std::string grass = "textures/10450_Rectangular_Grass_Patch_v1_Diffuse.jpg";
//    std::string grid = "textures/grid.png";
//
//    // Skybox face textures.
//    std::vector<std::string> skyboxFaces = {
//         "textures/skybox/DaylightBox_Right.bmp", // right
//         "textures/skybox/DaylightBox_Left.bmp",  // left
//         "textures/skybox/DaylightBox_Top.bmp",     // top
//         "textures/skybox/DaylightBox_Bottom.bmp",  // bottom
//         "textures/skybox/DaylightBox_Front.bmp",   // front
//         "textures/skybox/DaylightBox_Back.bmp"     // back
//    };
//}
//
//// Loads a model (OBJ file) into the provided render context.
//inline void loadModelToContext(const std::string& path, Core::RenderContext& context)
//{
//    Assimp::Importer importer;
//    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);
//    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
//    {
//        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
//        return;
//    }
//    context.initFromAssimpMesh(scene->mMeshes[0]);
//}
//
//// Loads the skybox using the file paths specified in paths::skyboxFaces.
//Skybox* loadSkybox()
//{
//    return new Skybox(paths::skyboxFaces);
//}
//
//// Loads all model assets.
//inline void loadAllModels()
//{
//    loadModelToContext("./models/sphere.obj", models::sphereContext);
//    loadModelToContext("./models/spaceship.obj", models::shipContext);
//    loadModelToContext("./models/grass.obj", models::grassContext);
//}
//
//// Loads all textures.
//inline void loadAllTextures()
//{
//    textures::earth = Core::LoadTexture(paths::earth.c_str());
//    textures::clouds = Core::LoadTexture(paths::clouds.c_str());
//    textures::moon = Core::LoadTexture(paths::moon.c_str());
//    textures::ship = Core::LoadTexture(paths::ship.c_str());
//    textures::sun = Core::LoadTexture(paths::sun.c_str());
//    textures::grass = Core::LoadTexture(paths::grass.c_str());
//    textures::grid = Core::LoadTexture(paths::grid.c_str());
//    textures::skybox = Core::LoadTexture(paths::skyboxFaces.c_str());
//}
//
//// A single function call that loads all assets (models, textures, and skybox).
//inline void loadAllAssets()
//{
//    loadAllModels();
//    loadAllTextures();
//    textures::skybox = loadSkybox();
//}