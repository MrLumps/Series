#pragma once


#include "imgui.h"
#include <vector>

namespace ImGui
{
	static auto vector_getter = [](void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).c_str();
		return true;
	};

	bool Combo2(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return Combo(label, currIndex, vector_getter,
			static_cast<void*>(&values), values.size());
	}

	bool ListBox2(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return ListBox(label, currIndex, vector_getter,
			static_cast<void*>(&values), values.size());
	}

	//Someday constexpr 32 bit size_t
	bool DragSize_t(const char* label, size_t* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d") {
		return DragScalar(label, ImGuiDataType_U64, v, v_speed, &v_min, &v_max, format);
	}

	bool Draguint8_t(const char* label, uint8_t* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d") {
		return DragScalar(label, ImGuiDataType_U8, v, v_speed, &v_min, &v_max, format);
	}

	bool Slideruint8_t(const char* label, uint8_t* v, int v_min, int v_max, const char* format = "%d")
	{
		return SliderScalar(label, ImGuiDataType_U8, v, &v_min, &v_max, format);
	}

	bool LimitInputsize_t(const char* label, size_t* v, int v_min, int v_max) {
		const ImU32 step = 1;
		const ImU32 fast = 10;
		if (*v < v_min) {
			*v = v_min;
		}
			
		if (*v > v_max) {
			*v = v_max;
		}
		return InputScalar(label, ImGuiDataType_U32, v, &step, &fast, "%d");
	}
	

	//bool ImGui::DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format)
	//{
	//	return DragScalar(label, ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format);
	//}

}