#include "Scene.h"
#include "GlobalSettings.h"

static const float M_PI = 3.14159265358979323846f;

BLASBuilder::BLAS BLASBuilder::buildBLAS(const Mesh* mesh)
{
    BLAS result;
    std::vector<Triangle> triangles;

    const auto& vertices = mesh->getVertices();
    const auto& indices  = mesh->getIndices();

    for (size_t i = 0; i < indices.size(); i += 3) {
        Triangle tri{
            vertices[indices[i + 0]].Position,
            vertices[indices[i + 1]].Position,
            vertices[indices[i + 2]].Position,
            static_cast<int>(i)
        };
        triangles.push_back(tri);
    }

    result.triangles = triangles;
    result.nodes.reserve(triangles.size() * 2);  // 预估节点数量
    buildRecursive(result, triangles, 0, static_cast<int>(triangles.size()), result.nodes);

    return result;
}

int BLASBuilder::buildRecursive(BLAS& blas, std::vector<Triangle>& tris, int begin, int end, std::vector<BLASNode>& nodes) {
    // 安全检查：空区间或非法参数
    if (begin >= end) return -1;

    BLASNode node;
    AABB bounds;
    for (int i = begin; i < end; ++i) {
        bounds.expand(tris[i].v0);
        bounds.expand(tris[i].v1);
        bounds.expand(tris[i].v2);
    }
    node.bounds = bounds;

    int count = end - begin;
    if (count <= 3) {
        // 叶子节点
        for(int i = 0; i < count; ++i){
            node.indices[i] = tris[begin + i].index;
        }
        int index = static_cast<int>(nodes.size());
        nodes.push_back(node);
        return index;
    }

    // 简单中位数分割
    glm::vec3 centerSum(0.0f);
    for (int i = begin; i < end; ++i) {
        centerSum += (tris[i].v0 + tris[i].v1 + tris[i].v2) / 3.0f;
    }
    glm::vec3 center = centerSum / static_cast<float>(count);
    int axis = maxAxis(bounds.max - bounds.min);

    auto mid = std::partition(tris.begin() + begin, tris.begin() + end, [&](const Triangle& t) {
        float c = ((t.v0 + t.v1 + t.v2) / 3.0f)[axis];
        return c < center[axis];
    }) - tris.begin();

    // 防止 partition 失败（所有三角形中心都相同）
    if (mid == begin || mid == end) {
        mid = begin + count / 2;
    }

    node.left = buildRecursive(blas, tris, begin, mid, nodes);
    node.right = buildRecursive(blas, tris, mid, end, nodes);
    node.indices = glm::ivec3(-1, -1 , -1);

    int index = static_cast<int>(nodes.size());
    nodes.push_back(node);
    return index;
}

TLASBuilder::TLAS TLASBuilder::buildTLAS(const std::vector<objectInfo>& objects, const std::vector<BLASBuilder::BLAS>& blasList) {
    TLAS result;

    uint32_t rootOffset = 0;
    uint32_t baseIndexOffset = 0;
    for (size_t i = 0; i < objects.size(); ++i) {
        const auto& obj = objects[i];
        const auto& blas = blasList[i];

        AABB bounds;
        for (const auto& tri : blas.triangles) {
            bounds.expand(glm::vec3(obj.transform * glm::vec4(tri.v0, 1.0f)));
            bounds.expand(glm::vec3(obj.transform * glm::vec4(tri.v1, 1.0f)));
            bounds.expand(glm::vec3(obj.transform * glm::vec4(tri.v2, 1.0f)));
        }

        result.instances.push_back({
            obj.transform, bounds, static_cast<int>(rootOffset + blas.nodes.size() - 1), static_cast<int>(baseIndexOffset), obj.materialID // 根节点在最后一个
        });
        rootOffset += static_cast<uint32_t>(blas.nodes.size());
        baseIndexOffset += static_cast<uint32_t>(blas.triangles.size() * 3);
    }

    result.nodes.reserve(objects.size() * 2);
    buildRecursive(result, 0, result.instances.size(), result.nodes);
    return result;
}

