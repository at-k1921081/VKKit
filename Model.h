#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Buffer.h"
// #include "RenderData.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace VKKit {
struct ModelVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 tex_coords;
};

struct Mesh {
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
};

class Model {
public:
    Model() noexcept = default;
    Model(std::string_view filepath);

    const std::vector<float>& GetVertices() const noexcept { return vertices; }
    const std::vector<uint32_t>& GetIndices() const noexcept { return indices; }

private:
    std::vector<Mesh> meshes;
    std::string directory;

    void LoadModel(std::string_view path);
    void ProcessNode(aiNode* node, const aiScene* scene);
    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
};
}

#endif