#include "CommonShader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace CommonShader;

CommonShader::CommonShaderVertex::CommonShaderVertex() : Position(0,0,0), Normal(0,0,0), Tangent(0,0,0), Bitangent(0,0,0), UV(0,0)
{
    
}

std::shared_ptr<Mesh> CommonShader::CommonShaderMeshCreation(aiScene* scene, aiNode* node, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material, std::wstring meshPath)
{
    uint32_t vertCount = inMesh->mNumVertices;
    std::vector<CommonShaderVertex> verts{};
    verts.resize(vertCount);

    bool hasUVs = inMesh->HasTextureCoords(0);
    bool hasNormals = inMesh->HasNormals();
    bool hasTangents = inMesh->HasTangentsAndBitangents();
    for (uint32_t v = 0; v < vertCount; v++)
    {
        aiVector3D vert = inMesh->mVertices[v];
        verts[v].Position = Vector3(vert.x, vert.y, vert.z);

        if (hasUVs)
        {
            aiVector3D uv = inMesh->mTextureCoords[0][v];
            verts[v].UV = Vector2(uv.x, uv.y);
        }

        if (hasNormals)
        {
            aiVector3D normal = inMesh->mNormals[v];
            verts[v].Normal = Vector3(normal.x, normal.y, normal.z);
        }

        if (hasTangents)
        {
            aiVector3D tangent = inMesh->mTangents[v];
            verts[v].Tangent = Vector3(tangent.x, tangent.y, tangent.z);
            aiVector3D bitangent = inMesh->mBitangents[v];
            verts[v].Bitangent = Vector3(bitangent.x, bitangent.y, bitangent.z);
        }
    }

    uint32_t faceCount = inMesh->mNumFaces;
    std::vector<uint16_t> tris{};
    tris.reserve(faceCount);
    uint32_t index = 0;
    for (uint32_t f = 0; f < faceCount; f++)
    {
        aiFace face = inMesh->mFaces[f];
        if (face.mNumIndices != 3) // Points and lines will get discarded, quads and polys shouldn't exist thanks to the importer flag of triangulate
            continue;

        tris.push_back(face.mIndices[0]);
        tris.push_back(face.mIndices[1]);
        tris.push_back(face.mIndices[2]);
    }

    if (verts.size() <= 0 || tris.size() <= 0)
        return nullptr;
    std::shared_ptr<Mesh> mesh = SHADER_MESH_MAKE_SHARED_VECTORS(StringToWString(inMesh->mName.C_Str()), verts, tris, CommonShaderVertex, shader);

    // Get the DirectX bounding box from points
    DirectX::BoundingBox aabb;
    DirectX::BoundingBox::CreateFromPoints(aabb, verts.size(), (DirectX::XMFLOAT3*)verts.data(), sizeof(CommonShaderVertex));
    aabb.Extents = Vector3::Max(aabb.Extents, Vector3(0.05f)) * 1.05f; // avoid tiny numbers for objects like planes and scale it up a little bit
    mesh->SetBoundingBox(aabb);

    return mesh;
}