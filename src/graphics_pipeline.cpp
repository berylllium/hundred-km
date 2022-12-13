#include "graphics_pipeline.hpp"

#include <stdexcept>

#include "resource_handler.hpp"

namespace hkm
{

GraphicsPipeline::GraphicsPipeline()
{

}

GraphicsPipeline::GraphicsPipeline(
            const LogicalDevice* device,
            const std::string& vert_name,
            const std::string& frag_name,
            const GraphicsPipelineConfigInfo& config_info
            ) : device {device}, vert_name {vert_name}, frag_name {frag_name}, config_info {config_info}
{
    create_graphics_pipeline();
}

GraphicsPipeline::~GraphicsPipeline() {}

GraphicsPipeline& GraphicsPipeline::operator = (GraphicsPipeline&& source)
{
    if (device != nullptr) destroy();

    vert_name = source.vert_name;
    frag_name = source.frag_name;
    device = source.device;
    pipeline_layout = source.pipeline_layout;
    graphics_pipeline_handle = source.graphics_pipeline_handle;
    config_info = source.config_info;

    source.vert_name = "";
    source.frag_name = "";
    source.device = nullptr;
    source.pipeline_layout = nullptr;
    source.graphics_pipeline_handle = nullptr;

    return *this;
}

VkPipeline GraphicsPipeline::get_graphics_pipeline_handle() const
{
    return graphics_pipeline_handle;
}

void GraphicsPipeline::destroy()
{
    vkDestroyPipeline(device->get_logical_device(), graphics_pipeline_handle, nullptr);
    vkDestroyPipelineLayout(device->get_logical_device(), pipeline_layout, nullptr);
}

void GraphicsPipeline::create_graphics_pipeline()
{
    std::vector<char> vert_shader_code = resource_handler::read_shader_binary(vert_name);
    std::vector<char> frag_shader_code = resource_handler::read_shader_binary(frag_name);

    VkShaderModule vert_shader_module = create_shader_module(vert_shader_code);
    VkShaderModule frag_shader_module = create_shader_module(frag_shader_code);

    // Make shader stages
    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    // Dynamic state
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(float) * (3 * 2 + 2);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription position_description {};
    position_description.binding = 0;
    position_description.location = 0;
    position_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    position_description.offset = 0;

    VkVertexInputAttributeDescription normal_description {};
    normal_description.binding = 0;
    normal_description.location = 1;
    normal_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    normal_description.offset = sizeof(float) * 3;

    VkVertexInputAttributeDescription tex_coords_description {};
    tex_coords_description.binding = 0;
    tex_coords_description.location = 2;
    tex_coords_description.format = VK_FORMAT_R32G32_SFLOAT;
    tex_coords_description.offset = sizeof(float) * 3 * 2;

    vertex_input_attribute_descriptions.push_back(position_description);
    vertex_input_attribute_descriptions.push_back(normal_description);
    vertex_input_attribute_descriptions.push_back(tex_coords_description);

    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_description; // Optional
    vertex_input_info.vertexAttributeDescriptionCount = vertex_input_attribute_descriptions.size();
    vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data(); // Optional

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    // Viewports and scissors
    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer_info{};
    rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.0f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;
    rasterizer_info.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer_info.depthBiasClamp = 0.0f; // Optional
    rasterizer_info.depthBiasSlopeFactor = 0.0f; // Optional

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_info.minSampleShading = 1.0f; // Optional
    multisampling_info.pSampleMask = nullptr; // Optional
    multisampling_info.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling_info.alphaToOneEnable = VK_FALSE; // Optional

    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo color_blending_info{};
    color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_info.logicOpEnable = VK_FALSE;
    color_blending_info.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending_info.attachmentCount = 1;
    color_blending_info.pAttachments = &color_blend_attachment;
    color_blending_info.blendConstants[0] = 0.0f; // Optional
    color_blending_info.blendConstants[1] = 0.0f; // Optional
    color_blending_info.blendConstants[2] = 0.0f; // Optional
    color_blending_info.blendConstants[3] = 0.0f; // Optional

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0; // Optional
    pipeline_layout_info.pSetLayouts = nullptr; // Optional
    pipeline_layout_info.pushConstantRangeCount = 0; // Optional
    pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device->get_logical_device(), &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    } 

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Shader stages
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;

    // All fixed-function stages
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pDepthStencilState = nullptr; // Optional
    pipeline_info.pColorBlendState = &color_blending_info;
    pipeline_info.pDynamicState = &dynamic_state_info;

    pipeline_info.layout = pipeline_layout;

    pipeline_info.renderPass = config_info.render_pass;
    pipeline_info.subpass = 0;

    pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipeline_info.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(device->get_logical_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline_handle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // Cleanup after creation of pipeline
    vkDestroyShaderModule(device->get_logical_device(), frag_shader_module, nullptr);
    vkDestroyShaderModule(device->get_logical_device(), vert_shader_module, nullptr);
}

VkShaderModule GraphicsPipeline::create_shader_module(const std::vector<char>& code) const
{
    VkShaderModuleCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device->get_logical_device(), &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shader_module;
}

}
