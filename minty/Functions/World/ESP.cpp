﻿#include "ESPRender.h"

namespace cheat
{
	struct ESPFilter {
		std::string name;
		bool enabled;
		game::SimpleFilter simpleFilter;

		explicit operator bool() const {
			return name != "Empty";
		}
	};

	std::vector<ESPFilter> filters;
	void DrawExternal();
	ImColor CalcContrastColor(const ImColor& foreground, float maxContrastRatio = 2.0f, const ImColor& background = ImColor(1.0f, 1.0f, 1.0f), const ImColor& inverted = ImColor(0.0f, 0.0f, 0.0f));

	std::string FirstCharToLowercase(std::string string) {
		std::string output = string;
		output[0] = std::tolower(output[0]);
		return output;
	}

	std::string ConvertToWords(const std::string& input) { // convert strings with format "SomeString" to "Some string"
		std::string result;

		for (size_t i = 0; i < input.length(); ++i) {
			if (i > 0 && isupper(input[i]))
				result += ' ';

			result += tolower(input[i]);
		}
		result[0] = toupper(result[0]);

		return result;
	}

	/*
	The first parameter is a name for the entity and at the same time a key to save the filter status in the config.
	Second - path to the place where the filter value will be saved.
	Third - the filter itself(see game / filters.h).
	*/
	void AddFilter(std::string name, std::string path, game::SimpleFilter filter) {
		std::string key = FirstCharToLowercase(name);
		auto value = config::getValue(path, key, false);
		filters.push_back({ name, value.getValue(), filter });
	}

	ESP::ESP() {
		f_Enabled = config::getValue("functions:ESP", "enabled", false);

		f_TracerSize = config::getValue("functions:ESP", "tracerSize", 1.f);
		f_OutlineThickness = config::getValue("functions:ESP", "outlineThickness", 1.f);

		f_Range = config::getValue("functions:ESP", "range", 200.f);

		f_DrawBox = config::getValue("functions:ESP", "drawBox", true);
		f_DrawTracer = config::getValue("functions:ESP", "drawTracer", true);

		f_DrawName = config::getValue("functions:ESP", "drawName", false);
		f_DrawDistance = config::getValue("functions:ESP", "drawDistance", false);

		f_FontSize = config::getValue("functions::ESP", "textSize", 15);

		f_FillTransparency = config::getValue("functions:ESP", "fillTransparency", 50.f);

		f_GlobalTextColor = config::getValue<std::vector<float>>("functions:ESP:colors", "text", { 1.f, 1.f, 1.f, 1.0f });
		f_GlobalLineColor = config::getValue<std::vector<float>>("functions:ESP:colors", "tracer", { 0.5f, 0.5f, 0.5f, 1.0f });
		f_GlobalBoxColor = config::getValue<std::vector<float>>("functions:ESP:colors", "box", { 0.5f, 0.5f, 0.5f, 1.0f });

		GlobalTextColor.Value = { f_GlobalTextColor.getValue()[0], f_GlobalTextColor.getValue()[1], f_GlobalTextColor.getValue()[2], f_GlobalTextColor.getValue()[3] };
		GlobalLineColor.Value = { f_GlobalLineColor.getValue()[0], f_GlobalLineColor.getValue()[1], f_GlobalLineColor.getValue()[2], f_GlobalLineColor.getValue()[3] };
		GlobalBoxColor.Value = { f_GlobalBoxColor.getValue()[0], f_GlobalBoxColor.getValue()[1], f_GlobalBoxColor.getValue()[2], f_GlobalBoxColor.getValue()[3] };

		entityManager = &game::EntityManager::getInstance();

		/*
		Filters
		Write names in the format "SomeEntityName" to save them in the config as
		"someEntityName" and for the name in the game as "Some entity name"
		*/
		AddFilter("Chest", "functions:ESP:filters", game::filters::combined::Chests);
		AddFilter("Ore", "functions:ESP:filters", game::filters::combined::Ores);
		AddFilter("Plant", "functions:ESP:filters", game::filters::combined::Plants);
		AddFilter("Monsters", "functions:ESP:filters", game::filters::combined::AllMonsters);
		AddFilter("Items", "functions:ESP:filters", game::filters::featured::ItemDrops);
		// AddFilter("Items", "functions:ESP:filters", game::filters::combined::AllPickableLoot); idk why its not workin
		AddFilter("Oculus", "functions:ESP:filters", game::filters::combined::Oculies);
		AddFilter("Seelie", "functions:ESP:filters", game::filters::combined::Seelies);
		AddFilter("SeelieLamp", "functions:ESP:filters", game::filters::puzzle::SeelieLamp);
	}

	std::string ESP::getModule() {
		return _("MODULE_WORLD");
	}

	void ESP::Outer() {
		if (f_Hotkey.IsPressed())
			f_Enabled.setValue(!f_Enabled.getValue());
		DrawExternal();
	}

