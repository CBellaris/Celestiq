#version 450
#pragma shader_stage(compute)
#extension GL_EXT_nonuniform_qualifier : require

layout(local_size_x = 16, local_size_y = 16) in;

// 场景数据----------------------------------------------------------------
// binding = 0: Vertex
struct Vertex {
    vec3 Position;
    vec3 Normal;
    vec3 Tangent;  
    vec2 TexCoords;
    float tangentW; 
};
layout(std430, set = 0, binding = 0) readonly buffer VertexBuffer {
    Vertex vertices[];
};

// binding = 1: Index
layout(std430, set = 0, binding = 1) readonly buffer IndexBuffer {
    uint indices[];
};

// binding = 2: BLASNode
struct AABB {
    vec3 min;
    vec3 max;
};
struct BLASNode {
    AABB bounds;
    ivec3 indices;
    int left;
    int right;
};
layout(std430, binding = 2) readonly buffer BLASBuffer {
    BLASNode blasNodes[];
};

// binding = 3: TLASInstance
struct TLASInstance {
    mat4 transform;
    AABB worldBounds;
    int rootNodeIndex; // 对应BLAS根节点位置
    int baseIndexOffset;
    int materialID;
};
layout(std430, binding = 3) readonly buffer TLASInstanceBuffer {
    TLASInstance instances[];
};

// binding = 4: TLASNode
struct TLASNode {
    AABB bounds;
    int left;
    int right;
    int instanceIndex;
};
layout(std430, binding = 4) readonly buffer TLASNodeBuffer {
    TLASNode tlasNodes[];
};

// binding = 5: Material
struct Material {
    vec3 baseColor;
    vec3 normal;
    float metallic;     // 金属度
    float roughness;    // 粗糙度
    float ior;          // 折射率，空气≈1.0，玻璃≈1.5
    float transmission; // 透射能量占比（“透明度”）

    int baseColorTexture;  // -1 表示无图像纹理
    int normalTexture;
    int metallicTexture;
    int roughnessTexture;
    int transmissionTexture;
};
layout(std430, set = 0, binding = 5) readonly buffer MaterialBuffer {
    Material materials[];
};

// binding = 6: camera
layout(set = 0, binding = 6) uniform Camera {
    mat4 viewProjectionMatrix; // 视图投影矩阵
    vec3 cameraPosition;       // 摄像机位置
};

// binding = 7: FaceLight
struct FaceLight {
    vec3 v0; // 顺序：逆时针
    vec3 v1;
    vec3 v2;
    vec3 v3;
    vec3 color;
    float intensity;
    float cdf;
};
layout(std430, set = 0, binding = 7) readonly buffer FaceLightBuffer {
    FaceLight faceLights[];
};

// binding = 8: Directional
struct DirectionalLight {
    vec3 direction; // 单位向量
    vec3 color;
    float intensity;
    float angularRadius;
};
layout(std430, set = 0, binding = 8) readonly buffer DirectionalLightBuffer {
    DirectionalLight directionalLights[];
};

// binding = 9: SceneInfo
layout(set = 0, binding = 9) uniform SceneInfo {
    int skyboxIndex;
    float skyboxIntensity; // 天空盒亮度
};

// binding = 0: 纹理数组
layout(set = 1, binding = 0) uniform sampler2D textures[];

// binding = 0: output image (储存最终颜色)
layout(set = 2, binding = 0, rgba32f) uniform writeonly image2D resultImage;
// binding = 1: accumulation image (储存累积颜色)
layout(set = 2, binding = 1, rgba32f) uniform readonly image2D accumImage;

// binding = 2: constants buffer
layout(set = 2, binding = 2) uniform Params {
    int frameIndex;
    int debugMode; // 调试模式
};

// 结构体定义-----------------------------------------------------------
struct HitInfo {
    float tWorld;
    vec3 hitPos;
    vec3 normal;
    vec2 texCoord;
    int materialID;
    bool hit;
    mat3 TBN;
};

struct RNGState { uint state; };   // 32-bit 状态，一条路径一个实例

struct LightSample {
    vec3  pos;           // 采样点世界座标
    vec3  normal;        // 面法线
    vec3  radiance;      // lightColor * intensity
    float pdfArea;       // 对 *面积* 的 pdf
    int isDirectional; // 0: 面光，1: 方向光
};

// 函数定义-----------------------------------------------------------
const float PI = 3.14159265359;
// 求交相关
bool intersectAABB(vec3 origin, vec3 dir, AABB aabb);
bool intersectTriangle(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2, out float t);
bool intersectTriangleBarycentric(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2, out float t, out vec3 baryCoord);
void traceTLAS_stack(int rootIndex, vec3 rayOrig, vec3 rayDir, inout HitInfo hitInfo);
void traceBLAS_stack(int rootIndex, vec3 rayOrig, vec3 rayDir, mat4 model, int materialID, int baseIndexOffset, inout HitInfo hitInfo);
bool traceShadow(vec3 rayOrig, vec3 rayDir, float maxDist, int rootIndex);
bool traceBLAS_shadow(int rootIndex, vec3 rayOrig, vec3 rayDir, mat4 model, float maxDist, int baseIndexOffset);
bool hitFaceLight(vec3 ro, vec3 rd, out float tMin, out vec3 color, out float intensity);
bool hitDirectionalLight(vec3 rayDir, out vec3 color, out float intensity);

// 随机相关
float RadicalInverse_VdC(uint bits);
RNGState rng_init(uvec2 pixel, uint frame, uint bounce);
uint rng_next(inout RNGState rng);
float rng_nextFloat(inout RNGState rng);
vec2 rng_nextFloat2(inout RNGState rng);
float sobolOwen(uint index, uint scramble, uint dim);
vec2 sobol2D(uint index, uint bounce, uint scramble);

// Cook-Torrance specular BRDF
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float NdotV, vec3 F0, float rough);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
float fresnelDielectric(float cosThetaI, float etaI, float etaT);

// 光线采样相关
vec3 uniformSampleHemisphere(vec2 Xi, vec3 N);
vec3 cosineSampleHemisphere(vec2 Xi, vec3 N);
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness);
float sampleLightIndex(inout RNGState rng);
LightSample sampleFaceLight(uint idx, vec2 Xi);
vec3 sampleCone(vec3 axis, float cosThetaMax, vec2 Xi, out float pdfSolidAngle);
LightSample sampleDirectionalLight(vec2 Xi);

