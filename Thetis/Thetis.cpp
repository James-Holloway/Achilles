#include "Thetis.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Thetis::Thetis(std::wstring name) : Achilles(name)
{

}

void Thetis::OnUpdate(float deltaTime)
{
	Vector3 rotation = cube->GetLocalRotation();
	rotation.x = fmod(rotation.x + deltaTime * 2 * cubeRotationSpeed, Achilles2Pi);
	rotation.y = fmod(rotation.y + deltaTime * 0.75f * cubeRotationSpeed, Achilles2Pi);
	cube->SetLocalRotation(rotation);

	cube->SetLocalScale(Vector3(1.0f + (0.5f * sinf((float)totalElapsedSeconds * 2.0f))));
}

void Thetis::OnRender(float deltaTime)
{

}

static int radioIndex = 0;
static bool DrawImGuiObjectTree(std::shared_ptr<Object> object)
{
	ImGui::Indent(8);
	if (ImGui::RadioButton(WStringToString(object->GetName() + L"##" + std::to_wstring(radioIndex++)).c_str(), Thetis::selectedPropertiesObject == object))
	{
		if (Thetis::selectedPropertiesObject != object)
			Thetis::selectedPropertiesObject = object;
		else
			Thetis::selectedPropertiesObject = nullptr;
	}
	return true;
}
static void DrawImGuiObjectTreeUp(std::shared_ptr<Object> object)
{
	ImGui::Unindent(8);
}

