// openTIT - Agl texture search paths and loading textures from loose files.
#include "agl/AglTexture.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"

#include <cstring>

namespace agl {

namespace {

constexpr int kMaxTexturePaths = 16;
constexpr size_t kDirectorySize = 64;

char* g_texturePaths[kMaxTexturePaths] = {};   // 0x010DDAFC
uint32_t g_texturePathCount = 0;               // 0x010DDAF8
char g_textureDirectory[kDirectorySize] = {};  // 0x010DDB40
uint32_t g_texturePathFlag = 0;                // 0x010DDB3C

} // namespace

// @func 0x0051D198 TexturePathChunkHandler
int TexturePathChunkHandler(ape::iff::IffReader* reader, void* /*arg*/)
{
    char path[256];
    const bool ok = ape::iff::ParseIncludedFile(reader, path);
    if (ok && g_texturePathCount < kMaxTexturePaths)
        g_texturePaths[g_texturePathCount++] = ape::StrDup(path);
    return ok ? 1 : 0;
}

// @func 0x0051E7F0 SetTextureDirectory
void SetTextureDirectory(const char* directory)
{
    ape::StrCopyBounded(g_textureDirectory, directory, sizeof(g_textureDirectory));
}

const char* GetTextureDirectory()
{
    return g_textureDirectory;
}

// @func 0x0051E824 ClearTexturePaths
void ClearTexturePaths()
{
    g_texturePathFlag = 0;
    if (g_texturePathCount == 0)
        return;
    while (g_texturePathCount != 0) {
        --g_texturePathCount;
        if (g_texturePaths[g_texturePathCount]) {
            ape::MemFree(g_texturePaths[g_texturePathCount]);
            g_texturePaths[g_texturePathCount] = nullptr;
        }
    }
    ape::StrCopyBounded(g_textureDirectory, "", sizeof(g_textureDirectory));
}

// @func 0x0051E68C TextureLoadFile
AglTexture* TextureLoadFile(const char* path, bool useDefaultOnFailure)
{
    AglTexture* texture = nullptr;
    uint32_t size = 0;
    void* data = ape::file::LoadFile(path, &size);
    if (data) {
        const char* extension = ape::file::GetFileExtension(path);
        texture = AglTexture::Create();
        ape::StrNCopy(texture->name, path, sizeof(texture->name));
        texture->name[sizeof(texture->name) - 1] = '\0';
        texture->nameHash = ape::HashNameNoCase(texture->name);
        if (!texture->Load(data, size, extension)) {
            texture->Release();
            texture = nullptr;
        }
        ape::file::FreeFileData(data);
        if (texture)
            return texture;
    }
    if (useDefaultOnFailure) {
        texture = GetDefaultTexture(kAglTex_White);
        if (texture)
            texture->AddRef();
    }
    return texture;
}

} // namespace agl
