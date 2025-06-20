#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <string>

using json = nlohmann::json;

namespace GlobalSettings {

    enum class ComputeShaderBinding : uint32_t {
        // set 0
        VERTEX_BUFFER_SSBO = 0,
        INDEX_BUFFER_SSBO = 1,
        BLAS_NODE_BUFFER_SSBO = 2,
        TLAS_INSTANCE_BUFFER_SSBO = 3,
        TLAS_NODE_BUFFER_SSBO = 4,
        MATERIAL_BUFFER_SSBO = 5,
        CAMERA_UBO = 6,
        FACE_LIGHT_SSBO = 7,
        DIRECTIONAL_LIGHT_SSBO = 8,
        SCENE_INFO = 9
    };

    enum class TempSetting  {
        MAX_TEXTURE_COUNT = 100
    };

    class Config {
    public:
        static inline Config& getInstance() {
            static Config instance;
            return instance;
        }

        inline void SetBool(const std::string& key, bool value) {
            boolSettings[key] = value;
        }

        inline void SetFloat(const std::string& key, float value) {
            floatSettings[key] = value;
        }

        inline void SetInt(const std::string& key, int value) {
            intSettings[key] = value;
        }

        inline bool GetBool(const std::string& key) const {
            auto it = boolSettings.find(key);
            return it != boolSettings.end() ? it->second : false;
        }

        inline float GetFloat(const std::string& key) const {
            auto it = floatSettings.find(key);
            return it != floatSettings.end() ? it->second : 0.0f;
        }

        inline int GetInt(const std::string& key) const {
            auto it = intSettings.find(key);
            return it != intSettings.end() ? it->second : 0;
        }

        inline bool LoadFromFile(const std::string& filename) {
            std::ifstream inFile(filename);
            if (!inFile.is_open()) return false;

            json j;
            inFile >> j;

            if (j.contains("bool")) {
                for (auto& [k, v] : j["bool"].items()) {
                    boolSettings[k] = v.get<bool>();
                }
            }
            if (j.contains("float")) {
                for (auto& [k, v] : j["float"].items()) {
                    floatSettings[k] = v.get<float>();
                }
            }
            if (j.contains("int")) {
                for (auto& [k, v] : j["int"].items()) {
                    intSettings[k] = v.get<int>();
                }
            }

            return true;
        }

        inline bool SaveToFile(const std::string& filename) const {
            json j;
            j["bool"] = boolSettings;
            j["float"] = floatSettings;
            j["int"] = intSettings;

            std::ofstream outFile(filename);
            if (!outFile.is_open()) return false;

            outFile << j.dump(4);
            return true;
        }

    private:
        Config() = default;

        std::unordered_map<std::string, bool> boolSettings;
        std::unordered_map<std::string, float> floatSettings;
        std::unordered_map<std::string, int> intSettings;
    };
}
