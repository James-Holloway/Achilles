#include "ShadowCamera.h"
#include "LightObject.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

ShadowCamera::ShadowCamera() : ShadowCamera(L"ShadowCamera", ShadowCameraWidth, ShadowCameraHeight)
{

}

ShadowCamera::ShadowCamera(std::wstring _name, uint32_t width, uint32_t height) : Camera(_name, width, height)
{
    cameraWidth = width;
    cameraHeight = height;
    shadowMap = ShadowMap::CreateShadowMap(cameraWidth, cameraHeight);
    shadowMapRT = std::make_shared<RenderTarget>();
    shadowMapRT->AttachTexture(AttachmentPoint::DepthStencil, shadowMap);
    SetOrthographic(true);
}

ShadowCamera::~ShadowCamera()
{

}

void ShadowCamera::UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingSphere shadowBounds, uint32_t bufferPrecision)
{
    UpdateMatrix(lightPos, lightRotation, shadowBounds, (uint32_t)viewport.Width, (uint32_t)viewport.Height, bufferPrecision);
}

void ShadowCamera::UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingSphere shadowBounds, uint32_t bufferWidth, uint32_t bufferHeight, uint32_t bufferPrecision)
{
    // Look at the shadow center (often 0,0,0)
    // Matrix view = (Matrix)XMMatrixLookAtLH(lightPos, (Vector3)shadowBounds.Center, Vector3::Up);
    // SetView(view);
    SetRotation(lightRotation.ToEuler());
    SetPosition(lightPos);

    if (GetLightType() == LightType::Directional)
    {
        Vector3 sphereCenterLS = XMVector3TransformCoord((Vector3)shadowBounds.Center, GetView());

        float l = sphereCenterLS.x - shadowBounds.Radius;
        float b = sphereCenterLS.y - shadowBounds.Radius;
        float n = sphereCenterLS.z - shadowBounds.Radius;
        float r = sphereCenterLS.x + shadowBounds.Radius;
        float t = sphereCenterLS.y + shadowBounds.Radius;
        float f = sphereCenterLS.z + shadowBounds.Radius;

        nearZ = n;
        farZ = f;

        SetProj((Matrix)XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f));
        SetOrthographic(true);
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
    else if ((GetLightType() & LightType::Point) != LightType::None)
    {

    }

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    Matrix transform{
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    };

    shadowMatrix = ((GetView() * GetProj()) * transform).Transpose();
}

const Matrix& ShadowCamera::GetShadowMatrix() const
{
    return shadowMatrix;
}

std::shared_ptr<ShadowMap> ShadowCamera::GetShadowMap()
{
    return shadowMap;
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
    if (shadowMap)
        return shadowMap->GetRank();
    return -1000.0f; // Return small rank so it becomes the last in the sorted shadow camera list
}

void ShadowCamera::SetRank(float rank)
{
    if (shadowMap)
        shadowMap->SetRank(rank);
}

void ShadowCamera::ResizeShadowMap(uint32_t width, uint32_t height)
{
    if (shadowMap == nullptr)
        return;

    cameraWidth = width;
    cameraHeight = height;
    shadowMap->Resize(cameraWidth, cameraHeight);
    UpdateViewport(width, height);
}
