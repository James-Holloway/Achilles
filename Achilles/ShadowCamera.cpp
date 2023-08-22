#include "ShadowCamera.h"
#include "LightObject.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

static const XMVECTORF32 g_vFLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
static const XMVECTORF32 g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
static const XMVECTORF32 g_vHalfVector = { 0.5f, 0.5f, 0.5f, 0.5f };
static const XMVECTORF32 g_vMultiplySetzwToZero = { 1.0f, 1.0f, 0.0f, 0.0f };
static const XMVECTORF32 g_vZero = { 0.0f, 0.0f, 0.0f, 0.0f };

ShadowCamera::ShadowCamera() : ShadowCamera(L"ShadowCamera", ShadowCameraWidth, ShadowCameraWidth)
{

}

ShadowCamera::ShadowCamera(std::wstring _name, uint32_t width, uint32_t height) : Camera(_name, width, height)
{
    cameraWidth = width;
    cameraHeight = height;

    SetOrthographic(true);

    ResizeCascades(numCascades);

    shadowMapRT = std::make_shared<RenderTarget>();
    shadowMapRT->AttachTexture(AttachmentPoint::DepthStencil, shadowMaps[0]);
}

ShadowCamera::~ShadowCamera()
{

}

void ShadowCamera::UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingBox shadowBounds, std::shared_ptr<Camera> camera, uint32_t bufferPrecision)
{
    UpdateMatrix(lightPos, lightRotation, shadowBounds, camera, (uint32_t)viewport.Width, (uint32_t)viewport.Height, bufferPrecision);
}

void ShadowCamera::UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingBox shadowBounds, std::shared_ptr<Camera> camera, uint32_t bufferWidth, uint32_t bufferHeight, uint32_t bufferPrecision)
{
    ScopedTimer _prof(L"UpdateMatrix");

    SetRotation(lightRotation.ToEuler());
    SetPosition(lightPos);

    if (GetLightType() == LightType::Directional)
    {
        Vector3 sphereCenterLS = XMVector3TransformCoord((Vector3)shadowBounds.Center, GetView());

        float l = sphereCenterLS.x - shadowBounds.Extents.x;
        float b = sphereCenterLS.y - shadowBounds.Extents.y;
        float n = sphereCenterLS.z - shadowBounds.Extents.z;
        float r = sphereCenterLS.x + shadowBounds.Extents.x;
        float t = sphereCenterLS.y + shadowBounds.Extents.y;
        float f = sphereCenterLS.z + shadowBounds.Extents.z;

        nearZ = n;
        farZ = f;

        SetProj((Matrix)XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f));
        SetOrthographic(true);

        cascadeProjections = GetDirectionalLightFrustumFromSceneAndCamera(shadowBounds, camera, numCascades);
    }
    else if (GetLightType() == LightType::Spot)
    {
        LightObject* lightObject = GetLightObject();
        if (lightObject != nullptr)
        {
            SpotLight spot = lightObject->GetSpotLight();
            SetFOV((90.0f - toDeg(cosf(spot.OuterSpotAngle))) * 2.0f);
            nearZ = 0.005f;
            farZ = spot.Light.MaxDistance;
        }
        else
        {
            SetFOV(60.0f);
            nearZ = 0.05f;
            farZ = 100.0f;
        }
        SetPerspective(true);
    }
    else if (GetLightType() == LightType::Point)
    {
        SetFOV(90.0f);
        nearZ = 0.005f;
        farZ = 100.0f;

        LightObject* lightObject = GetLightObject();
        if (lightObject != nullptr)
        {
            PointLight point = lightObject->GetPointLight();
            farZ = point.Light.MaxDistance;
        }

        SetPerspective(true);
    }

    shadowMatrix = ((GetView() * GetProj()) * NDCToTextureTransform).Transpose();
}

const Matrix& ShadowCamera::GetShadowMatrix() const
{
    return shadowMatrix;
}