// 主循环-----------------------------------------------------------------
void main() {
    uvec2 pixelCoord = gl_GlobalInvocationID.xy; // 当前像素坐标
    ivec2 imageSize = imageSize(resultImage);
    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y)
        return;

    RNGState rng = rng_init( pixelCoord, frameIndex, 0u );  // 初始化随机数生成器


    vec3 finalOutputColor = vec3(0.0); // 最终输出颜色，根据模式不同而定

    // 生成当前像素的初始射线
    vec2 primarySubPixel   = vec2(rng_nextFloat(rng), rng_nextFloat(rng)); // tent filter 更好
    vec2 primaryNdc        = ((vec2(pixelCoord) + primarySubPixel) / vec2(imageSize)) * 2.0 - 1.0;
    vec4 primaryRayClip    = vec4(primaryNdc, -1.0, 1.0);
    vec4 primaryRayView    = inverse(viewProjectionMatrix) * primaryRayClip;
    primaryRayView        /= primaryRayView.w;

    vec3 primaryRayOrig    = cameraPosition;
    vec3 primaryRayDir     = normalize(primaryRayView.xyz - cameraPosition);

    // 首先，对主射线进行一次追踪，无论是否处于调试模式
    HitInfo primaryHitInfo;
    primaryHitInfo.tWorld = 1e20;
    primaryHitInfo.hit = false;
    int tlasRoot = int(tlasNodes.length()) - 1;
    traceTLAS_stack(tlasRoot, primaryRayOrig, primaryRayDir, primaryHitInfo);

     // --- 调试模式逻辑 -----------------------------------------------------------
    if (debugMode != 0) {
        if (primaryHitInfo.hit) {
            // 获取命中点的材质和几何信息
            Material mat = materials[primaryHitInfo.materialID];
            vec3 N_geometric = normalize(primaryHitInfo.normal);

            // 计算平滑法线 (如果存在法线贴图)
            vec3 N_shading = N_geometric;
            if (mat.normalTexture != -1) {
                vec3 normalMapSample = texture(textures[mat.normalTexture], primaryHitInfo.texCoord).rgb;
                normalMapSample = normalMapSample * 2.0 - 1.0;
                N_shading = normalize(primaryHitInfo.TBN * normalMapSample);
            }
            // 确保法线朝向相机 (对于可视化很重要)
            if (dot(N_shading, -primaryRayDir) < 0.0) N_shading = -N_shading;
            if (dot(N_geometric, -primaryRayDir) < 0.0) N_geometric = -N_geometric;


            vec3 albedo = (mat.baseColorTexture == -1) ? mat.baseColor : texture(textures[mat.baseColorTexture], primaryHitInfo.texCoord).rgb;
            float metallic = (mat.metallicTexture == -1) ? mat.metallic : texture(textures[mat.metallicTexture], primaryHitInfo.texCoord).r;
            float roughness = (mat.roughnessTexture == -1) ? mat.roughness : texture(textures[mat.roughnessTexture], primaryHitInfo.texCoord).r;
            const float MIN_ROUGHNESS = 0.04; // 确保与原始代码一致
            roughness = max(roughness, MIN_ROUGHNESS);

            switch (debugMode) {
                case 1: // 显示着色法线 (Shading Normals)
                    // 将法线从 [-1, 1] 映射到 [0, 1] 以用作颜色
                    finalOutputColor = (N_shading + 1.0) * 0.5;
                    break;
                case 2: // 显示基础颜色(Albedo)
                    finalOutputColor = albedo;
                    break;
                case 3: // 显示粗糙度 (Roughness)
                    // 粗糙度是标量，用灰度表示
                    finalOutputColor = vec3(roughness);
                    break;
                case 4: // 显示金属度 (Metallic)
                    // 金属度是标量，用灰度表示
                    finalOutputColor = vec3(metallic);
                    break;
                case 5: // 显示 UV 坐标 (UV Coordinates)
                    // 将 UV (通常在 [0, 1]) 映射到 RGB 颜色
                    finalOutputColor = vec3(primaryHitInfo.texCoord.x, primaryHitInfo.texCoord.y, 0.0);
                    break;
                case 6: // 显示深度 (Depth / Distance)
                    // 将距离映射到 [0, 1] 范围。需要根据场景大小调整分母。
                    // 50.0 是一个示例值，你可以根据你的场景深度调整
                    finalOutputColor = vec3(clamp(primaryHitInfo.tWorld / 50.0, 0.0, 1.0));
                    break;
                case 7: // 显示几何法线 (Geometric Normals)
                    // 将几何法线从 [-1, 1] 映射到 [0, 1] 以用作颜色
                    finalOutputColor = (N_geometric + 1.0) * 0.5;
                    break;
                default: // 未知的调试模式，显示洋红色
                    finalOutputColor = vec3(1.0, 0.0, 1.0);
                    break;
            }
        } else {
            // 没有命中物体，显示天空盒或特定背景色
            float phi   = atan(primaryRayDir.z, primaryRayDir.x);
            float theta = acos(primaryRayDir.y);
            float u = phi / (2.0 * PI) + 0.5;
            float v = theta / PI;
            finalOutputColor = texture(textures[skyboxIndex], vec2(u, v)).rgb;
        }
    } else { // debugMode == 0: 正常路径追踪渲染-----------------------------------------------------------

        const int MAX_BOUNCES = 5;
        const int SAMPLES_PER_PIXEL = 1;

        int tlasRoot = int(tlasNodes.length()) - 1;

        vec3 accumulateColor = vec3(0.0);

        for (uint samples = 0; samples < SAMPLES_PER_PIXEL; ++samples) {
            // Optional: 不同 sample 用一次跳跃 (‘leapfrog’) 防止序列重叠
            rng.state += 0x9e3779b9u;  // golden-ratio skip，可选
            // 生成 ray
            vec2 subPixel   = vec2(rng_nextFloat(rng), rng_nextFloat(rng)); // tent filter 更好
            vec2 ndc        = ((vec2(pixelCoord) + subPixel) / vec2(imageSize)) * 2.0 - 1.0;
            vec4 rayClip    = vec4(ndc, -1.0, 1.0);
            vec4 rayView    = inverse(viewProjectionMatrix) * rayClip;
            rayView        /= rayView.w;

            vec3 rayOrig    = cameraPosition;
            vec3 rayDir     = normalize(rayView.xyz - cameraPosition);
            vec3 throughput = vec3(1.0);
            vec3 radiance   = vec3(0.0);

            // 维护透射变量
            float etaExt = 1.0;     // 外侧的 η­­（空气）
            float etaInt = 1.0;     // 进入物体的 η
            bool  inside = false;   // 当前光线是否在介质内

            for (uint bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
                // 场景射线追踪--------------------------------------------------------------
                HitInfo hitInfo;
                hitInfo.tWorld = 1e20;
                hitInfo.hit = false;

                traceTLAS_stack(tlasRoot, rayOrig, rayDir, hitInfo);

                // 检测沿此方向的光源--------------------------------------------------------
                // 此逻辑仅用于标示面光位置
                float tLight = 1e30;      
                vec3 lightColor;
                float lightIntensity;    
                if (bounce == 0){
                    if (hitFaceLight(rayOrig, rayDir, tLight, lightColor, lightIntensity)) {
                        if (tLight < hitInfo.tWorld - 1e-4) {   // 最近的是光，不是物体
                            radiance += throughput * lightColor;      // 首次即命中面光源，直接返回，用于标识灯光位置
                            break;       // 直接出循环（不再反弹）
                        }
                    }
                }

                if (!hitInfo.hit) {        
                    // 将光线方向转换为等距柱状投影的UV坐标
                    // phi (longitude): 范围 [-PI, PI]，从X轴开始
                    // theta (latitude): 范围 [0, PI]，从Y轴开始
                    float phi   = atan(rayDir.z, rayDir.x); // 
                    float theta = acos(rayDir.y);  

                    // u: [-PI, PI] -> [0, 1]
                    // v: [0, PI] -> [0, 1]
                    float u = phi / (2.0 * PI) + 0.5; 
                    float v = theta / PI;    

                    vec3 envRadiance = texture(textures[skyboxIndex], vec2(u, v)).rgb;
                    if(bounce == 0) {
                        radiance = throughput * envRadiance; // 显示天空盒背景
                        break;
                    }
                    radiance += throughput * envRadiance * skyboxIntensity; // 天空盒辐射
                    break;
                }


                // 获取材质-------------------------------------------------------------------
                Material mat   = materials[hitInfo.materialID];
                vec3 N_geometric = normalize(hitInfo.normal); 
                // N 为最终用于光照计算的法线，默认为几何法线
                vec3 N = N_geometric; 

                if (mat.normalTexture != -1) { // 检查材质是否有法线贴图
                    // 采样法线贴图
                    vec3 normalMapSample = texture(textures[mat.normalTexture], hitInfo.texCoord).rgb;

                    // 将法线贴图值从 [0, 1] 范围转换到 [-1, 1] 范围
                    normalMapSample = normalMapSample * 2.0 - 1.0; 
                    // normalMapSample.y = -normalMapSample.y; // 可能需要在这里对 normalMapSample 的分量进行翻转

                    // 使用TBN矩阵将采样到的法线从切线空间转换到世界空间
                    N = normalize(hitInfo.TBN * normalMapSample);          
                }

                vec3 V         = -rayDir;
                if (dot(N, V) < 0.0) N = -N;
                float NdotV    = max(dot(N, V), 1e-4);

                vec3 albedo = vec3(0.5, 0.5, 0.5);
                if(mat.baseColorTexture == -1)   albedo = mat.baseColor;
                else    albedo = texture(textures[mat.baseColorTexture], hitInfo.texCoord).rgb;

                float metallic;
                if(mat.metallicTexture == -1)   metallic = mat.metallic;
                else    metallic = texture(textures[mat.metallicTexture], hitInfo.texCoord).r;

                float roughness;
                if(mat.roughnessTexture == -1)   roughness = mat.roughness;
                else    roughness = texture(textures[mat.roughnessTexture], hitInfo.texCoord).r;    
                // 常量：防止粗糙度为0
                const float MIN_ROUGHNESS = 0.04;
                roughness = max(roughness, MIN_ROUGHNESS);

                // 透射
                float transmission = mat.transmission;
                if(!inside) {
                    etaInt = mat.ior; 
                }
                bool sampledTransmission = false;

                // NEE 直接光源采样-------------------------------------------------------------------------
                // --- 用Fresnel估算各分支能量占比---
                float F0_scalar = pow((1.0 - mat.ior) / (1.0 + mat.ior), 2.0);
                vec3 F0 = mix(vec3(F0_scalar), albedo, metallic); 
                vec3 F_approx;
                if(metallic > 0.0)
                    F_approx = fresnelSchlickRoughness(clamp(dot(N, V), 0.0, 1.0), F0, roughness);
                else
                    F_approx = vec3(fresnelDielectric(clamp(dot(N, V), 0.0, 1.0), etaExt, etaInt));
                // 镜面 / 透射 / 漫反射 强度（标量化）
                float wS = dot(F_approx, vec3(0.2126,0.7152,0.0722));              // 镜面（亮度）
                float wT = (1.0 - metallic) * transmission
                         * dot(vec3(1.0) - F_approx, vec3(0.2126,0.7152,0.0722));   // 透射
                float wD = (1.0 - metallic) * (1.0 - transmission)
                         * dot(vec3(1.0) - F_approx, vec3(0.2126,0.7152,0.0722));   // 漫反射

                float sumW = wS + wD + wT + 1e-6;

                // 混合采样分支概率 （让 选择权重 与能量占比一致）
                float  pdfSelSpec    = wS / sumW;   // 镜面分支
                float  pdfSelTrans   = wT / sumW;   // 透射分支
                float  pdfSelDiffuse = wD / sumW;   // 漫反射分支

                vec3 direct = vec3(0.0);

                // -- 采样灯光 ---
                float selRand  = rng_nextFloat(rng);
                vec2  XiLight  = sobol2D(samples*17u, bounce*23u, rng.state);

                LightSample ls;
                float pdfLight = 1.0;

                if (selRand < 0.2 && faceLights.length() > 0)    
                {
                    uint idx  = uint(sampleLightIndex(rng));       // 按功率 CDF 采样，更可能采样到强光源
                    ls        = sampleFaceLight(idx, XiLight);

                    float seg = faceLights[idx].cdf - (idx==0?0.0:faceLights[idx-1].cdf);
                    pdfLight  = seg * ls.pdfArea * 0.2;                  // 面积 pdf
                }
                else                                                
                {
                    ls       = sampleDirectionalLight(XiLight);
                    pdfLight = ls.pdfArea * 0.8;     
                }

                // --- 开始直接光计算 ---
                vec3  L1      = (ls.isDirectional==1) ? normalize(ls.normal)
                                                : (ls.pos - hitInfo.hitPos);
                float distLight = length(L1);
                L1 /= distLight;

                if(ls.isDirectional == 1) 
                    distLight = 1e10; // 方向光，距离无穷大

                float NdotL1 = dot(N, L1);
                float L1Ndot = (ls.isDirectional==1) ? 1.0
                                              : max(dot(ls.normal, -L1), 0.0);

                if (NdotL1 > 0.0 && L1Ndot > 0.0)
                {
                    // 检测光源可见性
                    bool blocked = traceShadow(hitInfo.hitPos + N*1e-4,
                                               L1, distLight-2e-4, tlasRoot);
                    if (!blocked)
                    {
                        // 面光 pdf 转 solid-angle
                        if (ls.isDirectional == 0)
                            pdfLight *= (distLight*distLight) / L1Ndot;
                        pdfLight = max(pdfLight, 1e-4); 

                        // BSDF，与下方混合采样相同，暂时忽略透射
                        vec3  H     = normalize(V + L1);
                        float NDF   = DistributionGGX(N, H, roughness);
                        float G     = GeometrySmith(N, V, L1, roughness);
                        float cosTheta = clamp(abs(dot(V, H)), 0.0, 1.0);
                        vec3 F;
                        if (metallic > 0.0) {
                            F = fresnelSchlickRoughness(cosTheta, F0, roughness);
                        } else {
                            F = vec3(fresnelDielectric(cosTheta, etaExt, etaInt));
                        }
                        vec3 spec   = (NDF*G*F) / (4.0*NdotV*NdotL1+1e-4);
                        vec3 kD     = vec3(1.0) - F;
                        kD         *= (1.0 - metallic) * (1.0 - transmission);  
                        vec3 diff   = kD * albedo / PI;
                        vec3 f_brdf = diff + spec;

                        // 计算混合采样pdf
                        float pdfSpec = DistributionGGX(N,H,roughness)
                                       * max(dot(N,H),0.0)
                                       / (4.0*max(dot(H,V),1e-4));
                        float pdfDiff = NdotL1 / PI;
                        float pdfBsdf = pdfSelSpec * pdfSpec + pdfSelDiffuse * pdfDiff;
                        pdfBsdf = max(pdfBsdf, 1e-4);

                        // 计算MIS 权重
                        float w = (pdfLight*pdfLight) /
                                  (pdfLight*pdfLight + pdfBsdf*pdfBsdf);

                        direct = throughput * f_brdf * ls.radiance
                               * NdotL1 * w / pdfLight;
                    }
                }

                radiance += direct;           // ← 立即累加直接光

                // 混合重要性采样 ------------------------------------------------------------

                // 选择分支
                vec2  Xi      = sobol2D(samples, bounce, rng.state);
                float rTech   = rng_nextFloat(rng);

                vec3  L2;          // 采样出的入射方向（第二个）
                vec3 H;

                if (rTech < pdfSelSpec)      // --- 镜面分支：GGX importance sample ---
                {
                    H = ImportanceSampleGGX(Xi, N, roughness);
                    L2     = normalize(reflect(-V, H));                               
                } else if(rTech < pdfSelSpec + pdfSelDiffuse) // --- 漫反射分支：余弦加权 ---
                {
                    L2     = cosineSampleHemisphere(Xi, N);
                    H      = normalize(V + L2); // 计算 H 向量    
                } else                         // --- 透射分支：GGX importance sample ---
                {
                    sampledTransmission = true;
                    H = ImportanceSampleGGX(Xi, N, roughness);
                    float eta = etaExt / etaInt;
                    vec3 T = refract(-V, H, eta);
                    if (length(T) == 0.0) {      // 回退为漫反射/镜面分支
                        sampledTransmission = false;
                        L2     = normalize(reflect(-V, H));  
                    }else{
                        L2     = T;
                        H = normalize(V * eta + T);  // 透射 Half-vector 定义
                        if (dot(N, H) < 0.0) H = -H;    // 保证 N·H ≥ 0
                    }
                }

                float NdotL2    = abs(dot(N, L2));

                // 混合采样 pdf 和 MIS 权重
                float pdfCombined;
                float misW;

                vec3 f_bsdf;       // 总 BSDF
                float  NDF = DistributionGGX(N, H, roughness);
                float  G   = GeometrySmith(N, V, L2, roughness);
                float cosTheta = clamp(abs(dot(V, H)), 0.0, 1.0);

                if(sampledTransmission){
                    vec3 F = vec3(fresnelDielectric(cosTheta, etaExt, etaInt)); 
                    // 透射项
                    // ---Physically Based Rendering 4ed (eq 9.37 & 9.40)---
                    float eta = etaExt / etaInt;
                    float VdotH = dot(V, H);
                    float HdotL2 = dot(H, L2);
                    float denom = (VdotH + HdotL2/eta);
                    denom       = max(1e-4, denom*denom);
                    float nom = abs(dot(V,N)) * abs(dot(L2,N));
                    vec3 trans  = abs(VdotH) * abs(HdotL2) * albedo * (1.0 - F) * G * NDF / (denom * nom); 
                    float NdotH = abs(dot(N, H));
                    float pdfTrans = (NDF * NdotH * abs(dot(L2,H))) / denom;

                    // ---简化近似版---
                    // float transmissionWeight = transmission      
                    //          * (1.0 - metallic); 
                    // // 可给玻璃一点着色 (albedo)，也可用 vec3(1.0)
                    // vec3 trans = transmissionWeight * vec3(1.0) / PI;
                    // float pdfTrans = abs(dot(N, L2)) / PI; 

                    f_bsdf = trans;

                    pdfCombined = pdfSelTrans * pdfTrans;
                    misW = 1.0f;
                }else{
                    vec3 F;
                    if(metallic > 0.0)
                        F = fresnelSchlickRoughness(cosTheta, F0, roughness);

                    else
                        F = vec3(fresnelDielectric(cosTheta, etaExt, etaInt));
                    vec3 spec   = (NDF * G * F) / (4.0 * NdotV * NdotL2 + 1e-4);
                    vec3 kD = vec3(1.0) - F;
                    kD *= (1.0 - metallic) * (1.0 - transmission);    
                    vec3 diff   = kD * albedo / PI;
                    f_bsdf = spec + diff;

                    float NdotH = max(dot(N, H), 1e-4);
                    float HdotV = max(dot(H, V), 1e-4);
                    float pdfSpec = NDF * NdotH / (4.0 * HdotV + 1e-4);     
                    float pdfDiff      = abs(dot(N, L2)) / PI;  

                    pdfCombined = pdfSelSpec*pdfSpec + pdfSelDiffuse*pdfDiff;
                }
                pdfCombined = max(pdfCombined,1e-4);

                throughput *= f_bsdf * NdotL2 / pdfCombined;
                throughput = max(throughput, 1e-4);

                // 光线更新
                rayDir = L2;
                if(transmission > 0.0){
                    rayOrig = hitInfo.hitPos + rayDir * 1e-4; 
                }else{
                    rayOrig = hitInfo.hitPos + N * 1e-4;    // 防止自交
                }

                // 透射更新
                if (sampledTransmission) {
                    inside = !inside;
                    // 交换内外折射率，供下一次 bounce 使用
                    float tmp = etaExt;
                    etaExt = etaInt;
                    etaInt = tmp;
                }

                // 其他更新-----------------------------------------------------
                // Russian Roulette（从第三跳开始）
                if (bounce > 3) {
                    float p = max(throughput.r, max(throughput.g, throughput.b));
                    if (p < 1e-5f) { // 如果 throughput 已经很小，直接终止路径
                        break;
                    }
                    if (rng_nextFloat(rng) > p) {
                        break; // RR 终止
                    }
                    throughput /= p; // 补偿能量
                }

                // 更新 rng 的“反弹维度”信息 (可选)
                rng.state += uint( bounce * 0x632be59bu );   // decorrelate per bounce
            }

            // 钳制单次采样贡献的能量，防止萤火虫
            // 阈值需要根据场景亮度进行调整
            float radianceMax = 20.0;
            if(any(greaterThan(radiance, vec3(radianceMax)))){
                // 如果任何颜色通道超过阈值，按比例缩放，保持色调
                radiance *= radianceMax / max(radiance.r, max(radiance.g, radiance.b));
            }
            accumulateColor += radiance;
        }

        finalOutputColor = accumulateColor / float(SAMPLES_PER_PIXEL); // 平均
    }

        // 帧间缓存
        vec3 prevColor = imageLoad(accumImage, ivec2(pixelCoord)).rgb;
        vec3 blended = (frameIndex == 0 || debugMode != 0) // 如果是第一帧或在调试模式，不进行累积
        ? finalOutputColor
        : (prevColor * frameIndex + finalOutputColor) / float(frameIndex + 1);


        imageStore(resultImage, ivec2(pixelCoord), vec4(blended, 1.0));
}


