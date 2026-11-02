#pragma once
#include <bfc/platform/types.h>
#include <bfc/platform/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NSUTIL_EXPORTS
#define NSUTIL_EXPORT __declspec(dllexport)
#else
#define NSUTIL_EXPORT __declspec(dllimport)
#endif

NSUTIL_EXPORT int nsutil_image_FillRectAlpha_RGB32(RGB32 *destination_image, size_t destination_stride /* bytes! */, uint32_t width, uint32_t height, RGB32 color, int alpha);
#ifdef __cplusplus
}
#endif