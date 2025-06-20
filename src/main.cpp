#include "Application.h"
#include "Renderer.h"

class ExampleLayer : public Celestiq::Layer
{
public:
	virtual void OnStart() override
	{
		renderer = std::make_unique<Renderer>();
		renderer->Init();
	}

	virtual void OnUIRender() override
	{
		// 计算帧时间
    	currentTime = glfwGetTime();
    	deltaTime = currentTime - lastTime;
    	lastTime = currentTime;

		// 记录每秒帧率
		timeAccumulator += deltaTime;
		frameCounter++;

		if (timeAccumulator >= 1.0f) {
		    float avgFPS = frameCounter / timeAccumulator;
		
		    fpsHistory[fpsIndex] = avgFPS;
		    fpsIndex = (fpsIndex + 1) % FPS_HISTORY_SIZE;
		
		    timeAccumulator = 0.0f;
		    frameCounter = 0;
		}

		ImGui::Begin("Settings");
		if (fpsIndex > 0)
		    ImGui::Text("Last FPS: %.1f", fpsHistory[(fpsIndex - 1 + FPS_HISTORY_SIZE) % FPS_HISTORY_SIZE]);
		else
		    ImGui::Text("Last FPS: N/A");
		ImGui::PlotLines("FPS", fpsHistory, FPS_HISTORY_SIZE, fpsIndex, nullptr, 0.0f, 144.0f, ImVec2(0, 80));
		ImGui::Text("Accumulated Frame Count: %d", renderer->getParams().frameIndex);
		ImGui::Text("Triangle Count: %d", renderer->getTriCount());
		ImGui::End();
		

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");
		viewport_width = ImGui::GetContentRegionAvail().x;
		viewport_height = ImGui::GetContentRegionAvail().y;
		renderer->drawFrame({ viewport_width, viewport_height });
		ImGui::End();
		ImGui::PopStyleVar(1);
	}

private:
	uint32_t viewport_width = 0;	
	uint32_t viewport_height = 0;

	float deltaTime = 0.0f; // 当前帧与上一帧的时间差
    float lastTime = 0.0f; // 上一帧的时间
    float currentTime = 0.0f;

	// 帧数记录
	static constexpr int FPS_HISTORY_SIZE = 60;  // 记录最近60秒
	float fpsHistory[FPS_HISTORY_SIZE] = {};
	int fpsIndex = 0;

	float timeAccumulator = 0.0f;
	int frameCounter = 0;


	std::unique_ptr<Renderer> renderer;
};

Celestiq::Application* Celestiq::CreateApplication(int argc, char** argv)
{
	Celestiq::ApplicationSpecification spec;
	spec.Name = "Celestiq Renderer";

	Celestiq::Application* app = new Celestiq::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}

bool g_ApplicationRunning = true;

int main(int argc, char** argv)
{
	while (g_ApplicationRunning)
	{
		Celestiq::Application* app = Celestiq::CreateApplication(argc, argv);
		app->Run();
		delete app;
	}
	return 0;
}

