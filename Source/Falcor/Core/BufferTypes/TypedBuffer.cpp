/***************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "stdafx.h"
#include "TypedBuffer.h"

namespace Falcor
{
    TypedBufferBase::TypedBufferBase(uint32_t elementCount, ResourceFormat format, Resource::BindFlags bindFlags) :
        Buffer(elementCount * getFormatBytesPerBlock(format), bindFlags, Buffer::CpuAccess::None), mData(mSize, 0),
        mElementCount(elementCount), 
        mFormat(format)
    {
        apiInit(false);
    }

    bool TypedBufferBase::uploadToGPU()
    {
        if (mCpuDirty == false) return false;
        Buffer::setBlob(mData.data(), 0, mSize);
        mCpuDirty = false;
        return true;
    }

    void TypedBufferBase::readFromGpu()
    {
        if (mGpuDirty)
        {
            const uint8_t* pData = (uint8_t*)map(Buffer::MapType::Read);
            std::memcpy(mData.data(), pData, mData.size());
            unmap();
            mGpuDirty = false;
        }
    }

    bool TypedBufferBase::setBlob(const void* pSrc, size_t offset, size_t size)
    {
        if ((_LOG_ENABLED != 0) && (offset + size > mSize))
        {
            std::string Msg("Error when setting blob to buffer\"");
            Msg += mName + "\". Blob to large and will result in overflow. Ignoring call.";
            logError(Msg);
            return false;
        }
        std::memcpy(mData.data() + offset, pSrc, size);
        mCpuDirty = true;
        return true;
    }

    void* TypedBufferBase::getData()
    {
        readFromGpu();
        return mData.data();
    }
}
