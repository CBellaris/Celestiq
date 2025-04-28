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
		ImGui::Begin("Settings");
		ImGui::Button("Button");
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

