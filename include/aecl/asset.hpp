#pragma once

#include <umbf/umbf.hpp>
#include <umbf/version.h>

namespace aecl
{
    struct Asset
    {
        umbf::Header header;
        umbf::BlockList blocks;
    };

    inline void create_asset_structure(Asset &asset, u32 type_sign)
    {
        asset.header.vendor_sign = UMBF_VENDOR_ID;
        asset.header.vendor_version = UMBF_VERSION;
        asset.header.spec_version = UMBF_VERSION;
        asset.header.type_sign = type_sign;
    }

} // namespace aecl
