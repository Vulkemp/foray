#include "hsk_modelconverter.hpp"
#include "../base/hsk_vkcontext.hpp"

namespace hsk {
    std::unique_ptr<NScene> ModelConverter::LoadGltfModel(const VkContext* context, std::string utf8Path)
    {
        tinygltf::TinyGLTF gltfContext;
        std::string        error;
        std::string        warning;

        bool   binary = false;
        size_t extpos = utf8Path.rfind('.', utf8Path.length());
        if(extpos != std::string::npos)
        {
            binary = (utf8Path.substr(extpos + 1, utf8Path.length() - extpos) == "glb");
        }

        bool fileLoaded =
            binary ? gltfContext.LoadBinaryFromFile(&mGltfModel, &error, &warning, utf8Path.c_str()) : gltfContext.LoadASCIIFromFile(&mGltfModel, &error, &warning, utf8Path.c_str());

        if(warning.size())
        {
            logger()->warn("tinygltf warning loading file \"{}\": \"{}\"", utf8Path, warning);
        }
        if(!fileLoaded)
        {
            if(!error.size()) {
                error = "Unknown error";
            }
            logger()->error("tinygltf error loading file \"{}\": \"{}\"", utf8Path, error);
            Exception::Throw("Failed to load file");
        }

        // std::unique_ptr<NScene> uniqueptr = std::make_unique(context);

        // NScene& scene = *uniqueptr.get();

        // scene.GetRootNodes().resize(mGltfModel.nodes.size());

        // return uniqueptr;
        return nullptr;
    }
}  // namespace hsk