int TLASBuilder::buildRecursive(TLAS& tlas, int begin, int end, std::vector<TLASNode>& nodes) {
    if (begin >= end) return -1;
    TLASNode node;
    AABB bounds;
    for (int i = begin; i < end; ++i) {
        bounds.merge(tlas.instances[i].worldBounds);
    }
    node.bounds = bounds;

    int count = end - begin;
    if (count <= 1) {
        int index = static_cast<int>(nodes.size());
        node.instanceIndex = begin;
        nodes.push_back(node);
        return index;
    }

    glm::vec3 centerSum(0.0f);
    for (int i = begin; i < end; ++i)
        centerSum += tlas.instances[i].worldBounds.center();
    glm::vec3 center = centerSum / static_cast<float>(count);
    int axis = maxAxis(bounds.max - bounds.min);

    auto mid = std::partition(tlas.instances.begin() + begin, tlas.instances.begin() + end, [&](const TLASInstance& inst) {
        return inst.worldBounds.center()[axis] < center[axis];
    }) - tlas.instances.begin();

    if (mid == begin || mid == end) {
        mid = begin + count / 2;
    }

    node.left = buildRecursive(tlas, begin, mid, nodes);
    node.right = buildRecursive(tlas, mid, end, nodes);
    node.instanceIndex = -1;

    int index = static_cast<int>(nodes.size());
    nodes.push_back(node);
    return index;
}