void Thetis::OnPostRender(float deltaTime)
{
	if (showPerformance)
	{
		if (ImGui::Begin("Performance", &showPerformance))
		{
			ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);
			ImGui::SetWindowSize(ImVec2(300, 235), ImGuiCond_Once);

			ImGui::Text("FPS: %.2f", lastFPS);

			if (ImGui::BeginTabBar("PerformanceTabBar"))
			{
				if (ImGui::BeginTabItem("Frame Time"))
				{
					if (ImPlot::BeginPlot("##PerformanceFrameTime", ImVec2(-1, 150)))
					{
						ImPlot::SetupAxes(NULL, "Frame time (ms)", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_Invert, ImPlotAxisFlags_AutoFit);
						ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 150.0, ImPlotCond_Always); // 150 points of data, locked
						ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 35, ImPlotCond_Always); // 35 ms max plot size, initial only

						std::vector<double> frameTimes{};
						frameTimes.reserve(historicalFrameTimes.size() - 1);
						for (double ft : historicalFrameTimes)
						{
							frameTimes.push_back(ft * 1e3); // seconds to ms
						}

						ImPlot::PlotShaded<double>("", frameTimes.data(), (int)frameTimes.size());
						ImPlot::EndPlot();
					}
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("FPS"))
				{
					if (ImPlot::BeginPlot("##PerformanceFramesPerSecond", ImVec2(-1, 150)))
					{
						ImPlot::SetupAxes(NULL, "Frames per second", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_Invert, ImPlotAxisFlags_AutoFit);
						ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 150.0, ImPlotCond_Always); // 150 points of data, locked
						ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1000, ImPlotCond_Always); // 1000 fps max plot size, initial only
						// ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_SymLog);

						std::vector<double> frames{};
						frames.reserve(historicalFrameTimes.size() - 1);
						for (double ft : historicalFrameTimes)
						{
							frames.push_back(1.0 / ft); // seconds to fps
						}

						ImPlot::PlotShaded<double>("", frames.data(), (int)frames.size());
						ImPlot::EndPlot();
					}
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	if (showObjectTree)
	{
		if (ImGui::Begin("Scenes", &showObjectTree))
		{
			ImGui::SetWindowPos(ImVec2(0, 235), ImGuiCond_Once);
			ImGui::SetWindowSize(ImVec2(300, 350), ImGuiCond_Once);

			if (ImGui::BeginTabBar("SceneTabBar"))
			{
				for (std::shared_ptr<Scene> scene : scenes)
				{
					std::string sceneName = WStringToString(scene->GetName());
					if (ImGui::BeginTabItem(sceneName.c_str()))
					{
						ImGui::Unindent(8);
						radioIndex = 0;
						scene->GetObjectTree()->Traverse(DrawImGuiObjectTree, DrawImGuiObjectTreeUp, 8, 0);
						ImGui::Indent(8);
						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
			}

			ImGui::BeginChild("padding", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() - 6));
			ImGui::EndChild();
			if (ImGui::Button("Add Cube"))
			{
				CreateCubeInMainScene();
			}
			ImGui::SameLine();
			if (ImGui::Button("Add Monkey"))
			{
				CreateMonkeyInMainScene();
			}
		}
		ImGui::End();
	}
	if (showProperties)
	{
		if (ImGui::Begin("Properties", &showProperties))
		{
			ImGui::SetWindowPos(ImVec2(0, 235 + 350));
			ImGui::SetWindowSize(ImVec2(300, 150));

			std::shared_ptr<Object> object = selectedPropertiesObject;
			if (object != nullptr)
			{
				std::string strName = WStringToString(object->GetName());
				char name[64];
				strcpy_s(name, strName.c_str());
				ImGui::SetNextItemWidth(-1);
				ImGui::InputText("##PropertyName", name, 64);
				object->SetName(StringToWString(std::string(name)));

				ImGui::Separator();
				float pos[3] =
				{
					object->GetLocalPosition().x,
					object->GetLocalPosition().y,
					object->GetLocalPosition().z
				};
				if (ImGui::DragFloat3("Position: ", pos, 0.125f, 0.0, 0.0, "% .3f"))
				{
					object->SetLocalPosition(Vector3(pos[0], pos[1], pos[2]));
				}
				
				Vector3 degRot = EulerToDegrees(object->GetLocalRotation());
				float rot[3] =
				{
					degRot.x,
					degRot.y,
					degRot.z
				};
				if (ImGui::DragFloat3("Rotation: ", rot, 5.0f, -360, 360, "% .3f"))
				{
					object->SetLocalRotation(EulerToRadians(Vector3(rot[0], rot[1], rot[2])));
				}
				float scale[3] =
				{
					object->GetLocalScale().x,
					object->GetLocalScale().y,
					object->GetLocalScale().z
				};
				if (ImGui::DragFloat3("Scale: ", scale, 0.125f, 0.0f, 0.0f, "% .3f"))
				{
					object->SetLocalScale(Vector3(scale[0], scale[1], scale[2]));
				}
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
	auto cubeMesh = std::make_shared<Mesh>(L"cube", commandList, (void*)posColCubeVertices, (UINT)_countof(posColCubeVertices), sizeof(PosColVertex), posColCubeIndices, (UINT)_countof(posColCubeIndices), posColShader);
	cube = Object::CreateObject(cubeMesh, L"cube");
	cube->SetLocalPosition(Vector3(-1, 0, 0));
	mainScene->AddObjectToScene(cube);

	miniCube = Object::CreateObject(cubeMesh, L"mini cube");
	cube->AddChild(miniCube);
	miniCube->SetLocalPosition(Vector3(0, 2, 0));
	miniCube->SetLocalScale(Vector3(0.25f, 0.25f, 0.25f));

	// Create floor quad
	auto floorMesh = std::make_shared<Mesh>(L"floor quad", commandList, (void*)PosTextured::posTexturedQuadVertices, (UINT)_countof(PosTextured::posTexturedQuadVertices), sizeof(PosTextured::PosTexturedVertex), PosTextured::posTexturedQuadIndices, (UINT)_countof(PosTextured::posTexturedQuadIndices), posTexturedShader);

	floorQuad = Object::CreateObject(floorMesh, L"floor quad");
	floorQuad->SetLocalPosition(Vector3(0, -2, 0));
	floorQuad->SetLocalRotation(EulerToRadians(Vector3(-90, 0, 0)));
	floorQuad->SetLocalScale(Vector3(3, 3, 3));

	std::shared_ptr<Texture> floorTexture = std::make_shared<Texture>();
	std::wstring floorTexturePath = GetContentDirectoryW() + L"textures/MyUVSquare.png";
	commandList->LoadTextureFromFile(*floorTexture, floorTexturePath, TextureUsage::Albedo);
	floorQuad->GetMaterial().textures.insert({L"MainTexture", floorTexture});

	mainScene->AddObjectToScene(floorQuad);

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
	if (kbt.pressed.F3)
	{
		showObjectTree = !showObjectTree;
	}
	if (kbt.pressed.F4)
	{
		showProperties = !showProperties;
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


void Thetis::CreateCubeInMainScene()
{
	mainScene->AddObjectToScene(Object::CreateObject(cube->GetMesh(), L"New Cube"));
}

void Thetis::CreateMonkeyInMainScene()
{
	std::shared_ptr<Object> object =  Object::CreateObjectsFromContentFile(L"monkey.fbx", GetPosColShader());
	mainScene->AddObjectToScene(object);
}