std::shared_ptr<ShadowMap> ShadowCamera::GetShadowMap(uint32_t index)
{
    if (GetLightType() == LightType::Directional && numCascades == 0)
        return shadowMaps[0];

    if (GetLightType() == LightType::Spot)
        return shadowMaps[0];

    if (index >= shadowMaps.size())
        return nullptr;

    return shadowMaps[index];
}

std::shared_ptr<RenderTarget> ShadowCamera::GetShadowMapRenderTarget()
{
    return shadowMapRT;
}

LightType ShadowCamera::GetLightType()
{
    return lightType;
}

void ShadowCamera::SetLightType(LightType _lightType)
{
    lightType = _lightType;
}

LightObject* ShadowCamera::GetLightObject()
{
    return lightObject;
}

void ShadowCamera::SetLightObject(LightObject* _lightObject)
{
    lightObject = _lightObject;
}

float ShadowCamera::GetRank()
{
    return rank;
}

void ShadowCamera::SetRank(float _rank)
{
    rank = _rank;
}

void ShadowCamera::ResizeShadowMaps(uint32_t width, uint32_t height)
{
    cameraWidth = width;
    cameraHeight = height;

    for (uint32_t i = 0; i < shadowMaps.size(); i++)
        shadowMaps[i]->Resize(cameraWidth, cameraHeight);

    if (cubeShadowMap)
        cubeShadowMap->Resize(cameraWidth, cameraHeight, 6);

    UpdateViewport(width, height);
}

static const float preCalculatedPartitions[MAX_NUM_CASCADES][MAX_NUM_CASCADES] =
{
    {
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f // 1 cascade
    },
    {
        0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f // 2 cascades
    },
    {
        0.05f, 0.35f, 1.0f, 1.0f, 1.0f, 1.0f // 3 cascades
    },
    {
        0.05f, 0.15f, 0.45f, 1.0f, 1.0f, 1.0f // 4 cascades
    },
    {
        0.05f, 0.15f, 0.45f, 0.75f, 1.0f, 1.0f // 5 cascades
    },
    {
        0.05f, 0.15f, 0.33f, 0.5f, 0.85f, 1.0f // 6 cascades
    },
};

void ShadowCamera::ResizeCascades(uint32_t newCascades)
{
    numCascades = std::min<uint32_t>(newCascades, MAX_NUM_CASCADES);
    if (numCascades > 0)
    {
        cascadePartitions.resize(numCascades);
        shadowMaps.resize(numCascades);
        for (uint32_t i = 0; i < numCascades; i++)
        {
            // const float curve = 1.75f;
            // cascadePartitions[i] = (i + 1) / (curve + i + 1);
            cascadePartitions[i] = preCalculatedPartitions[numCascades - 1][i];
            shadowMaps[i] = ShadowMap::CreateShadowMap(cameraWidth, cameraHeight);
        }
    }
    else
    {
        cascadePartitions.resize(1);
        shadowMaps.resize(1);
        cascadePartitions[0] = 1.0f;
        shadowMaps[0] = ShadowMap::CreateShadowMap(cameraWidth, cameraHeight);
    }
}

void ShadowCamera::CreatePointShadowMaps()
{
    numCascades = 0;
    shadowMaps.resize(6);
    for (uint32_t i = 0; i < 6; i++)
    {
        shadowMaps[i] = ShadowMap::CreateShadowMap(cameraWidth, cameraHeight);
        shadowMaps[i]->SetName(L"Point Shadow Map " + std::to_wstring(i));
    }
    cascadePartitions.resize(1);
    cascadePartitions[0] = 1.0f;
}

uint32_t ShadowCamera::GetNumCascades()
{
    return numCascades;
}

std::vector<Matrix> ShadowCamera::GetCascadeProjections()
{
    return cascadeProjections;
}

