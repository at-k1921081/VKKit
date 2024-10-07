#include <unordered_map>
#include <iostream>
#include "Model.h"
#include "SString.h"
#define TINYOBJLOADER_IMPLEMENTATION
// #include "tiny_obj_loader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

// namespace std {
//     template<> struct hash<Vertex> {
//         size_t operator()(const Vertex& vertex) const {
//             return ((hash<glm::vec3>()(vertex.pos) ^
//                     (hash<glm::vec2>()(vertex.color) << 1)) >> 1) ^
//                     (hash<glm::vec2>()(vertex.tex_coord) << 1);
//         }
//     };
// }
// static constexpr size_t CalculateTotalIndices(ut::rspan<tinyobj::shape_t> shapes)
// {
//     size_t size = 0;

//     for (const auto& sh : shapes)
//         size += sh.mesh.indices.size();

//     return size;
// }

// static constexpr bool FindIndex(const float* __restrict past_vertices, size_t npast_vertices, const std::array<float, 3> vertex)
// {
//     for (size_t i = 0; i < npast_vertices; i += 3) {
//         const std::array<float, 3> current_vertex = { past_vertices[i], past_vertices[i + 1], past_vertices[i + 2] };

//         if (current_vertex == vertex) return true;
//     }

//     return false;
// }

// static constexpr size_t CalculateTotalVertices(ut::rspan<const float> vertices)
// {
//     size_t indices = 0;

//     for (size_t i = 0; i < vertices.size(); i += 3) {
//         const std::array<float, 3> vertex = { vertices[i], vertices[i + 1], vertices[i + 2] };

//         if (FindIndex(vertices.data(), i, vertex)) continue;
//         indices += 1;
//     }

//     return indices;
// }

namespace VKKit {
Model::Model(std::string_view filepath)
{
    (void)filepath;
    // Assimp::Importer importer;
    // const aiScene* scene = importer.ReadFile(filepath.data(), aiProcess_Triangulate | aiProcess_FlipUVs);
}

void Model::LoadModel(std::string_view path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.data(), aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        ut::SString<256> error = "Failed to load model: ";
        error.append(path).append("\nError: ").append(importer.GetErrorString());
        throw std::runtime_error(error.data());
    }

    directory = path.substr(0, path.find_last_of('/'));

    ProcessNode(scene->mRootNode, scene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(mesh, scene));
    }

    for (size_t i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene);
    }
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    (void)scene;

    Mesh result_mesh;

    for (size_t i = 0; i < mesh->mNumVertices; ++i) {
        const ModelVertex vertex = {
            .pos = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            ),
            .normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            ),
            .tex_coords = mesh->mTextureCoords[0] ? glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            ) : glm::vec2(0.0f, 0.0f)
        };

        result_mesh.vertices.push_back(vertex);
    }

    for (size_t i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];

        for (size_t j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return result_mesh;
}
}

// Model::Model(std::string_view filepath)
// {
    // // TO DO: Optimise this function (remove vector duplication)

    // tinyobj::attrib_t attrib;
    // std::vector<tinyobj::shape_t> shapes;
    // std::vector<tinyobj::material_t> materials;
    // std::string warn, err;

    // if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.data())) {
    //     throw std::runtime_error(warn + err);
    // }

    // assert(attrib.vertices.size() % 3 == 0);
    // const auto nvertices = CalculateTotalVertices(attrib.vertices);
    // (void)nvertices;
    
    // vertices.reserve(CalculateTotalVertices(attrib.vertices));
    // std::unordered_map<Vertex, uint32_t> unique_vertices;
    // indices.reserve(CalculateTotalIndices(shapes));

    // std::vector<Vertex> vertex_vec;
    // vertex_vec.reserve(CalculateTotalVertices(attrib.vertices));

    // for (const auto& shape : shapes) {
    //     for (const auto& index : shape.mesh.indices) {
    //         Vertex vertex{};
    //         vertex.pos = {
    //             // attrib.vertices[3 * index.vertex_index + 0],
    //             // attrib.vertices[3 * index.vertex_index + 1],
    //             // attrib.vertices[3 * index.vertex_index + 2]
    //             attrib.vertices.at(3 * index.vertex_index + 0),
    //             attrib.vertices.at(3 * index.vertex_index + 1),
    //             attrib.vertices.at(3 * index.vertex_index + 2)
    //         },
    //         vertex.tex_coord = {
    //             // attrib.texcoords[2 * index.texcoord_index + 0],
    //             // 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
    //             attrib.texcoords.at(2 * index.texcoord_index + 0),
    //             1.0f - attrib.texcoords.at(2 * index.texcoord_index + 1)
    //         },
    //         vertex.color = {
    //             1.0f, 1.0f, 1.0f
    //         };

    //         if (unique_vertices.count(vertex) == 0) {
    //             unique_vertices[vertex] = static_cast<uint32_t>(vertex_vec.size());
    //             vertex_vec.push_back(vertex);
    //         }

    //         indices.push_back(unique_vertices[vertex]);
    //     }
    // }

    // vertices.reserve(vertex_vec.size() * 5);

    // for (const auto& v : vertex_vec) {
    //     vertices.push_back(v.pos.x);
    //     vertices.push_back(v.pos.y);
    //     vertices.push_back(v.pos.z);
    //     vertices.push_back(v.tex_coord.x);
    //     vertices.push_back(v.tex_coord.y);
    // }
// }