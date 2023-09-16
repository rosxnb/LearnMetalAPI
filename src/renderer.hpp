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

    private:
        MTL::Device* p_device;
        MTL::CommandQueue* p_cmdQ;
        std::string* p_shaderSrc;
};