std::vector<Matrix> ShadowCamera::GetCascadeMatrices()
{
    std::vector<Matrix> matrices;
    matrices.resize(cascadeProjections.size());
    for (uint32_t i = 0; i < cascadeProjections.size(); i++)
    {
        matrices[i] = ((GetView() * cascadeProjections[i]) * ShadowCamera::NDCToTextureTransform).Transpose();
    }
    return matrices;
}

std::vector<float> ShadowCamera::GetCascadePartitions()
{
    return cascadePartitions;
}

DirectX::SimpleMath::Matrix ShadowCamera::GetPointDirectionShadowMatrix(uint32_t directionIndex)
{
    Vector3 pos = GetPosition();
    Matrix directionalView = Matrix::Identity;

    switch (directionIndex)
    {
    case 0:
        directionalView = XMMatrixLookAtLH(pos, pos + Vector3::Right, Vector3::Up); // +x
        break;
    case 1:
        directionalView = XMMatrixLookAtLH(pos, pos + Vector3::Left, Vector3::Up); // -x
        break;
    case 2:
        directionalView = XMMatrixLookAtLH(pos, pos + Vector3::Up, Vector3::Forward); // +y
        break;
    case 3:
        directionalView = XMMatrixLookAtLH(pos, pos + Vector3::Down, Vector3::Backward); // -y
        break;
    case 4:
        directionalView = XMMatrixLookAtLH(pos, pos + Vector3::Backward, Vector3::Up); // +z
        break;
    case 5:
        directionalView = XMMatrixLookAtLH(pos, pos + Vector3::Forward, Vector3::Up); // -z
        break;
    }

    return directionalView;
}

std::shared_ptr<ShadowMap> ShadowCamera::GetPointCubeShadowMap(std::shared_ptr<CommandList> commandList)
{
    if (cubeShadowMap == nullptr)
    {
        cubeShadowMap = ShadowMap::CreateShadowMap(cameraWidth, cameraHeight, 6Ui16);
        cubeShadowMap->SetName(L"Point Cube ShadowMap");
    }

    if (commandList != nullptr)
    {
        commandList->CopyTexturesToCubemap(*cubeShadowMap, *GetShadowMap(0), *GetShadowMap(1), *GetShadowMap(2), *GetShadowMap(3), *GetShadowMap(4), *GetShadowMap(5));
    }

    return cubeShadowMap;
}

