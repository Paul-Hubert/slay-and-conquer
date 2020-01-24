#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include "include_vk.h"

#include <tinyobjloader/tiny_obj_loader.h>
#include <stb/stb_image.h>

#include <vector>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <QDebug>

class Windu;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normals;
    glm::vec2 texCoord;
    
    bool operator==(const Vertex& other) const {
        return pos == other.pos && normals == other.normals && texCoord == other.texCoord;
    }
};

struct Image {
    vk::Image image;
    vk::ImageView view;
    vk::Format format;
    stbi_uc* pixels;
    vk::DeviceSize size;
    uint32_t width;
    uint32_t height;
    uint32_t comp;
    bool isOwned;
};

struct Material {
    glm::vec4 color;
    Image* image;
    vk::DescriptorSet tex;
    bool hasTexture;
};

struct SubObject {
    Material* mat;
    uint32_t first_index;
    uint32_t num_index;
};

struct Object {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vk::DeviceSize offset;
    std::vector<SubObject> sub;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.normals) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

QDebug operator<< (QDebug debug, const Vertex vertex);

class ObjectSystem {
public:

    ObjectSystem(Windu* win);
    void preinit();
    void init();
    Object loadModel(QString objPath, QString mtlPath);
    void initMaterial(const tinyobj::material_t& mat);
    Image* initImage(const QString path, int req_comp, vk::Format format);
    Image* initImage(unsigned char* img, int width, int height, int req_comp, vk::Format format);
    vk::DescriptorSet getDescriptorSetFromImage(Image* image);
    void transitionImages(vk::CommandBuffer buffer);
    std::vector<Object> objects;
    vk::Sampler sampler;
    vk::DescriptorSetLayout layout;
    ~ObjectSystem();
    
    const vk::Format defaultFormat = vk::Format::eR8G8B8A8Srgb;
    
    vk::DescriptorPool pool;
    
private:
    Windu *win;
    size_t vert_size = 0;
    size_t img_size = 0;
    std::list<Material> materials;
    std::list<Image> images;
    vk::DeviceMemory imageMemory;
};

#endif
