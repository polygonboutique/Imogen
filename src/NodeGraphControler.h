// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
//
// Copyright(c) 2019 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once

#include "NodeGraph.h"
#include "EvaluationStages.h"
#include "EvaluationContext.h"
#include "GraphModel.h"

struct NodeGraphControler : public NodeGraphControlerBase
{
    NodeGraphControler();

    void Clear();

    void SetKeyboardMouse(const UIInput& input, bool bValidInput);

	// accessors
    virtual unsigned int GetNodeTexture(size_t index)
    {
        return mEditingContext.GetEvaluationTexture(index);
    }
    virtual int NodeIsProcesing(size_t nodeIndex) const
    {
        return mEditingContext.StageIsProcessing(nodeIndex);
    }
    virtual float NodeProgress(size_t nodeIndex) const
    {
        return mEditingContext.StageGetProgress(nodeIndex);
    }
    virtual bool NodeIsCubemap(size_t nodeIndex) const;
    virtual bool NodeIs2D(size_t nodeIndex) const;
    virtual bool NodeIsCompute(size_t nodeIndex) const;
    virtual ImVec2 GetEvaluationSize(size_t nodeIndex) const;


	// UI
    void NodeEdit();
    virtual void DrawNodeImage(ImDrawList* drawList, const ImRect& rc, const ImVec2 marge, const size_t nodeIndex);
    virtual bool RenderBackground();
    virtual void ContextMenu(ImVec2 offset, int nodeHovered);

    virtual void UpdateEvaluationList(const std::vector<size_t>& nodeOrderList)
    {
        mModel.SetEvaluationOrder(nodeOrderList);
    }

    EvaluationContext mEditingContext;
    
    int mBackgroundNode;
    

    EvaluationStage* Get(ASyncId id)
    {
        return GetByAsyncId(id, mModel.mEvaluationStages.mStages);
    }
    
    void ApplyDirtyList();
	GraphModel mModel;

protected:
    bool mbMouseDragging;
    bool mbUsingMouse;
    

    bool EditSingleParameter(unsigned int nodeIndex,
                             unsigned int parameterIndex,
                             void* paramBuffer,
                             const MetaParameter& param);
    void PinnedEdit();
    void EditNodeParameters();
    void HandlePin(size_t nodeIndex, size_t parameterIndex);
    void HandlePinIO(size_t nodeIndex, size_t slotIndex, bool forOutput);
    int ShowMultiplexed(const std::vector<size_t>& inputs, int currentMultiplexedOveride);
    
};
