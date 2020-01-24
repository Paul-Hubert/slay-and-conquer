#include "object_system.h"

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <QDebug>
#include <QTemporaryDir>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>

#include "util/resource_copy.h"

#include <iostream>

#include "helper.h"

#include "windu.h"

ObjectSystem::ObjectSystem(Windu* win) {
    this->win = win;
}

void ObjectSystem::preinit() {
    
    win->resman.allocateResource(FO_RESOURCE_VERTEX_BUFFER, vert_size);
    
    win->resman.allocateResource(FO_RESOURCE_STAGING_BUFFER, vert_size+img_size+1000);
    
}

void ObjectSystem::init() {
    
    FoBuffer* vertex = win->resman.getBuffer(FO_RESOURCE_VERTEX_BUFFER);
    FoBuffer* staging = win->resman.getBuffer(FO_RESOURCE_STAGING_BUFFER);
    
    {
        char* data = static_cast<char*> (win->device.logical.mapMemory(staging->memory, staging->offset, staging->size, {}));
        for (Object& obj : objects) {
            memcpy(data + obj.offset, obj.vertices.data(), obj.vertices.size() * sizeof(Vertex));
            memcpy(data + obj.offset + obj.vertices.size() * sizeof(Vertex), obj.indices.data(), obj.indices.size() * sizeof(uint32_t));
            obj.vertices.clear();
            obj.vertices.shrink_to_fit();
            obj.indices.clear();
            obj.indices.shrink_to_fit();
        }
        vk::DeviceSize offset = 0;
        for (const Image& image : images) {
            
            offset = ((vert_size + offset) / (image.comp*4) + 1) * (image.comp*4) - vert_size;
            
            memcpy(data + vert_size + offset, image.pixels, image.size);
            offset += image.size;
            
            if(image.isOwned) stbi_image_free(image.pixels);
            
        }
        win->device.logical.unmapMemory(staging->memory);
    }
    
    vk::MemoryRequirements memreq = {};
    for (Image& image : images) {
        auto imageInfo = vk::ImageCreateInfo({}, vk::ImageType::e2D, image.format, vk::Extent3D(image.width, image.height, 1), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive, 1, &win->device.t_i, vk::ImageLayout::eUndefined);
        
        image.image = win->device.logical.createImage(imageInfo);
        
        vk::MemoryRequirements memRequirements = win->device.logical.getImageMemoryRequirements(image.image);
        memreq.alignment = memRequirements.alignment > memreq.alignment ? memRequirements.alignment : memreq.alignment;
    }
    
    for (const Image& image : images) {
        vk::MemoryRequirements memRequirements = win->device.logical.getImageMemoryRequirements(image.image);
        memreq.size += (memRequirements.size/memreq.alignment+1)*memreq.alignment;
        memreq.memoryTypeBits = memRequirements.memoryTypeBits;
    }
    
    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memreq.size;
    allocInfo.memoryTypeIndex = win->device.getMemoryType(memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    imageMemory = win->device.logical.allocateMemory(allocInfo);
    
    win->transfer.record([&](const vk::CommandBuffer& buffer) -> void {
        
        buffer.copyBuffer(staging->buffer, vertex->buffer, vk::BufferCopy(0, 0, vert_size));
        
        vk::DeviceSize offset = 0, deviceoffset = 0;
        for (const Image& image : images) {
            
            vk::MemoryRequirements memRequirements = win->device.logical.getImageMemoryRequirements(image.image);
            win->device.logical.bindImageMemory(image.image, imageMemory, deviceoffset);
            deviceoffset += (memRequirements.size/memreq.alignment+1)*memreq.alignment;
            
            auto barrier = vk::ImageMemoryBarrier(vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferRead,
                                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                                  win->device.t_i, win->device.t_i, image.image,
                                                  vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                                                 );

            buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer,
                {},
                {},
                {},
                {barrier}
            );

            offset = ((vert_size + offset) / (image.comp*4) + 1) * (image.comp*4) - vert_size;
            
            buffer.copyBufferToImage(staging->buffer, image.image, vk::ImageLayout::eTransferDstOptimal,
                                     {vk::BufferImageCopy(vert_size + offset, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                         {0,0,0}, {image.width, image.height, 1})});
            
            auto barrier2 = vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                                                  vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                                  win->device.t_i, win->device.g_i, image.image,
                                                  vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                                                 );

            buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                {},
                {},
                {},
                {barrier2}
            );
            
            offset += image.size;
        }
        
    }, &win->renderer, vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eFragmentShader);
    
    for (Image& image : images) {
        
        image.view = win->device.logical.createImageView(vk::ImageViewCreateInfo(
            {}, image.image, vk::ImageViewType::e2D, image.format, {},
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        ));
        
    }
    
    
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 0;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    sampler = win->device.logical.createSampler(samplerInfo);
    
    
    // CREATE DESCRIPTORS
    
    // POOL
    auto size = std::vector<vk::DescriptorPoolSize> {{vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t> (images.size())}};
    pool = win->device.logical.createDescriptorPool(vk::DescriptorPoolCreateInfo({}, images.size(), size.size(), size.data()));
    
    // LAYOUT
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        {0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
    };
    layout = win->device.logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo({}, bindings.size(), bindings.data()));
    
    std::vector<vk::DescriptorSetLayout> layouts;
    
    for (const Material& mat : materials) {
        layouts.push_back(layout);
    }
    
    // SET
    std::vector<vk::DescriptorSet> sets = win->device.logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(pool, layouts.size(), layouts.data()));
    
    uint32_t i = 0;
    
    for (Material& mat : materials) {
        
        mat.tex = sets[i];
        i++;
        
        auto info = vk::DescriptorImageInfo(sampler, mat.image->view, vk::ImageLayout::eShaderReadOnlyOptimal);
        
        win->device.logical.updateDescriptorSets({vk::WriteDescriptorSet(mat.tex, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &info, 0, 0)}, {});
        
    }
    
}


