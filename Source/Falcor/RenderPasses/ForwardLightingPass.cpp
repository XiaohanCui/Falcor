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
#include "ForwardLightingPass.h"

namespace Falcor
{
    const char* ForwardLightingPass::kDesc =  "The pass computes the lighting results for the current scene. It will compute direct-illumination, indirect illumination from the light-probe and apply shadows (if a visibility map is provided).\n"
        "The pass can output the world-space normals and screen-space motion vectors, both are optional";

    namespace
    {
        const std::string kDepth = "depth";
        const std::string kColor = "color";
        const std::string kMotionVecs = "motionVecs";
        const std::string kNormals = "normals";
        const std::string kVisBuffer = "visibilityBuffer";

        const std::string kSampleCount = "sampleCount";
        const std::string kSuperSampling = "enableSuperSampling";
    }

    ForwardLightingPass::SharedPtr ForwardLightingPass::create(RenderContext* pRenderContext, const Dictionary& dict)
    {
        auto pThis = SharedPtr(new ForwardLightingPass());
        pThis->setColorFormat(ResourceFormat::RGBA32Float).setMotionVecFormat(ResourceFormat::RG16Float).setNormalMapFormat(ResourceFormat::RGBA8Unorm).setSampleCount(1).usePreGeneratedDepthBuffer(true);

        for (const auto& v : dict)
        {
            if (v.key() == kSampleCount) pThis->setSampleCount(v.val());
            else if (v.key() == kSuperSampling) pThis->setSuperSampling(v.val());
            else logWarning("Unknown field `" + v.key() + "` in a ForwardLightingPass dictionary");
        }

        return pThis;
    }

    Dictionary ForwardLightingPass::getScriptingDictionary() const 
    {
        Dictionary d;
        d[kSampleCount] = mSampleCount;
        d[kSuperSampling] = mEnableSuperSampling;
        return d;
    }

    ForwardLightingPass::ForwardLightingPass()
    {
        GraphicsProgram::SharedPtr pProgram = GraphicsProgram::createFromFile("RenderPasses/ForwardLightingPass.slang", "", "ps");
        mpState = GraphicsState::create();
        mpState->setProgram(pProgram);

        mpFbo = Fbo::create();
        
        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthWriteMask(false).setDepthFunc(DepthStencilState::Func::LessEqual);
        mpDsNoDepthWrite = DepthStencilState::create(dsDesc);        
    }

    RenderPassReflection ForwardLightingPass::reflect(const CompileData& compileData)
    {
        RenderPassReflection reflector;

        reflector.addInput(kVisBuffer, "Visibility buffer used for shadowing. Range is [0,1] where 0 means the pixel is fully-shadowed and 1 means the pixel is not shadowed at all").flags(RenderPassReflection::Field::Flags::Optional);
        reflector.addInputOutput(kColor, "Color texture").format(mColorFormat).texture2D(0, 0, mSampleCount);

        auto& depthField = mUsePreGenDepth ? reflector.addInputOutput(kDepth, "Pre-initialized depth-buffer") : reflector.addOutput(kDepth, "Depth buffer");
        depthField.bindFlags(Resource::BindFlags::DepthStencil).texture2D(0, 0, mSampleCount);
        
        if(mNormalMapFormat != ResourceFormat::Unknown)
        {
            reflector.addOutput(kNormals, "World-space normal, [0,1] range. Don't forget to transform it to [-1, 1] range").format(mNormalMapFormat).texture2D(0, 0, mSampleCount);
        }

        if (mMotionVecFormat != ResourceFormat::Unknown)
        {
            reflector.addOutput(kMotionVecs, "Screen-space motion vectors").format(mMotionVecFormat).texture2D(0, 0, mSampleCount);
        }

        return reflector;
    }

    void ForwardLightingPass::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
    {
        mpScene = pScene;
        mpState->getProgram()->addDefines(pScene->getSceneDefines());
        mpVars = GraphicsVars::create(mpState->getProgram()->getReflector());
        Sampler::Desc samplerDesc;
        samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
        setSampler(Sampler::create(samplerDesc));
    }