// ---------- AABB Intersection ----------
bool intersectAABB(vec3 origin, vec3 dir, AABB aabb) {
    vec3 invDir = 1.0 / dir;
    vec3 t0 = (aabb.min - origin) * invDir;
    vec3 t1 = (aabb.max - origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tEnter = max(max(tmin.x, tmin.y), tmin.z);
    float tExit  = min(min(tmax.x, tmax.y), tmax.z);
    return tExit >= max(tEnter, 0.0);
}

// ---------- Triangle Intersection ----------
bool intersectTriangle(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2, out float t) {
    const float EPSILON = 0.000001;
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(dir, edge2);
    float a = dot(edge1, h);
    if (abs(a) < EPSILON)
        return false; // 射线与三角形平行
    float f = 1.0 / a;
    vec3 s = orig - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    vec3 q = cross(s, edge1);
    float v = f * dot(dir, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    t = f * dot(edge2, q);
    return t > EPSILON;
}

// ---------- Triangle Intersection and Barycentric Coordinates ----------
bool intersectTriangleBarycentric(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2, out float t, out vec3 baryCoord) {
    const float EPSILON = 0.000001;
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(dir, edge2);
    float a = dot(edge1, h);
    if (abs(a) < EPSILON) return false;
    float f = 1.0 / a;
    vec3 s = orig - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return false;
    vec3 q = cross(s, edge1);
    float v = f * dot(dir, q);
    if (v < 0.0 || u + v > 1.0) return false;
    t = f * dot(edge2, q);
    if (t < EPSILON) return false;
    baryCoord = vec3(1.0 - u - v, u, v);
    return true;
}

// ---------- Non-Recursive Traversal ----------

const int MAX_STACK_SIZE = 128;

void traceTLAS_stack(int rootIndex, vec3 rayOrig, vec3 rayDir, inout HitInfo hitInfo) {
    int stack[MAX_STACK_SIZE];
    int sp = 0;
    stack[sp++] = rootIndex;

    while (sp > 0) {
        int nodeIndex = stack[--sp];
        TLASNode node = tlasNodes[nodeIndex];
        if (!intersectAABB(rayOrig, rayDir, node.bounds)) continue;

        if (node.left == -1 && node.right == -1) {
            int instanceIndex = node.instanceIndex;
            TLASInstance inst = instances[instanceIndex];
            mat4 model = inst.transform;
            mat4 invModel = inverse(model);
            // hitInfo.materialID = inst.materialID;

            traceBLAS_stack(inst.rootNodeIndex, rayOrig, rayDir, model, inst.materialID, inst.baseIndexOffset, hitInfo);
        } else {
            if (node.right >= 0) stack[sp++] = node.right;
            if (node.left >= 0)  stack[sp++] = node.left;
        }

        if (sp >= MAX_STACK_SIZE) break;
    }
}

void traceBLAS_stack(int rootIndex, vec3 rayOrig, vec3 rayDir, mat4 model, int materialID, int baseIndexOffset, inout HitInfo hitInfo) {
    int stack[MAX_STACK_SIZE];
    int sp = 0;
    stack[sp++] = rootIndex;

    mat4 invModel = inverse(model);
    vec3 localOrigin = vec3(invModel * vec4(rayOrig, 1.0));
    vec3 localDir = normalize(mat3(invModel) * rayDir);

    mat3 normalMatrix = mat3(transpose(invModel)); 

    while (sp > 0) {
        int nodeIndex = stack[--sp];
        BLASNode node = blasNodes[nodeIndex];
        if (!intersectAABB(localOrigin, localDir, node.bounds)) continue;

        if (node.right < 0 && node.left < 0) {
            for (int i = 0; i < 3; ++i) {
                int localIndex = node.indices[i];
                if(localIndex == -1) break;
                int idx = baseIndexOffset  + localIndex;
                Vertex v0 = vertices[indices[idx + 0]];
                Vertex v1 = vertices[indices[idx + 1]];
                Vertex v2 = vertices[indices[idx + 2]];

                // // --- 背面剔除 ---
                // // 在局部空间计算三角形的几何法线
                // vec3 p0 = v0.Position;
                // vec3 p1 = v1.Position;
                // vec3 p2 = v2.Position;
                // vec3 geomNormalLocal = cross(p1 - p0, p2 - p0);
                // // 将几何法线与局部空间的光线方向进行点积
                // // 如果点积大于0 ，说明光线是从背面击中三角形
                // // 对于不透明表面，应该忽略这次命中
                // if (dot(geomNormalLocal, localDir) > -1e-6) {
                //     continue; // 跳过这个三角形，处理叶子节点中的下一个
                // }

                float t;
                vec3 baryCoord;
                if (intersectTriangleBarycentric(localOrigin, localDir, v0.Position, v1.Position, v2.Position, t, baryCoord)) {
                    // 局部空间交点转为世界空间，计算世界空间下的距离
                    vec3 hitLocal = localOrigin + t * localDir;
                    vec3 hitWorld = vec3(model * vec4(hitLocal, 1.0));
                    float tWorld = length(hitWorld - rayOrig);

                    if (tWorld < hitInfo.tWorld) {
                        hitInfo.tWorld = tWorld;
                        hitInfo.hitPos = hitWorld;
                        hitInfo.materialID = materialID;  
                        hitInfo.hit = true;

                        // --- 插值法线 & 纹理坐标 ---
                        vec3 n0 = normalize(normalMatrix * v0.Normal);
                        vec3 n1 = normalize(normalMatrix * v1.Normal);
                        vec3 n2 = normalize(normalMatrix * v2.Normal);
                        hitInfo.normal = normalize(n0 * baryCoord.x + n1 * baryCoord.y + n2 * baryCoord.z);

                        vec2 uv = v0.TexCoords * baryCoord.x + v1.TexCoords * baryCoord.y + v2.TexCoords * baryCoord.z;
                        hitInfo.texCoord = uv;

                        // --- TBN Matrix Calculation ---
                        vec3 localTangent = v0.Tangent * baryCoord.x + v1.Tangent * baryCoord.y + v2.Tangent * baryCoord.z;
                        vec3 worldTangent = normalize(mat3(model) * localTangent);
                        worldTangent = normalize(worldTangent - dot(worldTangent, hitInfo.normal) * hitInfo.normal);
                        float interpolatedTangentW = v0.tangentW * baryCoord.x + v1.tangentW * baryCoord.y + v2.tangentW * baryCoord.z;
                        vec3 worldBitangent = normalize(cross(worldTangent, hitInfo.normal) * interpolatedTangentW);
                        hitInfo.TBN = mat3(worldTangent, worldBitangent, hitInfo.normal);
                    }
                }
            }
        } else {
            if (node.right >= 0) stack[sp++] = node.right;
            if (node.left >= 0)  stack[sp++] = node.left;
        }

        if (sp >= MAX_STACK_SIZE) break;
    }
}

// 简化版，仅用于检测某点的光源可见性
bool traceShadow(vec3 rayOrig, vec3 rayDir, float maxDist, int rootIndex) {
    int stack[MAX_STACK_SIZE];
    int sp = 0;
    stack[sp++] = rootIndex;

    while (sp > 0) {
        int nodeIndex = stack[--sp];
        TLASNode node = tlasNodes[nodeIndex];
        if (!intersectAABB(rayOrig, rayDir, node.bounds))
            continue;

        if (node.left == -1 && node.right == -1) {
            int instanceIndex = node.instanceIndex;
            TLASInstance inst = instances[instanceIndex];
            mat4 model = inst.transform;
            mat4 invModel = inverse(model);

            if (traceBLAS_shadow(inst.rootNodeIndex, rayOrig, rayDir, model, maxDist, inst.baseIndexOffset))
                return true; // 被遮挡，提前返回
        } else {
            if (node.right >= 0) stack[sp++] = node.right;
            if (node.left  >= 0) stack[sp++] = node.left;
        }

        if (sp >= MAX_STACK_SIZE) break;
    }

    return false; // 没有任何遮挡
}

bool traceBLAS_shadow(int rootIndex, vec3 rayOrig, vec3 rayDir, mat4 model, float maxDist, int baseIndexOffset) {
    int stack[MAX_STACK_SIZE];
    int sp = 0;
    stack[sp++] = rootIndex;

    mat4 invModel = inverse(model);
    vec3 localOrigin = vec3(invModel * vec4(rayOrig, 1.0));
    vec3 localDir = normalize(mat3(invModel) * rayDir);

    while (sp > 0) {
        int nodeIndex = stack[--sp];
        BLASNode node = blasNodes[nodeIndex];
        if (!intersectAABB(localOrigin, localDir, node.bounds))
            continue;

        if (node.right < 0 && node.left < 0) {
            for (int i = 0; i < 3; ++i) {
                int localIndex = node.indices[i];
                if (localIndex == -1) break;
                int idx = baseIndexOffset + localIndex;

                Vertex v0 = vertices[indices[idx + 0]];
                Vertex v1 = vertices[indices[idx + 1]];
                Vertex v2 = vertices[indices[idx + 2]];

                float t;
                if (intersectTriangle(localOrigin, localDir, v0.Position, v1.Position, v2.Position, t)) {
                    vec3 hitLocal = localOrigin + t * localDir;
                    vec3 hitWorld = vec3(model * vec4(hitLocal, 1.0));
                    float tWorld = length(hitWorld - rayOrig);

                    if (tWorld < maxDist) {
                        return true; // 有遮挡物
                    }
                }
            }
        } else {
            if (node.right >= 0) stack[sp++] = node.right;
            if (node.left  >= 0) stack[sp++] = node.left;
        }

        if (sp >= MAX_STACK_SIZE) break;
    }

    return false; // 无遮挡
}


bool hitFaceLight(vec3 ro, vec3 rd, out float tMin, out vec3 color, out float intensity)
{
    tMin      = 1e30;
    bool hit  = false;

    for (int i = 0; i < faceLights.length(); ++i)
    {
        vec3 v0 = faceLights[i].v0;
        vec3 v1 = faceLights[i].v1;
        vec3 v2 = faceLights[i].v2;
        vec3 v3 = faceLights[i].v3;

        vec3 normal = normalize(cross(v1 - v0, v2 - v0));

        // 只从正面照明
        if (dot(normal, rd) > 0.0) continue;

        float t0, t1;
        bool h0 = intersectTriangle(ro, rd, v0, v1, v2, t0);
        bool h1 = intersectTriangle(ro, rd, v2, v3, v0, t1);

        if (h0 && t0 < tMin) {
            tMin = t0;
            hit = true;
            color = faceLights[i].color;
            intensity = faceLights[i].intensity;
        }
        if (h1 && t1 < tMin) {
            tMin = t1;
            hit = true;
            color = faceLights[i].color;
            intensity = faceLights[i].intensity;
        }
    }
    return hit;
}

bool hitDirectionalLight(vec3 rayDir, out vec3 color, out float intensity) {
    // 可调节的软硬度阈值，越接近1越“硬”，越小越“软”
    const float softnessThreshold = 0.95; // 约为18度锥角

    for (int i = 0; i < directionalLights.length(); ++i) {
        float cosTheta = dot(rayDir, -directionalLights[i].direction);
        if (cosTheta > softnessThreshold) {
            color = directionalLights[i].color;
            // 边缘平滑
            float softFactor = (cosTheta - softnessThreshold) / (1.0 - softnessThreshold);
            intensity = directionalLights[i].intensity * softFactor;
            return true;
        }
    }
    return false;
}

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// ---- PCG32 --------------------------------------------------------------------

// 乘数与增量来自 PCG 论文，选这两个可以保证 full-period
const uint PCG32_MULT = 747796405u;
const uint PCG32_INC  = 2891336453u;

uint splitmix32(uint x)
{
    x += 0x9e3779b9u;                 // 爬山常量 (φ·2³²)
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

RNGState rng_init(uvec2 pixel, uint frame, uint bounce)
{
    // 先把所有维度搅成一个 32-bit 值
    uint seed =
      (pixel.x * 1664525u)  ^ (pixel.y * 1013904223u) ^
      (frame   * 69069u)    ^ (bounce  * 362437u);

    seed = splitmix32(seed);      // 再做一次 avalanching
    return RNGState(seed | 1u);   // 避免 state==0
}

// 启用64-bits可用版本
// RNGState  rng_init(uvec2 pixel, uint frame, uint bounce)
// {
//     // 1. 先把 (pixel, frame, bounce) 打包成 64-bit seed
//     uint seed_hi = (pixel.x << 16) ^ pixel.y;      // 低熵像素混合
//     uint seed_lo = (frame  << 3)  ^ (bounce<<19);  // 帧 & 反弹混合到不同位

//     // 2. 使用 64->32 的简易 avalanching（Xorshift64* 一轮）
//     uint64_t z = (uint64_t(seed_hi) << 32) | seed_lo;
//     z ^= z >> 12;  z ^= z << 25;  z ^= z >> 27;
//     uint state = uint(z * 2685821657736338717ul);  // 随机常量

//     // 3. 不能让 state=0，否则 PCG 周期退化
//     return RNGState(state | 1u);
// }

uint rng_next(inout RNGState rng)
{
    rng.state = rng.state * PCG32_MULT + PCG32_INC;         // LCG
    uint xorshifted = ((rng.state >> 18u) ^ rng.state) >> 27u;
    uint rot        = rng.state >> 28u;
    return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
}

float rng_nextFloat(inout RNGState rng)   // (0,1)
{
    return (rng_next(rng) >> 8) * (1.0 / 16777216.0);  // 24 有效 mantissa
}
vec2 rng_nextFloat2(inout RNGState rng)
{
    uint v = rng_next(rng);
    return vec2( (v & 0xffffu) * (1.0/65536.0),
                 (v >> 16)     * (1.0/65536.0) );
}


uint reverseBits(uint v)
{
    return bitfieldReverse(v);   // 反转 32 bit；高低位互换
}

// 单维 Owen scramble
float sobolOwen(uint index, uint scramble, uint dim) {
    index = reverseBits(index);
    index ^= splitmix32(scramble ^ dim);   // 每维扰动
    return float(index) * 2.3283064365386963e-10;
}

// 生成 (d0,d1) 两维，dPair 起点随 bounce 递增
vec2 sobol2D(uint index, uint bounce, uint scramble) {
    uint d0 = bounce * 2u + 0u;        // 每个 bounce 占两维
    uint d1 = bounce * 2u + 1u;
    return vec2(
        sobolOwen(index << 0u, scramble, d0),   // 维度哈希进 index
        sobolOwen(index << 0u, scramble, d1)
    );
}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 fresnelSchlickRoughness(float NdotV, vec3 F0, float rough)
{
    float r = clamp(rough, 0.02, 1.0);
    return F0 + (max(vec3(1.0 - r), F0) - F0) * pow(1.0 - NdotV, 5.0);
}

float fresnelDielectric(float cosThetaI, float etaI, float etaT)   
{
    cosThetaI = clamp(cosThetaI, -1.0, 1.0);
    bool entering = cosThetaI > 0.0;
    if (!entering) {      // 反向时交换介质
        float tmp  = etaI; etaI = etaT; etaT = tmp;
        cosThetaI  = abs(cosThetaI);
    }

    float sin2ThetaI = max(0.0, 1.0 - cosThetaI * cosThetaI);
    float eta = etaI / etaT;
    float sin2ThetaT = eta * eta * sin2ThetaI;

    // 全内反射
    if (sin2ThetaT >= 1.0) return 1.0;

    float cosThetaT = sqrt(max(0.0, 1.0 - sin2ThetaT));
    float Rs = (etaI * cosThetaI - etaT * cosThetaT) /
               (etaI * cosThetaI + etaT * cosThetaT);
    float Rp = (etaT * cosThetaI - etaI * cosThetaT) /
               (etaT * cosThetaI + etaI * cosThetaT);
    return 0.5 * (Rs * Rs + Rp * Rp);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// for IBL
// float GeometrySchlickGGX(float NdotV, float roughness)
// {
//     float r = (roughness + 1.0);
//     float k = (r*r) / 8.0;

//     float num   = NdotV;
//     float denom = NdotV * (1.0 - k) + k;

//     return num / denom;
// }

// for direct lighting
// float GeometrySchlickGGX(float NdotV, float k)
// {
//     float nom   = NdotV;
//     float denom = NdotV * (1.0 - k) + k;

//     return nom / denom;
// }
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float k = (roughness + 1.0);      // = α'   where α' = (α+1)^2 / 8
    k = (k*k) / 8.0;                 // 物理推导见 Heitz 2014
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = abs(dot(N, V));  
    float NdotL = abs(dot(N, L)); 
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 均匀半球取样 
vec3 uniformSampleHemisphere_local(vec2 Xi)
{
    float cosTheta = Xi.x;                     // z 轴（= cosθ）在 [0,1)
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi      = 6.28318530718 * Xi.y;     // 2π·u

    return vec3(
        sinTheta * cos(phi),                   // x
        sinTheta * sin(phi),                   // y
        cosTheta                               // z
    );
}

vec3 uniformSampleHemisphere(vec2 Xi, vec3 N)
{
    // 1) 局部半球采样（+Z 朝上）
    vec3 L_local = uniformSampleHemisphere_local(Xi);

    // 2) 构建切线基 (T, B, N)
    vec3 T = normalize(abs(N.z) < 0.999 ? cross(N, vec3(0.0, 0.0, 1.0))
                                        : cross(N, vec3(0.0, 1.0, 0.0)));
    vec3 B = cross(T, N);

    // 3) 变换回世界
    return L_local.x * T +
           L_local.y * B +
           L_local.z * N;
}

/// 生成与 N 对齐、余弦加权的半球方向
vec3 cosineSampleHemisphere(vec2 Xi, vec3 N)
{
    //--------------------------------------
    // 1. 先在局部坐标系 (T,B,N) 采样
    //--------------------------------------
    float r   = sqrt(Xi.x);            // 半径开平方 -> cos 权重
    float phi = 2.0 * PI * Xi.y;

    float x = r * cos(phi);            // 切平面坐标
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - Xi.x));   // 这里直接用 Xi.x

    //--------------------------------------
    // 2. 构建正交基 (T,B,N)
    //--------------------------------------
    vec3 T = (abs(N.z) < 0.999)
           ? normalize(cross(N, vec3(0.0, 0.0, 1.0)))
           : normalize(cross(N, vec3(1.0, 0.0, 0.0)));
    vec3 B = cross(T, N);

    //--------------------------------------
    // 3. 变换到世界空间
    //--------------------------------------
    vec3 L = x * T + y * B + z * N;
    return normalize(L);               // 理论上已归一，但保持稳健
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float sampleLightIndex(inout RNGState rng)
{
    float xi = rng_nextFloat(rng);
    // 二分搜索 CDF（假设已排序在 [0,1)）
    int l=0, h=faceLights.length()-1;
    while(l<h){
        int m=(l+h)>>1;
        if(xi < faceLights[m].cdf) h=m; else l=m+1;
    }
    return float(l);
}

LightSample sampleFaceLight(uint idx, vec2 Xi)
{
    FaceLight L = faceLights[idx];


    // 用面积权重决定落在哪个三角形
    vec3 p;          // 最终采样坐标
    vec3 n;          // 面法线（方向统一为背光面朝外）
    float pdfArea;   // 面积 pdf

    // 三角划分：(v0,v1,v3) + (v2,v3,v1)
    vec3   e01 = L.v1 - L.v0;
    vec3   e03 = L.v3 - L.v0;
    float  area1 = 0.5 * length(cross(e01, e03));

    vec3   e23 = L.v3 - L.v2;
    vec3   e21 = L.v1 - L.v2;
    float  area2 = 0.5 * length(cross(e23, e21));

    float totalArea = area1 + area2;

    // 均匀采样对应三角形
    float  triSelectProb = area1 / totalArea;
    bool   inTri1        = Xi.x < triSelectProb;

    // 重新映射随机数到目标三角形
    float  u        = inTri1 ? (Xi.x / triSelectProb)
                             : ((Xi.x - triSelectProb) / (1.0 - triSelectProb));
    float  v        = Xi.y;

    float  sqrt_u   = sqrt(u);
    float  b0 = 1.0 - sqrt_u;
    float  b1 = v * sqrt_u;
    float  b2 = (1.0 - v) * sqrt_u;

    if (inTri1)               // △(v0,v1,v3)
    {
        p = L.v0 * b0 + L.v1 * b1 + L.v3 * b2;
        n = normalize(cross(e01, e03));
    }
    else                      // △(v2,v3,v1)
    {
        p = L.v2 * b0 + L.v3 * b1 + L.v1 * b2;
        n = normalize(cross(e23, e21));
    }

    // 填写采样结果
    LightSample ls;
    ls.pos      = p;
    ls.normal   = n;
    ls.radiance = L.color * L.intensity;
    ls.pdfArea  = 1.0 / totalArea;   // 全灯面均匀，所以统一用 totalArea
    ls.isDirectional = 0; // 面光源
    return ls;
}

vec3 sampleCone(vec3 axis, float cosThetaMax, vec2 Xi, out float pdfSolidAngle)
{
    float cosTheta = mix(1.0, cosThetaMax, Xi.x);  // Xi.x ∈ [0,1]
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * Xi.y;

    // 局部坐标下
    vec3 localDir = vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );

    // 构建正交基（TBN）并转换回世界坐标
    vec3 up = abs(axis.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, axis));
    vec3 bitangent = cross(axis, tangent);

    vec3 dir = tangent * localDir.x + bitangent * localDir.y + axis * localDir.z;

    pdfSolidAngle = 1.0 / (2.0 * PI * (1.0 - cosThetaMax));
    return dir;
}

LightSample sampleDirectionalLight(vec2 Xi)  
{
    DirectionalLight d = directionalLights[0];              // 仅支持 1 个方向光
    float thetaMax     = d.angularRadius;                    // 半角
    float cosThetaMax  = cos(thetaMax);
    float coneSolidAngle = 2.0 * PI * (1.0 - cosThetaMax);

    LightSample ls;
    ls.radiance = d.color * d.intensity / coneSolidAngle; 

    // 阴影：在圆锥内均匀采样
    float pdfSA;
    vec3  lDir = sampleCone(-d.direction, cosThetaMax, Xi, pdfSA);  // 指向光源，锥形采样
    ls.pos      = vec3(0.0);      // “无限远”光源，位置无意义
    ls.normal   = lDir;      
    ls.pdfArea  = pdfSA;            // solid-angle PDF
    ls.isDirectional = 1;         // 方向光
    return ls;
}