// ----------------------------------------------------------------------------------------------------------------------------
void Scene::initScene()
{
    uint32_t texture_0 = TextureManager::get().loadTexture("res/HDRI.hdr", VK_FORMAT_R32G32B32A32_SFLOAT);
    s_sceneInfo.skyboxIndex = texture_0;
    s_sceneInfo.skyboxIntensity = 1.0f;

    // 手动创建材质
    // 红色粗糙材质
    MaterialData mat0{};
    mat0.baseColor = hexToVec3("#df4c68");
    mat0.roughness = 1.0f;
    mat0.metallic = 0.0f;
    uint32_t material_0 = MaterialManager::get().createMaterial(mat0);

    // 磨损金属材质
    MaterialData mat1{};
    uint32_t texture_1 = TextureManager::get().loadTexture("res/image_texture/scratchMetal_diffuse.jpg");
    uint32_t texture_2 = TextureManager::get().loadTexture("res/image_texture/scratchMetal_metallic.jpg", VK_FORMAT_R8_UNORM);  
    uint32_t texture_3 = TextureManager::get().loadTexture("res/image_texture/scratchMetal_roughness.jpg", VK_FORMAT_R8_UNORM);  
    mat1.baseColorTexture = texture_1;
    mat1.metallicTexture = texture_2;
    mat1.roughnessTexture = texture_3;
    uint32_t material_1 = MaterialManager::get().createMaterial(mat1);

    // 灰色粗糙材质
    MaterialData mat2{};  
    mat2.baseColor = glm::vec3(0.4f, 0.4f, 0.4f);
    mat2.roughness = 1.0f;
    mat2.metallic = 0.0f;
    uint32_t material_2 = MaterialManager::get().createMaterial(mat2);

    // 透明玻璃材质
    MaterialData mat3{};
    mat3.baseColor = glm::vec3(1.6f, 1.6f, 1.6f);
    mat3.roughness = 0.3f;
    mat3.metallic = 0.0f;
    mat3.transmission = 1.0f;
    mat3.ior = 1.2f;
    uint32_t material_3 = MaterialManager::get().createMaterial(mat3);

    // 砖块材质
    MaterialData mat4{};
    uint32_t texture_4 = TextureManager::get().loadTexture("res/image_texture/brick_diffuse.jpg");
    uint32_t texture_5 = TextureManager::get().loadTexture("res/image_texture/brick_normal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    mat4.metallic = 0.0f;
    mat4.roughness = 0.8f;
    mat4.baseColorTexture = texture_4;
    mat4.normalTexture = texture_5;
    uint32_t material_4 = MaterialManager::get().createMaterial(mat4);

    // 绿色金属材质
    MaterialData mat5{};
    mat5.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    mat5.metallic = 1.0f;
    mat5.roughness = 0.0f;
    uint32_t material_5 = MaterialManager::get().createMaterial(mat5);

    // 蓝色粗糙材质
    MaterialData mat6{};
    mat6.baseColor = hexToVec3("#39beee");
    mat6.roughness = 1.0f;
    mat6.metallic = 0.0f;
    uint32_t material_6 = MaterialManager::get().createMaterial(mat6);

    // 白色粗糙材质
    MaterialData mat7{};
    mat7.baseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    mat7.roughness = 1.0f;
    mat7.metallic = 0.0f;
    uint32_t material_7 = MaterialManager::get().createMaterial(mat7);

    // 导入模型
    //stbi_set_flip_vertically_on_load(1); 
    // Model model_scene = Model("res/backpack/backpack.obj");
    // model_scene.set_materialID(material_3);
    // s_meshes.insert(s_meshes.end(), model_scene.meshes.begin(), model_scene.meshes.end());

    Model model_tree1= Model("res/Trees.glb");
    model_tree1.set_scale(glm::vec3(0.001f, 0.001f, 0.001f));
    model_tree1.set_rotation(glm::vec3(0.0f, 90.0f, 0.0f));
    model_tree1.set_materialID(material_3);
    s_meshes.insert(s_meshes.end(), model_tree1.meshes.begin(), model_tree1.meshes.end());


    // 手动创建场景物体
    // auto cube = std::make_unique<Mesh>();
    // cube->set_mesh_cube();
    // cube->set_position(glm::vec3(1.1f, 1.0f, 1.0f));
    // cube->set_rotation(glm::vec3(0.0f, -30.0f, 0.0f));
    // cube->set_materialID(material_7);
    // s_meshes.push_back(std::move(cube));

    // auto sphere = std::make_unique<Mesh>();
    // sphere->set_mesh_sphere(32, 16, 1.0f);
    // sphere->set_position(glm::vec3(-1.1f, 1.0f, -1.0f));
    // sphere->set_rotation(glm::vec3(0.0f, 270.0f, 0.0f));
    // sphere->set_materialID(material_5);
    // s_meshes.push_back(std::move(sphere));

    // auto box = std::make_unique<Mesh>();
    // box->set_mesh_box();
    // box->set_scale(glm::vec3(3.0f, 3.0f, 3.0f));
    // box->set_position(glm::vec3(0.0f, 3.0f, 0.0f));
    // box->set_materialID(material_2);
    // s_meshes.push_back(std::move(box));   

    // auto wall = std::make_unique<Mesh>();
    // wall->set_mesh_plane();
    // wall->set_scale(glm::vec3(3.0f, 3.0f, 3.0f));
    // wall->set_position(glm::vec3(3.0f, 3.0f, 0.0f));
    // wall->set_rotation(glm::vec3(0.0f, 0.0f, 90.0f));
    // wall->set_materialID(material_6);
    // s_meshes.push_back(std::move(wall));

    // auto wall2 = std::make_unique<Mesh>();
    // wall2->set_mesh_plane();
    // wall2->set_scale(glm::vec3(3.0f, 3.0f, 3.0f));
    // wall2->set_position(glm::vec3(-3.0f, 3.0f, 0.0f));
    // wall2->set_rotation(glm::vec3(0.0f, 0.0f, 90.0f));
    // wall2->set_materialID(material_0);
    // s_meshes.push_back(std::move(wall2));

    // auto glass_sphere = std::make_unique<Mesh>();
    // glass_sphere->set_mesh_sphere(32, 32, 1.0f);
    // glass_sphere->set_position(glm::vec3(0.0f, 1.0f, 1.5f));
    // glass_sphere->set_materialID(material_3);
    // s_meshes.push_back(std::move(glass_sphere));

    FaceLightData lit{};
    //lit.intensity = 5.0f;
    lit.intensity = 0.0f;
    uint32_t light_0 = Lights::get().createFaceLight(lit);
    auto& light_0_instance = Lights::get().getFaceLight(light_0);
    light_0_instance.setPosition(glm::vec3(0.0f, 5.8f, 0.0f));

    DirectionalLightData dirLight{};
    dirLight.direction = glm::normalize(glm::vec3(-2.0f, -1.0f, 0.1f));
    dirLight.color = glm::vec3(1.0f, 0.8f, 0.8f);
    // dirLight.intensity = 5.0f;
    dirLight.intensity = 0.0f;
    dirLight.angularRadius = 2.0f * M_PI / 180.0f; // 5度
    uint32_t light_1 = Lights::get().createDirectionalLight(dirLight);

    s_camera = std::make_unique<Camera>();
    s_camera->setCameraPosition(glm::vec3(0.0f, 5.0f, 0.0f));
    Celestiq::Input::getInstance().registerKeyCallback(Celestiq::KeyCode::Tab, Celestiq::KeyState::Pressed, [this]{s_camera->resetFirstMouse();});

    initBufferManager();        // 创建缓冲
    uploadSceneToGPU();// 上传数据
}

void Scene::initDescriptor(descriptorPool* pool)
{
    // 创建descriptorSetLayout和descriptorSet
    s_descriptorSetLayout = std::make_unique<descriptorSetLayout>();
    s_descriptorSet = std::make_unique<descriptorSet>();

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[10] = {
        // VertexBuffer
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::VERTEX_BUFFER_SSBO),    //描述符被绑定到0号binding
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,                                           //类型为Storage缓冲区
            .descriptorCount = 1,                                                                          //个数是1个
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT                                                      //在计算着色器阶段读取缓冲区
        },
        // IndexBuffer
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::INDEX_BUFFER_SSBO),     // 1                              
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // BLASNode
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::BLAS_NODE_BUFFER_SSBO),                                 
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // TLASInstance
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::TLAS_INSTANCE_BUFFER_SSBO),                                 
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // TLASNode
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::TLAS_NODE_BUFFER_SSBO),                                
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // Material
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::MATERIAL_BUFFER_SSBO),                                           
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // 摄像机
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::CAMERA_UBO),                                           
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // Face Light
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::FACE_LIGHT_SSBO),                                           
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // Directional Light
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::DIRECTIONAL_LIGHT_SSBO),                                           
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        },
        // Params
        {
            .binding = static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::SCENE_INFO),                                           
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,    
            .descriptorCount = 1,                                   
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT               
        }   
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .bindingCount = 10,
        .pBindings = descriptorSetLayoutBinding
    };

    s_descriptorSetLayout->Create(descriptorSetLayoutCreateInfo);
    pool->AllocateSets(makeSpanFromOne(s_descriptorSet.get()), makeSpanFromOne(s_descriptorSetLayout.get()));
}

