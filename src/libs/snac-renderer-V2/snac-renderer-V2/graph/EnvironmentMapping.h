#pragma once

#include "../Model.h"
#include "../Pass.h"
#include "../Repositories.h"


namespace ad::renderer {


struct Loader;


struct Environment
{
    //Handle<const graphics::Texture> getEnvMap() const
    //{ return mTextureRepository.at(semantic::gEnvironmentTexture); }

    //Handle<const graphics::Texture> getFilteredRadiance() const
    //{ return mTextureRepository.at(semantic::gFilteredRadianceEnvironmentTexture); }

    //Handle<const graphics::Texture> getFilteredIrradiance() const
    //{ return mTextureRepository.at(semantic::gFilteredIrradianceEnvironmentTexture); }

    RepositoryTexture mTextureRepository;
};


Environment loadEnvironmentMap(const std::filesystem::path aDdsPath, Storage & aStorage);


struct SkyPassCache
{
    SkyPassCache(const Loader & aLoader, Storage & aStorage);

    void pass(const RepositoryUbo & aUboRepository, const Environment & aEnvironment);    

    PassCache mPassCache;
};


} // namespace ad::renderer