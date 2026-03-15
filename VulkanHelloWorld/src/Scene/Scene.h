#pragma once
#include "../Core/Devices.h"
#include "../Graphics/Model.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Material.h"
#include<vector>
#include<memory>
#include<string>
#include<unordered_map>

class Scene
{
public:
	Scene(Devices& device);
	~Scene();

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	std::shared_ptr<Model> loadModel(const std::string& path);
	std::shared_ptr<Texture> loadTexture(const std::string& path);
	void addMaterial(const std::shared_ptr<Material> mat);

private:
	Devices& m_device;

	std::vector<std::shared_ptr<Model>> m_models;
	std::vector<std::shared_ptr<Texture>> m_textures;
	std::vector<std::shared_ptr<Material>> m_materials;

	std::unordered_map<std::string, std::shared_ptr<Model>> m_modelCache;
	std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
};