void Scene::writeDescriptor()
{
    s_vertexBufferMgr->writeDescriptor(*s_descriptorSet, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::VERTEX_BUFFER_SSBO));
    s_indexBufferMgr->writeDescriptor(*s_descriptorSet, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::INDEX_BUFFER_SSBO));
    s_blasBufferMgr->writeDescriptor(*s_descriptorSet, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::BLAS_NODE_BUFFER_SSBO));
    s_tlasBufferMgr->writeDescriptor(*s_descriptorSet, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::TLAS_INSTANCE_BUFFER_SSBO));
    s_materialBufferMgr->writeDescriptor(*s_descriptorSet, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::MATERIAL_BUFFER_SSBO));

    // Camera 单独处理
    VkDescriptorBufferInfo bufferInfo_camera = {
        .buffer = s_camera->getCameraUBO(),
        .offset = 0,
        .range = s_camera->getCameraUBOSize()
    };
    s_descriptorSet->Write(makeSpanFromOne(bufferInfo_camera), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::CAMERA_UBO));

    s_faceLightBufferMgr->writeDescriptor(*s_descriptorSet, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::FACE_LIGHT_SSBO));
    s_directionalLightBufferMgr->writeDescriptor(*s_descriptorSet, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::DIRECTIONAL_LIGHT_SSBO));

    VkDescriptorBufferInfo bufferInfo_sceneInfo = {
        .buffer = s_sceneInfoBuffer->getHandle(),
        .offset = 0,
        .range = sizeof(SceneInfo)
    }; 
    s_descriptorSet->Write(makeSpanFromOne(bufferInfo_sceneInfo), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(GlobalSettings::ComputeShaderBinding::SCENE_INFO));
}