vk::DescriptorSet ObjectSystem::getDescriptorSetFromImage(Image* image) {
    
    // SET
    vk::DescriptorSet set = win->device.logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(pool, 1, &layout))[0];
    
    auto info = vk::DescriptorImageInfo(sampler, image->view, vk::ImageLayout::eShaderReadOnlyOptimal);
    
    win->device.logical.updateDescriptorSets({vk::WriteDescriptorSet(set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &info, 0, 0)}, {});
    
    return set;
    
}


void ObjectSystem::transitionImages(vk::CommandBuffer buffer) {
    
    if(!win->device.isSingleQueue()) {
        
        for (const Image& image : images) {
            
            auto barrier2 = vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                                                    vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                                    win->device.t_i, win->device.g_i, image.image,
                                                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                                                    );
            
            buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                vk::DependencyFlagBits::eByRegion,
                {},
                {},
                {barrier2}
            );

        }
        
    }
    
}




QDebug operator<< (QDebug debug, const Vertex vertex) {
    debug << "{" << "\n";
    debug << "\t(" << vertex.pos.x << ", " << vertex.pos.y << ", " << vertex.pos.z << ")," << "\n";
    debug << "\t(" << vertex.normals.x << ", " << vertex.normals.y << ", " << vertex.normals.z << ")," << "\n";
    debug << "\t(" << vertex.texCoord.x << ", " << vertex.texCoord.y << ")" << "\n";
    debug << "}";
    return debug;
}

Object ObjectSystem::loadModel(QString objPath, QString mtlPath) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    bool ret;

    ResourceCopy::loadResource(mtlPath);
    ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, qUtf8Printable(ResourceCopy::loadResource(objPath)), qUtf8Printable(ResourceCopy::dirPath()), true);

    if(!warn.empty()) qWarning() << QString::fromStdString(warn);
    
    if (!err.empty()) {
        qCritical() << QString::fromStdString(err);
        QMessageBox error;
        error.setText(QString("Erreur lors du chargement de: '") + objPath + QString("' Détails: \n ") + QString::fromStdString(err));
        error.setIcon(QMessageBox::Critical);
        error.exec();
    }

    if (!ret) {
        exit(1);
    }
    
    objects.push_back({});
    Object& obj = objects[objects.size()-1];
    
    for (const tinyobj::material_t& mat : materials) {
        
        initMaterial(mat);
        
        obj.sub.push_back({&this->materials.back(), 0, 0});
    }
    
    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
    
    std::vector<std::vector<uint32_t>> perMaterial(materials.size());
    
    for (const auto& shape : shapes) {
        
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            
            // Loop over vertices in the face.
            for (size_t v = 0; v < 3; v++) {
                auto& index = shape.mesh.indices[3*f+v];
                Vertex vertex = {
                    {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    },
                    {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    },
                    {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    }
                };
                
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(obj.vertices.size());
                    obj.vertices.push_back(vertex);
                }
                
                perMaterial[shape.mesh.material_ids[f]].push_back(uniqueVertices[vertex]);
            }
        }
    }
    
    for (uint32_t i = 0; i<materials.size(); i++) {
        
        obj.sub[i].first_index = obj.indices.size();
        
        for(const uint32_t index : perMaterial[i]) {
            obj.indices.push_back(index);
            obj.sub[i].num_index++;
        }
        
    }
    
    obj.offset = vert_size;
    vert_size += obj.vertices.size() * sizeof(Vertex) + obj.indices.size() * sizeof(uint32_t);
    
    return obj;
}





void ObjectSystem::initMaterial(const tinyobj::material_t& mat) {
    
    Material material = {};
    
    material.color = glm::vec4(mat.ambient[0], mat.ambient[1], mat.ambient[2], 1.0);
    
    material.hasTexture = false;
    
    material.image = nullptr;
    
    if(mat.diffuse_texname.length() > 0) {
        
        material.hasTexture = true;
        material.image = initImage(QString::fromStdString(mat.diffuse_texname), STBI_rgb_alpha, defaultFormat);
       
    }
    
    materials.push_back(material);
    
}

Image* ObjectSystem::initImage(const QString path, int req_comp, vk::Format format) {
    
    Image image = {};
    
    auto buffer = foLoad(path);
    //stbi_uc* pixels = stbi_load(mat.diffuse_texname.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    int width, height, comp;
    image.pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*> (buffer.constData()), buffer.size(), &width, &height, &comp, req_comp);
    image.isOwned = true;
    image.width = width;
    image.height = height;
    image.comp = req_comp;
    image.format = format;
    image.size = image.width * image.height * req_comp;
    
    if (!image.pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    
    img_size += image.size;
    
    images.push_back(image);
    
    return &images.back();
    
}

Image * ObjectSystem::initImage(unsigned char* img, int width, int height, int req_comp, vk::Format format) {
    
    Image image = {};
    
    image.pixels = img;
    image.isOwned = false;
    image.width = width;
    image.height = height;
    image.comp = req_comp;
    image.format = format;
    image.size = image.width * image.height * req_comp;
    
    if (!image.pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    
    img_size += image.size;
    
    images.push_back(image);
    
    return &images.back();
    
}


ObjectSystem::~ObjectSystem() {
    win->device.logical.destroy(layout);
    win->device.logical.destroy(pool);
    for (const Image& image : images) {
        win->device.logical.destroy(image.view);
        win->device.logical.destroy(image.image);
    }
    win->device.logical.destroy(sampler);
    win->device.logical.free(imageMemory);
}


