/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <vector>
#include <cstdint>

/**
 * Interface for video receivers that can decode video streams.
 * Implementations include GStreamer and Android NDK decoders.
 */
class IVideoReceiver {
public:
    // Frame data structure to encapsulate frame details
    struct FrameData {
        const std::uint8_t* pixels = nullptr;  // Pointer to raw pixel data
        int width = 0;                         // Frame width in pixels
        int height = 0;                        // Frame height in pixels
        int stride = 0;                        // Bytes per row (may include padding)
        uint64_t timestamp = 0;                // Timestamp of the frame in microseconds

        // Internal buffer to manage pixel data
        // TODO: We shouldn't do copies here, but rather use a shared_ptr or similar
        std::vector<std::uint8_t> pixelBuffer;
    };

    // Virtual destructor ensures proper cleanup for derived classes
    virtual ~IVideoReceiver() = default;

    // Lifecycle methods
    virtual bool init() = 0;
    virtual void deinit() = 0;

    // Process incoming encoded video data (takes ownership of the provided vector)
    virtual void consumeBitStream(std::vector<std::uint8_t>* video) = 0;

    // Get the latest decoded frame data
    // Returns true if a valid frame is available, false otherwise
    // The implementation must ensure the frame data remains valid until freeFrame is called
    virtual bool frameGet(FrameData& frameData) = 0;

    // Free resources associated with specified frameData
    virtual void frameRelease(FrameData& frameData) = 0;

    // Get the percentage of buffer filled (for status reporting)
    virtual std::uint16_t getBufferFilled() = 0;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/