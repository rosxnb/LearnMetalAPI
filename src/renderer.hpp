#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();

        void draw( MTK::View* pView );
        void build_shaders();
        void build_buffers();
        void build_textures();

    private:
        MTL::Device* p_device;
        MTL::CommandQueue* p_cmdQ;

        MTL::RenderPipelineState* p_pipelineState;
        MTL::Buffer* p_vertexPositions;
        MTL::Buffer* p_vertexColors;

        MTL::Buffer* p_argBuffer;
        MTL::Library* p_shaderLibrary;

        MTL::Texture* p_texture;
        MTL::Buffer* p_texCoords;

        std::string m_shaderSrc;
        const char* m_texPath = "resources/large-png.png";
        int m_imgWidth, m_imgHeight, m_imgChannels;
        unsigned char* m_imgData = nullptr;

        MTL::Buffer* p_proj;
};

