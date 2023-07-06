#include "Thetis.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Thetis::Thetis(std::wstring name) : Achilles(name)
{

}

void Thetis::OnUpdate(float deltaTime)
{
	// cube->rotation.x = fmod(cube->rotation.x + deltaTime * 2 * cubeRotationSpeed, Achilles2Pi);
	// cube->rotation.y = fmod(cube->rotation.y + deltaTime * 0.34f * cubeRotationSpeed, Achilles2Pi);
}

void Thetis::OnRender(float deltaTime)
{
	QueueMeshDraw(cube);
}

void Thetis::OnPostRender(float deltaTime)
{

}

void Thetis::OnResize(int newWidth, int newHeight)
{
	camera->UpdateViewport(newWidth, newHeight);
}

void Thetis::LoadContent()
{
	// Get command queue + list
	std::shared_ptr<CommandQueue> commandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	ComPtr<ID3D12GraphicsCommandList2> commandList = commandQueue->GetCommandList();

	// Create camera
	camera = std::make_shared<Camera>(L"camera", clientWidth, clientHeight);
	Camera::mainCamera = camera;
	camera->fov = 60;
	camera->position = Vector3(5, 0, 0);
	camera->rotation = EulerToRadians(Vector3(0, -90, 0));

	// Create shader
	std::shared_ptr<Shader> posColShader = GetPosColShader(device);

	// Create cube
	cube = std::make_shared<Mesh>(L"cube", device, commandList, (void*)posColCubeVertices, (UINT)_countof(posColCubeVertices), posColCubeIndices, (UINT)_countof(posColCubeIndices), posColShader);

	// Execute command list
	uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	// Free creation resources
	cube->FreeCreationResources();
}

void Thetis::UnloadContent()
{
	printf("Cube use_count: %i\n", cube.use_count());
}

void Thetis::OnKeyboard(Keyboard::KeyboardStateTracker kbt, float dt)
{
	if (kbt.pressed.PageUp)
	{
		camera->fov -= 5;
		OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
	}
	if (kbt.pressed.PageDown)
	{
		camera->fov += 5;
		OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
	}
	float cameraRotationSpeed = 15;
	if (kbt.pressed.Up)
	{
		camera->RotateEuler(Vector3(-toRad(cameraRotationSpeed), 0, 0));
	}
	if (kbt.pressed.Down)
	{
		camera->RotateEuler(Vector3(toRad(cameraRotationSpeed), 0, 0));
	}
	if (kbt.pressed.Right)
	{
		camera->RotateEuler(Vector3(0, toRad(cameraRotationSpeed), 0));
	}
	if (kbt.pressed.Left)
	{
		camera->RotateEuler(Vector3(0, -toRad(cameraRotationSpeed), 0));
	}

	if (kbt.pressed.W)
	{
		camera->MoveRelative(Vector3(0, 0, 1));
	}
	if (kbt.pressed.S)
	{
		camera->MoveRelative(Vector3(0, 0, -1));
	}
	if (kbt.pressed.D)
	{
		camera->MoveRelative(Vector3(1, 0, 0));
	}
	if (kbt.pressed.A)
	{
		camera->MoveRelative(Vector3(-1, 0, 0));
	}
	if (kbt.pressed.E)
	{
		camera->MoveRelative(Vector3(0, 1, 0));
	}
	if (kbt.pressed.Q)
	{
		camera->MoveRelative(Vector3(0, -1, 0));
	}
}

void Thetis::OnMouse(Mouse::ButtonStateTracker mt, float dt)
{

}
