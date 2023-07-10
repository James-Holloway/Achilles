#include "Thetis.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Thetis::Thetis(std::wstring name) : Achilles(name)
{

}

void Thetis::OnUpdate(float deltaTime)
{
	Mesh* cubeMesh = cube->GetMesh();
	cubeMesh->rotation.x = fmod(cubeMesh->rotation.x + deltaTime * 2 * cubeRotationSpeed, Achilles2Pi);
	cubeMesh->rotation.y = fmod(cubeMesh->rotation.y + deltaTime * 0.34f * cubeRotationSpeed, Achilles2Pi);

	cubeMesh->scale = Vector3(1.0f + (0.5f * sinf((float)totalElapsedSeconds * 2.0f)));

	cubeMesh->dirtyMatrix = true;
}

void Thetis::OnRender(float deltaTime)
{
	
}

void Thetis::OnPostRender(float deltaTime)
{
	if (showPerformance)
	{
		if (ImGui::Begin("Performance", &showPerformance))
		{
			ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);

			ImGui::Text("FPS: %.2f", lastFPS);

			if (ImPlot::BeginPlot("##Performance", ImVec2(-1, 150)))
			{
				static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;
				ImPlot::SetupAxes(NULL, "Frame time (ms)", flags, ImPlotAxisFlags_AutoFit);
				ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 150.0, ImPlotCond_Always); // 150 points of data, locked
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 35, ImPlotCond_Always); // 35 ms max plot size, initial only

				std::vector<double> frameTimes{};
				frameTimes.reserve(historicalFrameTimes.size() - 1);
				for (double ft : historicalFrameTimes)
				{
					frameTimes.push_back(ft * 1e3); // seconds to ms
				}

				ImPlot::PlotLine<double>("", frameTimes.data(), (int)frameTimes.size());
				ImPlot::EndPlot();
			}
		}

		ImGui::End();
	}
}

void Thetis::OnResize(int newWidth, int newHeight)
{
	camera->UpdateViewport(newWidth, newHeight);
	camera->dirtyProjMatrix = true;
}

void Thetis::LoadContent()
{
	// Get command queue + list
	std::shared_ptr<CommandQueue> commandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

	// Create camera
	camera = std::make_shared<Camera>(L"camera", clientWidth, clientHeight);
	Camera::mainCamera = camera;
	camera->fov = 60;
	camera->position = Vector3(5, 0, 0);
	camera->rotation = EulerToRadians(Vector3(0, -90, 0));

	// Create shaders
	std::shared_ptr<Shader> posColShader = GetPosColShader(device);
	std::shared_ptr<Shader> posTexturedShader = PosTextured::GetPosTexturedShader(device);

	// Create cube
	auto cubeMesh = new Mesh(L"cube", commandList, (void*)posColCubeVertices, (UINT)_countof(posColCubeVertices), sizeof(PosColVertex), posColCubeIndices, (UINT)_countof(posColCubeIndices), posColShader);
	cube = new Object(cubeMesh, L"cube");
	mainScene->AddObjectToScene(cube);

	// Create floor quad
	auto floorMesh = new Mesh(L"floor quad", commandList, (void*)PosTextured::posTexturedQuadVertices, (UINT)_countof(PosTextured::posTexturedQuadVertices), sizeof(PosTextured::PosTexturedVertex), PosTextured::posTexturedQuadIndices, (UINT)_countof(PosTextured::posTexturedQuadIndices), posTexturedShader);
	floorMesh->position = Vector3(0, -2, 0);
	floorMesh->rotation = EulerToRadians(Vector3(-90, 0, 0));
	floorMesh->scale = Vector3(3, 3, 3);

	std::shared_ptr<Texture> floorTexture = std::make_shared<Texture>();
	std::wstring floorTexturePath = GetContentDirectoryW() + L"textures/MyUVSquare.png";
	commandList->LoadTextureFromFile(*floorTexture, floorTexturePath, TextureUsage::Albedo);
	floorMesh->material.textures.insert({ L"MainTexture", floorTexture });

	Object* floorQuadObject = new Object(floorMesh, L"floor quad");
	mainScene->AddObjectToScene(floorQuadObject);

	// Execute command list
	uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}

void Thetis::UnloadContent()
{

}

void Thetis::OnKeyboard(Keyboard::KeyboardStateTracker kbt, Keyboard::State kb, float dt)
{
	if (kbt.pressed.F2)
	{
		showPerformance = !showPerformance;
	}

	if (kbt.pressed.PageUp)
	{
		camera->fov -= 5;
		OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
		camera->dirtyProjMatrix = true;
	}
	if (kbt.pressed.PageDown)
	{
		camera->fov += 5;
		OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
		camera->dirtyProjMatrix = true;
	}

	float cameraRotationAmount = 15;
	if (kbt.pressed.Up)
	{
		camera->RotateEuler(Vector3(-toRad(cameraRotationAmount), 0, 0));
	}
	if (kbt.pressed.Down)
	{
		camera->RotateEuler(Vector3(toRad(cameraRotationAmount), 0, 0));
	}
	if (kbt.pressed.Right)
	{
		camera->RotateEuler(Vector3(0, toRad(cameraRotationAmount), 0));
	}
	if (kbt.pressed.Left)
	{
		camera->RotateEuler(Vector3(0, -toRad(cameraRotationAmount), 0));
	}

	float movementSpeed = cameraBaseMoveSpeed;
	if (kb.LeftControl)
		movementSpeed *= 0.5f;
	if (kb.LeftShift)
		movementSpeed *= 2.0f;

	movementSpeed *= dt;

	if (kb.W)
	{
		camera->MoveRelative(Vector3(0, 0, movementSpeed));
	}
	if (kb.S)
	{
		camera->MoveRelative(Vector3(0, 0, -movementSpeed));
	}
	if (kb.D)
	{
		camera->MoveRelative(Vector3(movementSpeed, 0, 0));
	}
	if (kb.A)
	{
		camera->MoveRelative(Vector3(-movementSpeed, 0, 0));
	}
	if (kb.E)
	{
		camera->MoveRelative(Vector3(0, movementSpeed, 0));
	}
	if (kb.Q)
	{
		camera->MoveRelative(Vector3(0, -movementSpeed, 0));
	}
}

void Thetis::OnMouse(Mouse::ButtonStateTracker mt, MouseData md, Mouse::State state, float dt)
{
	if (mt.rightButton)
	{
		float cameraRotationSpeed = toRad(45) * dt * mouseSensitivity;
		camera->RotateEuler(Vector3(cameraRotationSpeed * md.mouseYDelta, cameraRotationSpeed * md.mouseXDelta, 0));
	}
}