std::vector<Matrix> ShadowCamera::GetDirectionalLightFrustumFromSceneAndCamera(BoundingBox sceneAABB, std::shared_ptr<Camera> camera, uint32_t numCascades)
{
    if (camera == nullptr)
        throw std::exception("Camera was invalid");

    std::vector<Matrix> shadowProjections;

    if (numCascades <= 0)
    {
        shadowProjections.resize(1);
        shadowProjections[0] = GetProj();
        return shadowProjections;
    }

    shadowProjections.resize(numCascades);

    static const Matrix fix = Matrix::CreateFromYawPitchRoll(0, 0, AchillesPi);

    Matrix lightView = GetView();

    // vSceneAABBPointsLightSpace / pvPointsInCameraView
    Vector3 sceneCorners[BoundingBox::CORNER_COUNT];
    sceneAABB.GetCorners(sceneCorners);

    // Convert scene bounding box to light space
    for (uint32_t i = 0; i < BoundingBox::CORNER_COUNT; i++)
    {
        sceneCorners[i] = XMVector3Transform(sceneCorners[i], lightView);
    }

    float intervalBegin = 0.0f;
    float intervalEnd = 0.0f;
    float cameraNearZ = camera->nearZ;
    float cameraFarZ = camera->farZ;
    float cameraZRange = cameraFarZ - cameraNearZ;
    Matrix cameraView = camera->GetView();
    Matrix cameraInverseView = XMMatrixInverse(nullptr, cameraView);
    // Matrix cameraInverseView = camera->GetInverseView().Transpose();

    XMVECTOR lightCameraOrthographicMin;
    XMVECTOR lightCameraOrthographicMax;

    XMVECTOR worldUnitsPerTexel = Vector4(0.0f);

    // We loop over each of the cascades to calculate the orthographic projection of each cascade
    for (uint32_t cascadeIndex = 0; cascadeIndex < numCascades; cascadeIndex++)
    {
        // Use fit to cascades, no overlap between cascades
        if (cascadeIndex == 0)
            intervalBegin = 0.0f;
        else
            intervalBegin = cascadePartitions[cascadeIndex - 1];

        intervalEnd = std::max<float>(0.01f, cascadePartitions[cascadeIndex]);

        /*
        if (cascadeIndex > 0)
            intervalBegin -= 0.01f; // overlap the cascades a lil bit
        else if (cascadeIndex < numCascades - 1)
            intervalEnd += 0.01f;
        */

        // In Fit To Scene, cascades overlap each other
        bool fitToScene = false;
        if (fitToScene)
            intervalBegin = 0;

        intervalBegin *= cameraZRange;
        intervalEnd *= cameraZRange;

        Vector4 frustumPoints[8];
        camera->CreateFrustumPointsFromCascadeInterval(intervalBegin, intervalEnd, frustumPoints);

        lightCameraOrthographicMin = g_vFLTMAX;
        lightCameraOrthographicMax = g_vFLTMIN;

        XMVECTOR tempTranslatedCornerPoint;
        for (uint32_t i = 0; i < 8; i++)
        {
            // Transform the frustum from camera view space to world space
            frustumPoints[i] = XMVector4Transform(frustumPoints[i], cameraInverseView);
            // Transform the point from world space to light/shadow camera space
            tempTranslatedCornerPoint = XMVector4Transform(frustumPoints[i], lightView);
            // Find the closest point
            lightCameraOrthographicMin = XMVectorMin(tempTranslatedCornerPoint, lightCameraOrthographicMin);
            lightCameraOrthographicMax = XMVectorMax(tempTranslatedCornerPoint, lightCameraOrthographicMax);
        }

        float farPlane = XMVectorGetZ(lightCameraOrthographicMax) * 5;
        float nearPlane = -farPlane;

        // Avoid shadow shimmering
        {
            // Grow the orthographic projection so it doesn't disappear into a corner
            XMVECTOR diagonal = frustumPoints[0] - frustumPoints[6];
            diagonal = XMVector3Length(diagonal);
            diagonal *= 2.5;
            if (cascadeIndex >= numCascades - 1)
                diagonal *= 3.33f;

            // The bound is the length of the diagonal of the frustum interval
            float cascadeBound = XMVectorGetX(diagonal);

            // The offset calculated will pad the ortho projection so that it is always the same size and big enough to cover the entire cascade interval
            XMVECTOR vBorderOffset = (diagonal - (lightCameraOrthographicMax - lightCameraOrthographicMin)) * g_vHalfVector;
            vBorderOffset *= g_vMultiplySetzwToZero;

            // Add the offsets to the projection
            lightCameraOrthographicMax += vBorderOffset;
            lightCameraOrthographicMin -= vBorderOffset;

            // The world units per texel are used to snap the shadow the orthographic projection to texel sized increments.  This keeps the edges of the shadows from shimmering
            float fWorldUnitsPerTexel = (cascadeBound) / (float)cameraWidth;
            worldUnitsPerTexel = XMVectorSet(fWorldUnitsPerTexel, fWorldUnitsPerTexel, 0.0f, 0.0f);
        }

        float lightCameraOrthographicMinZ = XMVectorGetZ(lightCameraOrthographicMin);

        const bool moveLightTexelSize = true;
        if (moveLightTexelSize)
        {
            // We snap the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
            // This is a matter of integer dividing by the world space size of a texel
            lightCameraOrthographicMin /= worldUnitsPerTexel;
            lightCameraOrthographicMin = XMVectorFloor(lightCameraOrthographicMin);
            lightCameraOrthographicMin *= worldUnitsPerTexel;

            lightCameraOrthographicMax /= worldUnitsPerTexel;
            lightCameraOrthographicMax = XMVectorFloor(lightCameraOrthographicMax);
            lightCameraOrthographicMax *= worldUnitsPerTexel;
        }

        // ComputeNearAndFar(fNearPlane, fFarPlane, vLightCameraOrthographicMin, vLightCameraOrthographicMax, vSceneAABBPointsLightSpace) - line 985 from CascadedShadowsMappedManager.cpp

        // float nearPlane = FLT_MAX;
        // float farPlane = -FLT_MAX;

        // nearPlane = 0.0f;
        // farPlane = 100.0f;

#pragma region ComputeNearAndFar

        // 12 triangles per frustum, plus 4 for extra triangles. x3 because 3 points make a triangle
        Vector3 triangleList[16 * 3] = { Vector3(0, 0, 0) };
        bool triangleCulled[16] = { false };
        uint32_t triangleCount = 1;

        triangleList[0 * 3 + 0] = sceneCorners[0];
        triangleList[0 * 3 + 1] = sceneCorners[1];
        triangleList[0 * 3 + 2] = sceneCorners[2];

        static const int aabbTriIndexes[] =
        {
            0,1,2,  1,2,3,
            4,5,6,  5,6,7,
            0,2,4,  2,4,6,
            1,3,5,  3,5,7,
            0,1,4,  1,4,5,
            2,3,6,  3,6,7
        };

        int pointPassesCollision[3];

        // At a high level: 
        // 1. Iterate over all 12 triangles of the AABB.  
        // 2. Clip the triangles against each plane. Create new triangles as needed.
        // 3. Find the min and max z values as the near and far plane.

        //This is easier because the triangles are in camera spacing making the collisions tests simple comparisions.

        float lightCameraOrthographicMinX = XMVectorGetX(lightCameraOrthographicMin);
        float lightCameraOrthographicMaxX = XMVectorGetX(lightCameraOrthographicMax);
        float lightCameraOrthographicMinY = XMVectorGetY(lightCameraOrthographicMin);
        float lightCameraOrthographicMaxY = XMVectorGetY(lightCameraOrthographicMax);

        for (uint32_t AABBTriIter = 0; AABBTriIter < 12; AABBTriIter++)
        {
            triangleList[0 * 3 + 0] = sceneCorners[aabbTriIndexes[AABBTriIter * 3 + 0]];
            triangleList[0 * 3 + 1] = sceneCorners[aabbTriIndexes[AABBTriIter * 3 + 1]];
            triangleList[0 * 3 + 2] = sceneCorners[aabbTriIndexes[AABBTriIter * 3 + 2]];
            triangleCount = 1;
            triangleCulled[0] = false;

            // Clip each individual triangle against the 4 frsutums. Whenever a triangle is clipped into new triangles, add them to the list
            for (uint32_t frustumPlaneIter = 0; frustumPlaneIter < 4; frustumPlaneIter++)
            {
                float edge;
                uint32_t component;

                if (frustumPlaneIter == 0)
                {
                    edge = lightCameraOrthographicMinX;
                    component = 0;
                }
                else if (frustumPlaneIter == 1)
                {
                    edge = lightCameraOrthographicMaxX;
                    component = 0;
                }
                else if (frustumPlaneIter == 2)
                {
                    edge = lightCameraOrthographicMinY;
                    component = 1;
                }
                else // frustumPlaneIter == 3
                {
                    edge = lightCameraOrthographicMaxY;
                    component = 1;
                }

                for (uint32_t triIter = 0; triIter < triangleCount; triIter++)
                {
                    if (triangleCulled[triIter] == true)
                        continue;

                    uint32_t insideVertCount = 0;
                    XMVECTOR tempOrder;
                    // Test against the correct frustum plane. This could be written more compactly but it would be harder to understand

                    if (frustumPlaneIter == 0)
                    {
                        for (uint32_t triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetX(triangleList[triIter * 3 + triPtIter]) > XMVectorGetX(lightCameraOrthographicMin))
                                pointPassesCollision[triPtIter] = 1;
                            else
                                pointPassesCollision[triPtIter] = 0;
                            insideVertCount += pointPassesCollision[triPtIter];
                        }
                    }
                    else if (frustumPlaneIter == 1)
                    {
                        for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetX(triangleList[triIter * 3 + triPtIter]) < XMVectorGetX(lightCameraOrthographicMax))
                                pointPassesCollision[triPtIter] = 1;
                            else
                                pointPassesCollision[triPtIter] = 0;
                            insideVertCount += pointPassesCollision[triPtIter];
                        }
                    }
                    else if (frustumPlaneIter == 2)
                    {
                        for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetY(triangleList[triIter * 3 + triPtIter]) > XMVectorGetY(lightCameraOrthographicMin))
                                pointPassesCollision[triPtIter] = 1;
                            else
                                pointPassesCollision[triPtIter] = 0;
                            insideVertCount += pointPassesCollision[triPtIter];
                        }
                    }
                    else // frustumPlaneIter == 3
                    {
                        for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetY(triangleList[triIter * 3 + triPtIter]) < XMVectorGetY(lightCameraOrthographicMax))
                                pointPassesCollision[triPtIter] = 1;
                            else
                                pointPassesCollision[triPtIter] = 0;
                            insideVertCount += pointPassesCollision[triPtIter];
                        }
                    }

                    // Move the points that pass the frustum test to the begining of the array.
                    if (pointPassesCollision[1] && !pointPassesCollision[0])
                    {
                        tempOrder = triangleList[triIter * 3 + 0];
                        triangleList[triIter * 3 + 0] = triangleList[triIter * 3 + 1];
                        triangleList[triIter * 3 + 1] = tempOrder;
                        pointPassesCollision[0] = TRUE;
                        pointPassesCollision[1] = FALSE;
                    }
                    if (pointPassesCollision[2] && !pointPassesCollision[1])
                    {
                        tempOrder = triangleList[triIter * 3 + 1];
                        triangleList[triIter * 3 + 1] = triangleList[triIter * 3 + 2];
                        triangleList[triIter * 3 + 2] = tempOrder;
                        pointPassesCollision[1] = TRUE;
                        pointPassesCollision[2] = FALSE;
                    }
                    if (pointPassesCollision[1] && !pointPassesCollision[0])
                    {
                        tempOrder = triangleList[triIter * 3 + 0];
                        triangleList[triIter * 3 + 0] = triangleList[triIter * 3 + 1];
                        triangleList[triIter * 3 + 1] = tempOrder;
                        pointPassesCollision[0] = TRUE;
                        pointPassesCollision[1] = FALSE;
                    }

                    // Straight port of line 692 of CascadedShadowsManager.cpp - Clips and tessellates triangles to the plane
                    if (insideVertCount == 0) // All points failed. We're done
                    {
                        triangleCulled[triIter] = true;
                    }
                    else if (insideVertCount == 1) // One point passed. Clip the triangle against the Frustum plane
                    {
                        triangleCulled[triIter] = false;

                        XMVECTOR vVert0ToVert1 = triangleList[triIter * 3 + 1] - triangleList[triIter * 3 + 0];
                        XMVECTOR vVert0ToVert2 = triangleList[triIter * 3 + 2] - triangleList[triIter * 3 + 0];

                        // Find the collision ratio.
                        FLOAT fHitPointTimeRatio = edge - XMVectorGetByIndex(triangleList[triIter * 3 + 0], component);

                        // Calculate the distance along the vector as ratio of the hit ratio to the component
                        FLOAT fDistanceAlongVector01 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert1, component);
                        FLOAT fDistanceAlongVector02 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert2, component);

                        // Add the point plus a percentage of the vector
                        vVert0ToVert1 *= fDistanceAlongVector01;
                        vVert0ToVert1 += triangleList[triIter * 3 + 0];
                        vVert0ToVert2 *= fDistanceAlongVector02;
                        vVert0ToVert2 += triangleList[triIter * 3 + 0];

                        triangleList[triIter * 3 + 1] = vVert0ToVert2;
                        triangleList[triIter * 3 + 2] = vVert0ToVert1;

                    }
                    else if (insideVertCount == 2) // 2 inside, so tesselate into 2 triangles
                    {
                        // Copy the triangle (if it exists) after the current triangle out of the way so we can override it with the new triangle we're inserting.
                        triangleList[triangleCount] = triangleList[triIter + 1];

                        triangleCulled[triIter] = false;
                        triangleCulled[triIter + 1] = false;

                        // Get the vector from the outside point into the 2 inside points.
                        XMVECTOR vVert2ToVert0 = triangleList[triIter * 3 + 0] - triangleList[triIter * 3 + 2];
                        XMVECTOR vVert2ToVert1 = triangleList[triIter * 3 + 1] - triangleList[triIter * 3 + 2];

                        // Get the hit point ratio.
                        FLOAT fHitPointTime_2_0 = edge - XMVectorGetByIndex(triangleList[triIter * 3 + 2], component);
                        FLOAT fDistanceAlongVector_2_0 = fHitPointTime_2_0 / XMVectorGetByIndex(vVert2ToVert0, component);

                        // Calcaulte the new vert by adding the percentage of the vector plus point 2.
                        vVert2ToVert0 *= fDistanceAlongVector_2_0;
                        vVert2ToVert0 += triangleList[triIter * 3 + 2];

                        // Add a new triangle.
                        triangleList[(triIter + 1) * 3 + 0] = triangleList[triIter * 3 + 0];
                        triangleList[(triIter + 1) * 3 + 1] = triangleList[triIter * 3 + 1];
                        triangleList[(triIter + 1) * 3 + 2] = vVert2ToVert0;

                        // Get the hit point ratio.
                        FLOAT fHitPointTime_2_1 = edge - XMVectorGetByIndex(triangleList[triIter * 3 + 2], component);
                        FLOAT fDistanceAlongVector_2_1 = fHitPointTime_2_1 / XMVectorGetByIndex(vVert2ToVert1, component);
                        vVert2ToVert1 *= fDistanceAlongVector_2_1;
                        vVert2ToVert1 += triangleList[triIter * 3 + 2];
                        triangleList[triIter * 3 + 0] = triangleList[(triIter + 1) * 3 + 1];
                        triangleList[triIter * 3 + 1] = triangleList[(triIter + 1) * 3 + 2];
                        triangleList[triIter * 3 + 2] = vVert2ToVert1;

                        // Increment triangle count and skip the triangle we just inserted.
                        ++triangleCount;
                        ++triIter;
                    }
                    else // all points inside in
                    {
                        triangleCulled[triIter] = false;
                    }
                }
            }

            for (uint32_t index = 0; index < triangleCount; ++index)
            {
                if (triangleCulled[index])
                    continue;

                // Set the near and far plan and the min and max z values respectivly
                for (int vertind = 0; vertind < 3; ++vertind)
                {
                    float triangleCoordZ = XMVectorGetZ(triangleList[index * 3 + vertind]);
                    if (nearPlane > triangleCoordZ)
                    {
                        nearPlane = triangleCoordZ;
                    }
                    if (farPlane < triangleCoordZ)
                    {
                        farPlane = triangleCoordZ;
                    }
                }
            }
        }
#pragma endregion

        nearPlane = std::clamp<float>(nearPlane, 0.0f, 9999.9f);
        farPlane = std::min<float>(farPlane, 10000.0f);

        shadowProjections[cascadeIndex] = XMMatrixOrthographicOffCenterLH(XMVectorGetX(lightCameraOrthographicMin), XMVectorGetX(lightCameraOrthographicMax), XMVectorGetY(lightCameraOrthographicMin), XMVectorGetY(lightCameraOrthographicMax), nearPlane, farPlane);
    }

    return shadowProjections;
}
