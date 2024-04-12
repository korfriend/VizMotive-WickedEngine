#pragma once

wi::ecs::Entity ImportModel_OBJ(const std::string& fileName, wi::scene::Scene& scene);
wi::ecs::Entity ImportModel_GLTF(const std::string& fileName, wi::scene::Scene& scene);
void ExportModel_GLTF(const std::string& filename, wi::scene::Scene& scene);
