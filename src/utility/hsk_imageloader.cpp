#include "hsk_imageloader.hpp"
#include "../hsk_env.hpp"

using namespace std::filesystem;

namespace hsk {

    ImageLoader::ImageInfo::~ImageInfo()
    {
        if(!!CustomLoaderInfo && !!CustomLoaderInfoDeleter)
        {
            CustomLoaderInfoDeleter(CustomLoaderInfo);
        }
    }

    bool ImageLoader::sGetImageInfo(std::string_view utf8path, ImageInfo& out)
    {
        new(&out) ImageInfo();

        out.Utf8Path = utf8path;

        path fspath = FromUtf8Path(utf8path);
        if(fspath.has_extension())
        {
            out.Extension = ToUtf8Path(fspath.extension());
        }
        if(fspath.has_filename())
        {
            out.Name = ToUtf8Path(fspath.filename());
        }

        if(!exists(fspath))
        {
            return false;
        }

        if(out.Extension == ".exr")
        {
            return sPopulateImageInfo_TinyExr(out);
        }
        else if(out.Extension == ".hdr")
        {
            return sPopulateImageInfo_StbHdr(out);
        }
        else
        {
            return sPopulateImageInfo_StbLdr(out);
        }
    }

    bool ImageLoader::Load()
    {
        if(!mInfo.Valid)
        {
            logger()->warn("Image Loader: Invalid Image Info!");
            return false;
        }

        if(mInfo.Extension == ".exr")
        {
            return Load_TinyExr();
        }
        else if(mInfo.Extension == ".hdr")
        {
            return Load_StbHdr();
        }
        else
        {
            return Load_StbLdr();
        }
    }

}  // namespace hsk