void Scene::update(float deltaTime)
{
    // 更新摄像机
    s_camera->processKey(Celestiq::Input::IsKeyDown(Celestiq::KeyCode::W),
                         Celestiq::Input::IsKeyDown(Celestiq::KeyCode::A),
                         Celestiq::Input::IsKeyDown(Celestiq::KeyCode::S),
                         Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D),
                         deltaTime);
    if(Celestiq::Input::getInstance().GetCursorMode() == Celestiq::CursorMode::Locked)
        s_camera->processMouseMove(Celestiq::Input::GetMousePosition().x,
                                   Celestiq::Input::GetMousePosition().y);
}

void Scene::initBufferManager()
{
    // 构造并收集 Mesh
    s_vertexBufferMgr = std::make_unique<VertexBufferManager>();
    s_vertexBufferMgr->collect(s_meshes);
    s_vertexBufferMgr->create();

    s_indexBufferMgr = std::make_unique<IndexBufferManager>();
    s_indexBufferMgr->collect(s_meshes);
    s_indexBufferMgr->create();

    s_blasBufferMgr = std::make_unique<BLASBufferManager>();
    s_blasBufferMgr->collect(s_meshes);
    s_blasBufferMgr->create();

    const auto& objectInfos = s_blasBufferMgr->getObjectInfo();
    const auto& blasList = s_blasBufferMgr->getBLASList();

    s_tlasBufferMgr = std::make_unique<TLASBufferManager>();
    s_tlasBufferMgr->build(objectInfos, blasList);
    s_tlasBufferMgr->create();

    s_materialBufferMgr = std::make_unique<MaterialBufferManager>();
    s_materialBufferMgr->create();

    s_faceLightBufferMgr = std::make_unique<FaceLightBufferManager>();
    s_faceLightBufferMgr->create();

    s_directionalLightBufferMgr = std::make_unique<DirectionalLightBufferManager>();
    s_directionalLightBufferMgr->create();

    s_sceneInfoBuffer = std::make_unique<uniformBuffer>(sizeof(SceneInfo));
}

void Scene::uploadSceneToGPU()
{
    s_vertexBufferMgr->upload();
    s_indexBufferMgr->upload();
    s_blasBufferMgr->upload();
    s_tlasBufferMgr->upload();
    s_materialBufferMgr->upload();
    s_faceLightBufferMgr->upload();
    s_directionalLightBufferMgr->upload();

    s_sceneInfoBuffer->TransferData(&s_sceneInfo, sizeof(SceneInfo));
}


glm::vec3 Scene::hexToVec3(const std::string &hexStr)
{
    std::string hex = hexStr;

    // 移除开头的'#'字符
    if (!hex.empty() && hex[0] == '#') {
        hex.erase(0, 1);
    }

    // 处理三位缩写格式（如"F0F"扩展为"FF00FF"）
    if (hex.length() == 3) {
        std::string expanded;
        for (char c : hex) {
            expanded += std::string(2, c); // 每个字符重复两次
        }
        hex = expanded;
    }

    // 验证长度是否为6位
    if (hex.length() != 6) {
        return glm::vec3(0.0f); // 返回黑色作为默认值
    }

    // 分割RGB分量
    unsigned int r, g, b;
    try {
        r = std::stoul(hex.substr(0, 2), nullptr, 16);
        g = std::stoul(hex.substr(2, 2), nullptr, 16);
        b = std::stoul(hex.substr(4, 2), nullptr, 16);
    } catch (...) {
        return glm::vec3(0.0f); // 解析失败返回黑色
    }

    // 转换为0.0-1.0范围的浮点数
    return glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
}