	void ESP::GUI()
	{
		ConfigCheckbox("ESP", f_Enabled, "Show filtered object through obstacles.");
		if (f_Enabled) {
			ImGui::Indent();
			if (BeginGroupPanel("Config", true))
			{
				ImGuiColorEditFlags_ flag = ImGuiColorEditFlags_::ImGuiColorEditFlags_AlphaBar;

				ImGui::SeparatorText("Overlay settings");
				ConfigCheckbox("Draw box", f_DrawBox);
				ConfigCheckbox("Draw tracer", f_DrawTracer); ImGui::SameLine(); HelpMarker("Draw tracer from center of the screen");
				ConfigCheckbox("Draw name", f_DrawName, "Draw name of object.");
				ConfigCheckbox("Draw distance", f_DrawDistance, "Draw distance to object.");
				ConfigSliderInt("Font size", f_FontSize, 1, 100, "Font size of name or distance.");
				ConfigSliderFloat("Range (m)", f_Range, 1.0f, 200.0f); ImGui::SameLine(); HelpMarker("Max. distance to detect target objects");
				ConfigSliderFloat("Outline width", f_OutlineThickness, 1.0f, 20.0f); ImGui::SameLine(); HelpMarker("Thickness of box outline");
				ConfigSliderFloat("Tracer width", f_TracerSize, 1.0f, 20.0f); ImGui::SameLine(); HelpMarker("Thickness of tracer");

				ImGui::SeparatorText("Colors");
				if (ImGui::ColorEdit4("Color of box", &f_GlobalBoxColor.getValue()[0], flag)) {
					config::setValue(f_GlobalBoxColor, f_GlobalBoxColor.getValue());
					f_GlobalBoxColor.setValue(f_GlobalBoxColor.getValue());
					GlobalBoxColor.Value = { f_GlobalBoxColor.getValue()[0], f_GlobalBoxColor.getValue()[1], f_GlobalBoxColor.getValue()[2], f_GlobalBoxColor.getValue()[3] };
				}
				if (ImGui::ColorEdit4("Color of tracer", &f_GlobalLineColor.getValue()[0], flag)) {
					config::setValue(f_GlobalLineColor, f_GlobalLineColor.getValue());
					f_GlobalLineColor.setValue(f_GlobalLineColor.getValue());
					GlobalLineColor.Value = { f_GlobalLineColor.getValue()[0], f_GlobalLineColor.getValue()[1], f_GlobalLineColor.getValue()[2], f_GlobalLineColor.getValue()[3] };
				}
				if (ImGui::ColorEdit4("Color of text", &f_GlobalTextColor.getValue()[0], flag)) {
					config::setValue(f_GlobalTextColor, f_GlobalTextColor.getValue());
					f_GlobalTextColor.setValue(f_GlobalTextColor.getValue());
					GlobalTextColor.Value = { f_GlobalTextColor.getValue()[0], f_GlobalTextColor.getValue()[1], f_GlobalTextColor.getValue()[2], f_GlobalTextColor.getValue()[3] };
				}
				ConfigSliderFloat("Transparency", f_FillTransparency, 0.01f, 1.0f, "Transparency of box filling.");
			}
			EndGroupPanel();

			if (BeginGroupPanel("Filters", true)) {
				for (ESPFilter& filter : filters) {
					auto name = ConvertToWords(filter.name);
					ImGui::Checkbox(name.c_str(), &filter.enabled);
					std::string key = FirstCharToLowercase(filter.name);
					config::setValue("functions:ESP:filters", key, filter.enabled);
				}
			}
			EndGroupPanel();

			f_Hotkey.Draw();

			ImGui::Unindent();
		}
	}

	void ESP::Status()
	{
		ImGui::Text("ESP [%.01fm]", f_Range.getValue());
	}

	ESP& ESP::getInstance()
	{
		static ESP instance;
		return instance;
	}

	ESPFilter FilterEntity(game::Entity entity) {
		auto& esp = ESP::getInstance();

		for (ESPFilter& filter : filters) {
			if (filter.simpleFilter.IsValid(&entity) and filter.enabled)
				return filter;
		}

		return { "Empty", false, game::filters::Empty };
	}

	void DrawExternal()
	{
		auto& esp = ESP::getInstance();
		if (!esp.f_Enabled.getValue())
			return;

		PrepareFrame();

		for (auto& entity : esp.entityManager->entities())
		{
			if (esp.entityManager->avatar()->distance(entity) > esp.f_Range.getValue())
				continue;

			ESPFilter data = FilterEntity(*entity);

			//LOG_DEBUG("%s", entity->name().c_str());

			if (data)
				DrawEntity(ConvertToWords(data.name), entity, CalcContrastColor(esp.GlobalTextColor));
		}
	}
}