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
#include "RasterPass.h"
#include "Core/API/RenderContext.h"

namespace Falcor
{
    RasterPass::SharedPtr RasterPass::create(const Program::Desc& desc, const Program::DefineList& defines)
    {
        try
        {
            return SharedPtr(new RasterPass(desc, defines));
        }
        catch (std::exception) { return nullptr; }
    }

    RasterPass::SharedPtr RasterPass::create(const std::string& filename, const std::string& vsEntry, const std::string& psEntry, const Program::DefineList& defines)
    {
        Program::Desc d;
        d.addShaderLibrary(filename).vsEntry(vsEntry).psEntry(psEntry);
        return create(d, defines);
    }

    RasterPass::RasterPass(const Program::Desc& progDesc, const Program::DefineList& programDefines) :
        BaseGraphicsPass(progDesc, programDefines) {}

    void RasterPass::drawIndexed(RenderContext* pContext, uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation)
    {
        pContext->drawIndexed(mpState.get(), mpVars.get(), indexCount, startIndexLocation, baseVertexLocation);
    }

    void RasterPass::draw(RenderContext* pContext, uint32_t vertexCount, uint32_t startVertexLocation)
    {
        pContext->draw(mpState.get(), mpVars.get(), vertexCount, startVertexLocation);
    }
}