    void ForwardLightingPass::initDepth(const RenderData& renderData)
    {
        const auto& pTexture = renderData[kDepth]->asTexture();

        if (pTexture)
        {
            mpState->setDepthStencilState(mpDsNoDepthWrite);
            mpFbo->attachDepthStencilTarget(pTexture);
        }
        else
        {
            mpState->setDepthStencilState(nullptr);
            if(mpFbo->getDepthStencilTexture() == nullptr)
            {
                auto pDepth = Texture::create2D(mpFbo->getWidth(), mpFbo->getHeight(), ResourceFormat::D32Float, 1, 1, nullptr, Resource::BindFlags::DepthStencil);
                mpFbo->attachDepthStencilTarget(pDepth);
            }
        }
    }

    void ForwardLightingPass::initFbo(RenderContext* pContext, const RenderData& renderData)
    {
        mpFbo->attachColorTarget(renderData[kColor]->asTexture(), 0);
        mpFbo->attachColorTarget(renderData[kNormals]->asTexture(), 1);
        mpFbo->attachColorTarget(renderData[kMotionVecs]->asTexture(), 2);

        for(uint32_t i = 1 ; i < 3 ; i++)
        {
            const auto& pRtv = mpFbo->getRenderTargetView(i).get();
            if(pRtv->getResource() != nullptr) pContext->clearRtv(pRtv, vec4(0));
        }

        // TODO Matt (not really matt, just need to fix that since if depth is not bound the pass crashes
        if (mUsePreGenDepth == false) pContext->clearDsv(renderData[kDepth]->asTexture()->getDSV().get(), 1, 0);
    }

    void ForwardLightingPass::execute(RenderContext* pContext, const RenderData& renderData)
    {
        initDepth(renderData);
        initFbo(pContext, renderData);

        if (mpScene)
        {
            mpVars["PerFrameCB"]["gRenderTargetDim"] = vec2(mpFbo->getWidth(), mpFbo->getHeight());
            mpVars->setTexture(kVisBuffer, renderData[kVisBuffer]->asTexture());

            mpState->setFbo(mpFbo);
            mpScene->render(pContext, mpState.get(), mpVars.get());
        }
    }

    void ForwardLightingPass::renderUI(Gui::Widgets& widget)
    {
        static const Gui::DropdownList kSampleCountList =
        {
            { 1, "1" },
            { 2, "2" },
            { 4, "4" },
            { 8, "8" },
        };

        if (widget.dropdown("Sample Count", kSampleCountList, mSampleCount))              setSampleCount(mSampleCount);
        if (mSampleCount > 1 && widget.checkbox("Super Sampling", mEnableSuperSampling))  setSuperSampling(mEnableSuperSampling);
    }

    ForwardLightingPass& ForwardLightingPass::setColorFormat(ResourceFormat format)
    {
        mColorFormat = format;
        mPassChangedCB();
        return *this;
    }

    ForwardLightingPass& ForwardLightingPass::setNormalMapFormat(ResourceFormat format)
    {
        mNormalMapFormat = format;
        mPassChangedCB();
        return *this;
    }

    ForwardLightingPass& ForwardLightingPass::setMotionVecFormat(ResourceFormat format)
    {
        mMotionVecFormat = format;
        if (mMotionVecFormat != ResourceFormat::Unknown)
        {
            mpState->getProgram()->addDefine("_OUTPUT_MOTION_VECTORS");
        }
        else
        {
            mpState->getProgram()->removeDefine("_OUTPUT_MOTION_VECTORS");
        }
        mPassChangedCB();
        return *this;
    }

    ForwardLightingPass& ForwardLightingPass::setSampleCount(uint32_t samples)
    {
        mSampleCount = samples;
        mPassChangedCB();
        return *this;
    }

    ForwardLightingPass& ForwardLightingPass::setSuperSampling(bool enable)
    {
        mEnableSuperSampling = enable;
        if (mEnableSuperSampling)
        {
            mpState->getProgram()->addDefine("INTERPOLATION_MODE", "sample");
        }
        else
        {
            mpState->getProgram()->removeDefine("INTERPOLATION_MODE");
        }

        return *this;
    }

    ForwardLightingPass& ForwardLightingPass::usePreGeneratedDepthBuffer(bool enable)
    {
        mUsePreGenDepth = enable;
        mPassChangedCB();
        mpState->setDepthStencilState(mUsePreGenDepth ? mpDsNoDepthWrite : nullptr);

        return *this;
    }

    ForwardLightingPass& ForwardLightingPass::setSampler(const Sampler::SharedPtr& pSampler)
    {
        mpVars->setSampler("gSampler", pSampler);
        return *this;
    }
}
