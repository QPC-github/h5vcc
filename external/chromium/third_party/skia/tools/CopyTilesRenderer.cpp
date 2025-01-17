/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "picture_utils.h"
#include "CopyTilesRenderer.h"
#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkImageEncoder.h"
#include "SkPicture.h"
#include "SkPixelRef.h"
#include "SkRect.h"
#include "SkString.h"

namespace sk_tools {
    CopyTilesRenderer::CopyTilesRenderer(int x, int y)
    : fXTilesPerLargeTile(x)
    , fYTilesPerLargeTile(y) {
    }
    void CopyTilesRenderer::init(SkPicture* pict) {
        SkASSERT(pict != NULL);
        // Only work with absolute widths (as opposed to percentages).
        SkASSERT(this->getTileWidth() != 0 && this->getTileHeight() != 0);
        fPicture = pict;
        fPicture->ref();
        this->buildBBoxHierarchy();
        // In order to avoid allocating a large canvas (particularly important for GPU), create one
        // canvas that is a multiple of the tile size, and draw portions of the picture.
        fLargeTileWidth = fXTilesPerLargeTile * this->getTileWidth();
        fLargeTileHeight = fYTilesPerLargeTile * this->getTileHeight();
        fCanvas.reset(this->INHERITED::setupCanvas(fLargeTileWidth, fLargeTileHeight));
    }

    bool CopyTilesRenderer::render(const SkString* path) {
        int i = 0;
        bool success = true;
        SkBitmap dst;
        for (int x = 0; x < this->getViewWidth(); x += fLargeTileWidth) {
            for (int y = 0; y < this->getViewHeight(); y += fLargeTileHeight) {
                SkAutoCanvasRestore autoRestore(fCanvas, true);
                fCanvas->translate(SkIntToScalar(-x), SkIntToScalar(-y));
                // Draw the picture
                fCanvas->drawPicture(*fPicture);
                // Now extract the picture into tiles
                const SkBitmap& baseBitmap = fCanvas->getDevice()->accessBitmap(false);
                SkIRect subset;
                for (int tileY = 0; tileY < fLargeTileHeight; tileY += this->getTileHeight()) {
                    for (int tileX = 0; tileX < fLargeTileWidth; tileX += this->getTileWidth()) {
                        subset.set(tileX, tileY, tileX + this->getTileWidth(),
                                   tileY + this->getTileHeight());
                        SkDEBUGCODE(bool extracted =)
                        baseBitmap.extractSubset(&dst, subset);
                        SkASSERT(extracted);
                        if (path != NULL) {
                            // Similar to writeAppendNumber in PictureRenderer.cpp, but just encodes
                            // a bitmap directly.
                            SkString pathWithNumber(*path);
                            pathWithNumber.appendf("%i.png", i++);
                            SkBitmap copy;
#if SK_SUPPORT_GPU
                            if (isUsingGpuDevice()) {
                                dst.pixelRef()->readPixels(&copy, &subset);
                            } else {
#endif
                                dst.copyTo(&copy, dst.config());
#if SK_SUPPORT_GPU
                            }
#endif
                            success &= SkImageEncoder::EncodeFile(pathWithNumber.c_str(), copy,
                                                                  SkImageEncoder::kPNG_Type, 100);
                        }
                    }
                }
            }
        }
        return success;
    }

    SkString CopyTilesRenderer::getConfigNameInternal() {
        return SkString("copy_tiles");
    }
}
