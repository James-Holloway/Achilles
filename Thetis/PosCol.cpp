#include "PosCol.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void PosColShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera)
{
    // Update the MVP matrix
    Matrix mvp = object->GetWorldMatrix() * (camera->GetView() * camera->GetProj());

    PosColCB0 cb0{ mvp };

    commandList->SetGraphics32BitConstants<PosColCB0>(0, cb0);
}

std::shared_ptr<Mesh> PosColMeshCreation(aiScene* scene, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material)
{
    uint32_t vertCount = inMesh->mNumVertices;
    std::vector<PosColVertex> verts{};
    verts.resize(vertCount);

    bool hasUVs = inMesh->HasTextureCoords(0);
    for (uint32_t v = 0; v < vertCount; v++)
    {
        aiVector3D vert = inMesh->mVertices[v];
        verts[v].Position = Vector3(vert.x, vert.y, vert.z);

        if (hasUVs)
        {
            aiVector3D uv = inMesh->mTextureCoords[0][v];
            verts[v].Color = Vector4(uv.x, uv.y, 0, 1);
        }
        else
        {
            verts[v].Color = Vector4(1, 1, 1, 1);
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
    std::shared_ptr<Mesh> mesh = SHADER_MESH_MAKE_SHARED_VECTORS(StringToWString(inMesh->mName.C_Str()), verts, tris, PosColVertex, shader);
    return mesh;
}

static std::shared_ptr<Shader> posColShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC polColRootSignature{};
std::shared_ptr<Shader> GetPosColShader(ComPtr<ID3D12Device2> device)
{
    if (posColShader.use_count() >= 1)
    {
        return posColShader;
    }
    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 rootParameters[1]{};
    rootParameters[0].InitAsConstants(sizeof(PosColCB0) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    polColRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    std::shared_ptr rootSignature = std::make_shared<RootSignature>(polColRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    posColShader = Shader::ShaderVSPS(device, posColInputLayout, _countof(posColInputLayout), sizeof(PosColVertex), rootSignature, PosColShaderRender, L"PosCol");
    posColShader->meshCreateCallback = PosColMeshCreation;

    return posColShader;
}