#include "EditorPch.h"
#include "AnimationTool.h"
#include "ModelLoader.h"
#include "GltfLoader.h"
#include "AnimationActor.h"
#include "AnimNotifyTool.h"
#include "AnimMachine.h"
#include "Effect_Controller.h"



#pragma region 기본함수들
CAnimationTool::CAnimationTool(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance { CGameInstance::GetInstance()}
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
    Safe_AddRef(m_pGameInstance);
}

HRESULT CAnimationTool::Initialize(LEVEL eLevel)
{
    m_eCurLevel = eLevel;

    m_pLoader = CModelLoader::Create();
    m_pGltfLoader = CGltfLoader::Create();

    m_pAnimNotifyTool = CAnimNotifyTool::Create(m_pDevice, m_pContext, m_eCurLevel);
    
#ifdef _DEBUG
	m_pAnimMachineCom = CAnimMachine::Create(m_pDevice, m_pContext);
#endif // _DEBUG

    

    return S_OK;
}

void CAnimationTool::Render()
{
    Render_Editor();
    
    if (m_IsVisibleNotify)
    {
        _string strModelDirPath = m_ModelDirPaths[m_wSelected_PrototypeModelTag];
        ASSERT_CRASH(m_pAnimNotifyTool);
        m_pAnimNotifyTool->Process_Notify(m_AnimationActors[m_wSelected_AnimActorTag], m_Selected_AnimationTag, strModelDirPath, m_fDuration);
        m_pAnimNotifyTool->Render();
    }

    // EditState 설정된 걸로.
    RenderUI_EditState();
        
}

void CAnimationTool::Set_EffectContorller(CEffect_Controller* pEffectController)
{
    ASSERT_CRASH(pEffectController);
    m_pEffectController = pEffectController;
}

void CAnimationTool::Export_AnimationData(CEffect_Controller* pEffectController)
{
    if (nullptr == m_pEffectController)
    {
        MSG_BOX("Nullptr EffectController");
        return;
    }
        

    EFFECTACTOR_DESC Desc{};
    // 현재 선택된 객체의 포인터와 필요한 값. 전달.
    Desc.pAnimActor = m_AnimationActors[m_wSelected_AnimActorTag];
#ifdef _DEBUG
    Desc.fDuration = Desc.pAnimActor->Get_Duration(m_Selected_AnimationTag);
#endif
    m_pEffectController->Import_AnimationData(Desc);
}

void CAnimationTool::Render_Editor()
{
    Render_DebugWindow();

    ImGui::Begin(u8"Editor", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar
        | ImGuiWindowFlags_AlwaysVerticalScrollbar);

    
    ImGui::Text("Settings");
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 WindowColor = style.Colors[ImGuiCol_WindowBg];
    ImGui::SliderFloat("Editor Opacity", &m_fEditorAlpha, 0.0f, 1.0f);
    const ImVec4 NewColor = ImVec4(WindowColor.x, WindowColor.y, WindowColor.z, m_fEditorAlpha);
    style.Colors[ImGuiCol_WindowBg] = NewColor;
    ImGui::NewLine();

    Render_Menu();

    ImGui::End();
}

void CAnimationTool::Render_DebugWindow()
{
    ImGuiIO& io = ImGui::GetIO();

    ImVec2 windowPos = ImVec2(300.f, g_iWinSizeY - 300.f);
    ImVec2 windowSize = ImVec2(300.f, 300.f);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoCollapse);

    _float4 camPos = {};
    camPos = *m_pGameInstance->Get_CamPos();
    ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);


    const char* typeNames[] = { "CONVERT_GLTF", "CONVERT_FBX", "LOAD_DAT", "CREATE_ACTOR","EDIT_ANIMATION", "SAVE_STATE", "NONE"};
    ImGui::Text("MODE : %s", typeNames[ENUM_CLASS(m_eMode)]);

    switch (m_eMode)
    {
    case MODE::CREATE_ACTOR:
    {
        if (!m_Selected_PrototypeModelTag.empty())
            ImGui::Text("Select Model : %s", m_Selected_PrototypeModelTag.c_str());
        else
            ImGui::Text("Select Model : None");
    }
        break;
    case MODE::EDIT_ANIMATION:
    {
        if (!m_Selected_AnimActorTag.empty())
            ImGui::Text("Select Actor : %s", m_Selected_AnimActorTag.c_str());
        else
            ImGui::Text("Select Actor : None");

        if(!m_Selected_AnimationTag.empty())
            ImGui::Text("Select Animation : %s", m_Selected_AnimationTag.c_str());
        else
            ImGui::Text("Select Model : None");
    }
        break;
    default:
        break;
    }

    ImGui::End();
}


void CAnimationTool::Render_Menu()
{
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("TabBar", tab_bar_flags))
    {

        if (ImGui::BeginTabItem("ConvertGLTF"))
        {
            m_pGltfLoader->Update();
            ImGui::EndTabItem();

            m_eMode = MODE::CONVERT_GLTF_TO_DAT;
        }

		if (ImGui::BeginTabItem("ConvertFBX"))
		{
			m_pLoader->Update();
			ImGui::EndTabItem();

			m_eMode = MODE::CONVERT_FBX_TO_DAT;
		}

        if (ImGui::BeginTabItem("LoadDAT"))
        {
            LoadDat();
            ImGui::EndTabItem();

            m_eMode = MODE::LOAD_DAT;
        }

        if (ImGui::BeginTabItem("CreateActor"))
        {
            RenderUI_CreateActor();
            ImGui::EndTabItem();

            m_eMode = MODE::CREATE_ACTOR;
        }

		if (ImGui::BeginTabItem("EditActor"))
		{
			RenderUI_EditActor();
			ImGui::EndTabItem();

			m_eMode = MODE::CREATE_ACTOR;
		}

        if (ImGui::BeginTabItem("EditAnimation"))
        {
            RenderUI_EditAnimation();
            ImGui::EndTabItem();

            m_eMode = MODE::EDIT_ANIMATION;
        }

        ImGui::EndTabBar();
    }

}

void CAnimationTool::RenderUI_ConvertFbx()
{
    m_pLoader->Update();
}

void CAnimationTool::RenderUI_CreateActor()
{
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Prototype", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Model"))
        {
            RenderUI_ModelPrototype();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
}

void CAnimationTool::RenderUI_EditActor()
{
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("Prototype", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Model Edit"))
		{
			RenderUI_EditModel();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void CAnimationTool::RenderUI_EditAnimation()
{
    _wstring objTag = {};
    _wstring modelTag = {};

    ImGui::BeginChild("left pane", ImVec2(500, 0), true);

    static int iSelectedIndex = -1;
    _uint id = 0;

	

    // 1. 액터 선택.
    for (auto& actorName : m_ActorNames)
    {
		_bool isSelected = (id == iSelectedIndex);

        if (ImGui::Selectable(actorName.c_str(), isSelected))
        {
            iSelectedIndex = id;
            m_Selected_AnimActorTag = actorName;
            m_wSelected_AnimActorTag = StringToWString(actorName);

			if (m_wSelected_AnimActorTag != m_wPrevSelected_AnimActorTag)
			{
				m_wPrevSelected_AnimActorTag = m_wSelected_AnimActorTag;
				m_PrevSelected_AnimActorTag = m_Selected_AnimActorTag;
				m_Selected_AnimationTag.clear(); // 현재 선택중인 Animation 비우기
				m_IsVisibleNotify = false; // Notifiy 끄기?
			}
			// 선택할때마다 NotifyVisible 끄기?
        }
		id++;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 3. 액터의 Animation List 체크.
    if (iSelectedIndex >= 0 && iSelectedIndex < m_ActorNames.size())
        RenderUI_AnimationList();


}

void CAnimationTool::RenderUI_FromState()
{
    ImGui::BeginGroup();
    {
        ImGui::Text("FROM");
        ImGui::BeginChild("FromState", ImVec2(400, 0), true);
        {
            static int iSelectedIndex = -1;
            _uint id = 0;
#ifdef _DEBUG
            for (auto& stateName : m_AnimationActors[m_wSelected_AnimActorTag]->Get_AnimationNames())
            {
                if (ImGui::Selectable(stateName.c_str(), id == iSelectedIndex))
                {
                    iSelectedIndex = id;
                    m_SelectedFromStateTag = stateName;
                    //m_iTransitionInfoSelectedIndex = -1;
                    if(false == m_pAnimMachineCom->isEmpty())
                        m_pAnimMachineCom->Get_StateData(m_SelectedFromStateTag, m_isRootMotion, m_isRootMotionRotate, m_isRootMotionTranslate, m_isLoopCheck,
                                                        m_fRootMotionRate, m_fTransitTrackPos, m_fAnimationSpeed);
                }
                id++;
            }
#endif // _DEBUG


        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();
   
}

void CAnimationTool::RenderUI_ToState()
{
    ImGui::BeginGroup();
    {
        ImGui::Text("TO");
        
        ImGui::BeginChild("ToState", ImVec2(400, 0), true);
        {
            static int iSelectedIndex = -1;
            _uint id = 0;

#ifdef _DEBUG
            for (auto& stateName : m_AnimationActors[m_wSelected_AnimActorTag]->Get_AnimationNames())
            {
                if (ImGui::Selectable(stateName.c_str(), id == iSelectedIndex))
                {
                    iSelectedIndex = id;
                    m_SelectedToStateTag = stateName;
                }
                id++;
            }
#endif // _DEBUG


        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();
    
}

void CAnimationTool::RenderUI_Transitions()
{
    ImGui::BeginGroup();
    {
        ImGui::Text("Transitions");
        ImVec2 vParentSize = ImGui::GetContentRegionAvail();
        _float fChildHeight = (vParentSize.y - ImGui::GetStyle().ItemSpacing.y) * 0.6f;
        ImGui::BeginChild("Transition", ImVec2(400, fChildHeight), true);
        {
            static int iSelectedIndex = -1;
            _uint id = 0;

            //for (auto& stateName : m_StateTransitions)
            //{
            //    if (ImGui::Selectable(stateName.c_str(), id == iSelectedIndex))
            //    {
            //        iSelectedIndex = id;
            //        m_SelectedToStateTag = stateName;
            //    }
            //    id++;
            //}
            
            for(auto& tTransition : m_TransitionDatas)
            {
                if(0 == m_SelectedFromStateTag.compare(tTransition.strFrom))
                {
                    if(ImGui::Selectable(tTransition.strTo.c_str(), id == iSelectedIndex))
                    {
                        iSelectedIndex = id;
                        m_iTransitionInfoSelectedIndex = iSelectedIndex;
                    }
                }
                id++;
            }
        }
        ImGui::EndChild();

        ImGui::BeginChild("AnyState", ImVec2(400, 0), true);
        {
            ImGui::Text("AnyState");
            ImGui::Separator();
            _int iSelectedIndex = m_iAnyStateTransitionSelectedIndex;
            _uint id = 0;
            for(auto& tTransition : m_AnyStateTransitions)
            {
                if(ImGui::Selectable(tTransition.strTo.c_str(), id == iSelectedIndex))
                {
                    m_iAnyStateTransitionSelectedIndex = id;
                }
                
                id++;
            }
        }
        ImGui::EndChild();
        //if(false == m_TransitionDatas.empty() && -1 != m_iTransitionInfoSelectedIndex)
        //    RenderUI_TransitionInfo();
    }
    ImGui::EndGroup();


}

void CAnimationTool::RenderUI_OptionState()
{
    ImGui::BeginGroup();
    {
        ImGui::Text("Option");
        ImGui::BeginChild("Option", ImVec2(400, 0), true);
        {
            // 1. TrackPosition 설정

            static float fTrackPosition = { 0.f };
            ImGui::InputFloat("Transit Target Position", &fTrackPosition, 0.f, 0.f, "%.3f");
            ImGui::InputFloat("Transit Enable Position", &m_iTransitionEnablePos, 0.f, 0.f, "%.3f");

            // 2. Add Translation 설정.
           

            // 2. 함수 설정.
            static char textBuffer[256] = "";
            ImGui::InputText("Function Name", textBuffer, sizeof(textBuffer));

            //3. 우선 순위
            ImGui::InputScalar("Priority", ImGuiDataType_U32, &m_iTransitionPriority);
            //4. 조건이 되어질 상태
            _uint iStateFlag = m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1));
            ImGui::Text("Target State: %u", iStateFlag);
            ImGui::InputScalar("State Index", ImGuiDataType_U32, &m_iTransitionTargetState);
            ImGui::RadioButton("_Single", reinterpret_cast<_int*>(&m_eTransitConditionType), 0);
            ImGui::SameLine();
            ImGui::RadioButton("_Multi", reinterpret_cast<_int*>(&m_eTransitConditionType), 1);
            ImGui::SameLine();
            ImGui::RadioButton("_Not", reinterpret_cast<_int*>(&m_eTransitConditionType), 2);
            //_string strTransition = {};
            if (ImGui::Button("Add Transition"))
            {
                if (fTrackPosition > m_fDuration)
                    MSG_BOX("Duration Over");

                //stringstream ss;
                //ss << m_SelectedFromStateTag << "," << m_SelectedFromStateTag 
                //    << "," << to_string(fTrackPosition);
                //strTransition = ss.str();
                //m_StateTransitions.emplace_back(strTransition);


                TRANSITION_DATA T_Data{
                m_SelectedFromStateTag,
                m_SelectedToStateTag,
                m_iTransitionPriority,
                m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1)),
                fTrackPosition,
                m_iTransitionEnablePos
                };
                //pair<_string, TRANSITION_DATA> Pair = {m_SelectedFromStateTag, T_Data};
                m_TransitionDatas.push_back(T_Data);
            }

            ImGui::SameLine();
            //if (ImGui::Button("Delete Back"))
            //{
            //    m_StateTransitions.pop_back();
            //}
            //
            //ImGui::SameLine();
            //if (ImGui::Button("Delete Front"))
            //{
            //    m_StateTransitions.pop_front();
            //}
            if(ImGui::Button("Delete Selection"))
            {
                m_TransitionDatas.erase(m_TransitionDatas.begin() + m_iTransitionInfoSelectedIndex);
                m_iTransitionInfoSelectedIndex = -1;
            }

            if (ImGui::Button("Clear Transition"))
            {
                m_StateTransitions.clear();
                m_TransitionDatas.clear();
            }

            if(ImGui::Button("Add to AnyState"))
            {
                TRANSITION_DATA T_Data{
                "",
                m_SelectedToStateTag,
                m_iTransitionPriority,
                m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1)),
                fTrackPosition,
                m_iTransitionEnablePos
                };
                m_AnyStateTransitions.push_back(T_Data);
            }
            ImGui::SameLine();
            if(ImGui::Button("Delete from AnyState"))
            {
                m_AnyStateTransitions.erase(m_AnyStateTransitions.begin() + m_iAnyStateTransitionSelectedIndex);
                m_iAnyStateTransitionSelectedIndex = -1;
            }

            
#ifdef _DEBUG
            if(m_pAnimMachineCom->isEmpty() && ImGui::Button("AnimState Set"))
            {
                m_pAnimMachineCom->Create_AnimStates(m_AnimationActors[m_wSelected_AnimActorTag]->Get_AnimationNames());
            }
            if(m_pAnimMachineCom->Render_CurrentStateGUI(m_SelectedFromStateTag))
            {
                ImGui::Checkbox("isRootMotion", &m_isRootMotion);
                ImGui::SameLine();
                ImGui::Checkbox("isRootMotionRotate", &m_isRootMotionRotate);
                ImGui::Checkbox("isRootMotionTranslate", &m_isRootMotionTranslate);
                ImGui::SameLine();
                ImGui::Checkbox("isLoop", &m_isLoopCheck);
                ImGui::InputFloat("RootMotionRate", &m_fRootMotionRate);
                ImGui::InputFloat("TransitTrackPos", &m_fTransitTrackPos);
                ImGui::InputFloat("AnimationSpeed", &m_fAnimationSpeed);
            }

            if(ImGui::Button("Reset Data"))
            {
                m_pAnimMachineCom->Reset_StateData(m_SelectedFromStateTag, m_isRootMotion, m_isRootMotionRotate, m_isRootMotionTranslate, m_isLoopCheck, m_fRootMotionRate, m_fTransitTrackPos, m_fAnimationSpeed);
            }

            Export_StateTransition_To_CSV();
            if(ImGui::Button("Load Json"))
            {
                m_isShowImport_ST_Dialog = true;
            }
            if(m_isShowImport_ST_Dialog)
                Import_StateTransition_From_Json();
#endif

        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();
    
}

void CAnimationTool::RenderUI_TransitionInfo()
{
    ImGui::Begin("Transition Info");
    ImVec2 vParentSize = ImGui::GetContentRegionAvail();
    _float fChildWidth = (vParentSize.x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    if(false == m_TransitionDatas.empty() && -1 != m_iTransitionInfoSelectedIndex)
    {
        ImGui::BeginChild("Current Transition", ImVec2(fChildWidth, 0), true);
        {
            ImGui::Text("Transition Count : %u", m_TransitionDatas.size());
            ImGui::Separator();
            auto& SelectData = m_TransitionDatas[m_iTransitionInfoSelectedIndex];

            ImGui::Text("From: %s", SelectData.strFrom.c_str());
            ImGui::Text("To: %s", SelectData.strTo.c_str());
            ImGui::InputScalar("Priority", ImGuiDataType_U32, &SelectData.iPriority);
            ImGui::Text("TargetState: %u", SelectData.iTargetState);
            _uint iTemp = SelectData.iTargetState;
            _int iCount = {};
            vector<_int> Check;
            while(iTemp > 0)
            {
                Check.insert(Check.begin(), iTemp % 2);
                iTemp /= 2;
            }

            for(size_t i = 0; i < Check.size(); i++)
            {
                if(Check[i])
                {
                    ImGui::SameLine();
                    ImGui::Text("%u", (1 << Check.size() - i - 1));
                }
            }
            ImGui::RadioButton("Single", reinterpret_cast<_int*>(&SelectData.eType), 0);
            ImGui::SameLine();
            ImGui::RadioButton("Multi", reinterpret_cast<_int*>(&SelectData.eType), 1);
            ImGui::SameLine();
            ImGui::RadioButton("Not", reinterpret_cast<_int*>(&SelectData.eType), 2);

            ImGui::InputFloat("Transit Target Position", &SelectData.fTargetTrackPos);
            ImGui::InputFloat("Transit Enable Position", &SelectData.fTransitEnablePos);
            _uint iStateFlag = m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1));
            ImGui::Text("%u", iStateFlag);
            ImGui::InputScalar("Additional State", ImGuiDataType_U32, &m_iTransitionTargetState);
            if(ImGui::Button("Add_State"))
            {
                SelectData.iTargetState |= (m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1)));
            }
            ImGui::SameLine();
            if(ImGui::Button("Remove_State"))
            {
                SelectData.iTargetState &= ~(m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1)));
            }
            ImGui::SameLine();
            if(ImGui::Button("Clear_State"))
            {
                SelectData.iTargetState = 0;
            }
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();
    if(false == m_AnyStateTransitions.empty() && -1 != m_iAnyStateTransitionSelectedIndex)
    {
        ImGui::BeginChild("AnyState Transition", ImVec2(0, 0), true);
        {
            ImGui::Text("AnyState Transition Count : %u", m_AnyStateTransitions.size());
            ImGui::Separator();
            auto& SelectData = m_AnyStateTransitions[m_iAnyStateTransitionSelectedIndex];

            ImGui::Text("From: %s", SelectData.strFrom.c_str());
            ImGui::Text("To: %s", SelectData.strTo.c_str());
            ImGui::InputScalar("Priority", ImGuiDataType_U32, &SelectData.iPriority);
            ImGui::Text("TargetState: %u", SelectData.iTargetState);
            _uint iTemp = SelectData.iTargetState;
            _int iCount = {};
            vector<_int> Check;
            while(iTemp > 0)
            {
                Check.insert(Check.begin(), iTemp % 2);
                iTemp /= 2;
            }

            for(size_t i = 0; i < Check.size(); i++)
            {
                if(Check[i])
                {
                    ImGui::SameLine();
                    ImGui::Text("%u", (1 << Check.size() - i - 1));
                }
            }
            ImGui::RadioButton(".Single", reinterpret_cast<_int*>(&SelectData.eType), 0);
            ImGui::SameLine();
            ImGui::RadioButton(".Multi", reinterpret_cast<_int*>(&SelectData.eType), 1);
            ImGui::SameLine();
            ImGui::RadioButton(".Not", reinterpret_cast<_int*>(&SelectData.eType), 2);
            ImGui::InputFloat("Transit Target Position", &SelectData.fTargetTrackPos);
            ImGui::InputFloat("Transit Enable Position", &SelectData.fTransitEnablePos);
            _uint iStateFlag = m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1));
            ImGui::Text("%u", iStateFlag);
            ImGui::InputScalar("Additional State", ImGuiDataType_U32, &m_iTransitionTargetState);
            if(ImGui::Button("Add_State"))
            {
                SelectData.iTargetState |= (m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1)));
            }
            ImGui::SameLine();
            if(ImGui::Button("Remove_State"))
            {
                SelectData.iTargetState &= ~(m_iTransitionTargetState == 0 ? 0 : (1 << (m_iTransitionTargetState - 1)));
            }
            ImGui::SameLine();
            if(ImGui::Button("Clear_State"))
            {
                SelectData.iTargetState = 0;
            }
        }
        ImGui::EndChild();
    }
    //if(ImGui::CollapsingHeader("Conditions"))
    //{
    //    for(auto& ConditionName : SelectData.ConditionConst)
    //        ImGui::Text(ConditionName.c_str());
    //}
    //ImGui::InputText("Condition Name",m_szConditionName, MAX_PATH);
    //ImGui::SameLine();
    //if(ImGui::Button("Add Condition"))
    //{
    //    SelectData.ConditionConst.push_back(m_szConditionName);
    //}
    //if(ImGui::Button("Remove Condition"))
    //{
    //    SelectData.ConditionConst.pop_back();
    //}
    ImGui::End();
}



void CAnimationTool::RenderUI_EditState()
{
    // StateTransition 설정하기.
    if (!m_IsStateTransition)
        return;


    ImVec2 windowPos = ImVec2(g_iWinSizeX * 0.5f, g_iWinSizeY * 0.5f);
    ImVec2 windowSize = ImVec2(1700.f, 700.f);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

    // 현재 선택된 Actor 모델에 대한 정보로 새창 띄우기.
    ImGui::Begin("State_Transition");


    RenderUI_FromState();

    ImGui::SameLine();
    RenderUI_ToState();

    ImGui::SameLine();
    RenderUI_Transitions();

    ImGui::SameLine();
    RenderUI_OptionState();

  
    RenderUI_TransitionInfo();

    ImGui::End();
}

void CAnimationTool::LoadDat()
{
    _wstring wStrModelName = {};

    _string strModelName = "Prototype_Component_Model_";
    
    _string strModelPath = {};
    string basePathString = {};

    CModel* pModelCom = { nullptr };

	_matrix		PreTransformMatrix = XMMatrixIdentity();
	//_float fSize = 1.f;
	//_float fSize = 0.0001f;
	//_float fSize = 0.01f;
	_float fSize = 0.01f;
	PreTransformMatrix = XMMatrixScaling(fSize, fSize, fSize) * XMMatrixRotationY(XMConvertToRadians(180.f)); // Default

	static bool isCharacter = { false };
	ImGui::Checkbox("Character", &isCharacter);

	static bool isPart = { false };
	ImGui::Checkbox("Part", &isPart);

	static _float fRadians[3] = { -90.f, 0.f, 0.f };
	static _bool IsRotation[3] = { true, false, false };

	if (isPart)
	{
		ImGui::Checkbox("IsRotX", &IsRotation[0]);
		ImGui::SameLine();
		ImGui::Checkbox("IsRotY", &IsRotation[1]);
		ImGui::SameLine();
		ImGui::Checkbox("IsRotZ", &IsRotation[2]);

		
		PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);
		if (IsRotation[0])
		{
			ImGui::SetNextItemWidth(70.0f);
			ImGui::InputFloat("fRadianX", &fRadians[0]);
			
			PreTransformMatrix *= XMMatrixRotationX(XMConvertToRadians(fRadians[0]));

		}
		if (IsRotation[1])
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(70.0f);
			ImGui::InputFloat("fRadianY", &fRadians[1]);
			PreTransformMatrix *= XMMatrixRotationY(XMConvertToRadians(fRadians[1]));
		}
		if (IsRotation[2])
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(70.0f);
			ImGui::InputFloat("fRadianZ", &fRadians[2]);
			PreTransformMatrix *= XMMatrixRotationZ(XMConvertToRadians(fRadians[2]));
		}
	}

    if (ImGui::Button("Load DAT File"))
    {
        IGFD::FileDialogConfig config;

        config.path = "../../Client/Bin/Resource/";
        config.flags = ImGuiFileDialogFlags_ReadOnlyFileNameField;

        basePathString = config.path;

        ImGuiFileDialog::Instance()->OpenDialog("DAT File Load", "Import File", ".dat", config);
    }
  
    ImVec2 vMinSize = ImVec2(600, 400);
    ImVec2 vMaxSize = ImVec2(800, 400);

    if (ImGuiFileDialog::Instance()->Display(
        "DAT File Load", ImGuiWindowFlags_NoCollapse
        , vMinSize
        , vMaxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            _string strFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            strModelPath = ImGuiFileDialog::Instance()->GetCurrentFileName();

            size_t lastDotPos = strModelPath.find_last_of('.');
            if (lastDotPos != string::npos) {
                strModelName += strModelPath.substr(0, lastDotPos);
                
            }
            else
            {
                return;
            }

            wStrModelName = StringToWString(strModelName);

			HRESULT hr = {};
			// Character를 설정했으면 Character로 Load Dat
			if (isCharacter)
			{
				_float fSize = 0.01f; // Blender에서 크기가 100배 작음.
				//_float fSize = 1.f; // gltf로 가져왔으면 
				//_float fSize = 0.0001f; // Blender에서 크기가 100배 작음.
				PreTransformMatrix = XMMatrixScaling(fSize, fSize, fSize) * XMMatrixRotationY(XMConvertToRadians(180.f)); // Default
				hr = Add_Prototype_AnimModel(wStrModelName, MODELTYPE::CHARACTER, PreTransformMatrix, strFilePath.c_str());
			}
			else
				hr = Add_Prototype_AnimModel(wStrModelName, MODELTYPE::ANIM, PreTransformMatrix, strFilePath.c_str());
            
            if (FAILED(hr))
            {
                return;
            }

            size_t lastSlashPos = strFilePath.find_last_of("/\\");
            string directoryPath = "";
            if (lastDotPos != string::npos)
            {
                directoryPath = strFilePath.substr(0, lastSlashPos);
                m_ModelDirPaths.emplace(wStrModelName, directoryPath);
            }
            
            m_ModelNames.emplace_back(strModelName);
        }
        ImGuiFileDialog::Instance()->Close();
    }

}

void CAnimationTool::RenderUI_ModelPrototype()
{
    _wstring objTag = {};
    _wstring modelTag = {};

    ImGui::BeginChild("left pane", ImVec2(500, 0), true);

    static int iSelectedIndex = -1;
    _uint id = 0;

    for (auto& modelName : m_ModelNames)
    {
		_bool isSelected = (id == iSelectedIndex);

        if (ImGui::Selectable(modelName.c_str(), isSelected))
        {
            iSelectedIndex = id;
            m_Selected_PrototypeModelTag = modelName;
            m_wSelected_PrototypeModelTag = StringToWString(modelName);
        }

		id++;
    }
    ImGui::EndChild();

    ImGui::SameLine();

#ifdef _DEBUG
	if (iSelectedIndex >= 0 && iSelectedIndex < m_ModelNames.size())
		Render_Model_Detail();
#endif // _DEBUG
    
}

void CAnimationTool::RenderUI_EditModel()
{
	_wstring objTag = {};
	_wstring modelTag = {};

	ImGui::BeginChild("left pane", ImVec2(500, 0), true);

	static int iSelectedIndex = -1;
	_uint id = 0;

	for (auto& actorName : m_ActorNames)
	{
		_bool isSelected = (id == iSelectedIndex);

		if (ImGui::Selectable(actorName.c_str(), isSelected))
		{
			iSelectedIndex = id;
			m_Selected_AnimActorTag = actorName;
			m_wSelected_AnimActorTag = StringToWString(actorName);
		}
		id++;
	}
	ImGui::EndChild();

	ImGui::SameLine();

#ifdef _DEBUG
	if (iSelectedIndex >= 0 && iSelectedIndex < m_ActorNames.size())
		Render_EditModel();
#endif
}

void CAnimationTool::RenderUI_AnimationList()
{
    
    ImGui::BeginChild("Right pane", ImVec2(400, 0.f), true);

    static int iSelectedIndex = -1;
    _uint id = 0;


    // 1. 검색 버퍼 추가.
	static char szSearchBuffer[256] = "";
	ImGui::InputText("Search", szSearchBuffer, sizeof(szSearchBuffer));
	ImGui::Separator();

	// 2. 검색 필터링.
	_string strSearch = szSearchBuffer;

	// 대소문자 구분없이 검색.
	transform(strSearch.begin(), strSearch.end(), strSearch.begin(), ::tolower);


#ifdef _DEBUG
    for (auto& animName : m_AnimationActors[m_wSelected_AnimActorTag]->Get_AnimationNames())
    {
		// 1. 검색어가 있으면 필터링
		if (szSearchBuffer[0] != '\0')
		{
			_string strAnimName = animName;
			transform(strAnimName.begin(), strAnimName.end(), strAnimName.begin(), ::tolower);

			if (strAnimName.find(strSearch) == string::npos)
			{
				id++;
				continue; // 매치 안되면 스킵
			}
		}

        // 2. 애니메이션을 선택했을 경우에는 애니메이션 각각에 대한 Detail한 설정.
        if (ImGui::Selectable(animName.c_str(), id == iSelectedIndex))
        {
            iSelectedIndex = id;
            m_Selected_AnimationTag = animName;
            m_fDuration = m_AnimationActors[m_wSelected_AnimActorTag]->Get_Duration(m_Selected_AnimationTag);
            m_AnimationActors[m_wSelected_AnimActorTag]->Change_CurrentAnimation(m_Selected_AnimationTag);

            if (m_IsVisibleNotify)
            {
                m_pAnimNotifyTool->Process_Notify(m_AnimationActors[m_wSelected_AnimActorTag], m_Selected_AnimActorTag, "", m_fDuration);
                m_pAnimNotifyTool->Clear();
            }
        }
        // 다음 항목을 위해 id를 증가시킵니다.
        id++;
    }

    



    ImGui::SameLine();

#endif 
    ImGui::EndChild();

    // 애니메이션 상세 정보 조절.

    Render_Animation_Detail();


#ifdef _DEBUG
    ImGui::SameLine();

    ImGui::BeginChild("State pane", ImVec2(400, 0.f), true);

	// 여기에 회전 가능하게?
	CTransform* pTransfrom = m_AnimationActors[m_wSelected_AnimActorTag]->Get_Transform();

	static _float fRadians[3] = {0.f, 0.f, 0.f};
	static _float fTranslation[3] = {0.f, 0.f, 0.f};
	if (nullptr != pTransfrom)
	{
		ImGui::InputFloat3("Set Quaternion", fRadians);

		if (ImGui::Button("Apply Quaternion"))
		{
			_fvector vQuaternion = XMQuaternionRotationRollPitchYaw(fRadians[0], fRadians[1], fRadians[2]);
			pTransfrom->Rotation_Quaternion(vQuaternion);
		}

		ImGui::InputFloat3("Set Translation", fTranslation);

		_float3 vPos = {};
		memcpy(&vPos, fTranslation, sizeof(_float3));
		
		if (ImGui::Button("Apply Translation"))
		{
			pTransfrom->Set_State(STATE::POSITION, XMLoadFloat3(&vPos));
		}

	}

    // 현재 State 목록을 .csv로 내보냅니다.
    Export_StateAnimationMap_ToCSV();

    if (ImGui::Button("State Transition Visible"))
    {
        m_IsStateTransition = !m_IsStateTransition;
    }

    ImGui::EndChild();
    
    
#endif // _DEBUG
}



#ifdef _DEBUG

void CAnimationTool::Render_Model_Detail()
{
    ImGui::BeginChild("Right pane", ImVec2(500, 0), true);

    static float fPosition[3] = { 0.f, 0.f, 0.f };
    ImGui::InputFloat3("Position", fPosition);

    static float fRotation[3] = { 0.f, 0.f, 0.f };
    ImGui::InputFloat3("Rotation", fRotation);

    static float fScale[3] = { 1.f, 1.f, 1.f };
    ImGui::InputFloat3("Scale", fScale);

    static float fSpeedPerSec = { 10.f };
    ImGui::InputFloat("Speed", &fSpeedPerSec);

    static float fRotationPerSec = { 90.f };
    ImGui::InputFloat("RotationSpeed", &fRotationPerSec);

    static unsigned int iShaderPath = {};
    static const unsigned int min_val = static_cast<_uint>(SHADER_ANIMPATH::DEFAULT_NORMAL);
    static const unsigned int max_val = static_cast<_uint>(SHADER_ANIMPATH::NORMAL_TEXTURE);
    ImGui::SliderScalar("Shader Path", ImGuiDataType_U32, &iShaderPath, &min_val, &max_val);

	
	static char textBone[256] = "";
	ImGui::InputText("Bone Name", textBone, sizeof(textBone));

	static char textObject[256] = "";
	ImGui::InputText("Object Name", textObject, sizeof(textObject));
	
	static bool IsPart = { false };
	ImGui::Checkbox("Parts", &IsPart);

	static bool IsFacial = { false };
	ImGui::Checkbox("Facial", &IsFacial);

    if (ImGui::Button("Create Instance"))
    {
        CAnimationActor::ANIMATION_ACTOR_DESC Desc{};
        Desc.fSpeedPerSec = fSpeedPerSec;
        Desc.fRotationPerSec = XMConvertToRadians(fRotationPerSec);
        Desc.strModelTag = m_wSelected_PrototypeModelTag;

		if (IsFacial)
		{
			Desc.strShaderTag = TEXT("Prototype_Component_Shader_VtxAnimMeshCharacter");
			Desc.IsFacial = true;
		}
		else 
			Desc.strShaderTag = TEXT("Prototype_Component_Shader_VtxAnimMesh"); 

        //Desc.strComputeShaderTag = TEXT("Prototype_Component_Shader_ComputeVtxAnimMesh");
        Desc.strComputeShaderTag = TEXT("Prototype_Component_Shader_ComputeVtxAnimMeshCharacter");
        Desc.strMorphComputeShaderTag = TEXT("Prototype_Component_Shader_ComputeVtxAnimMorph");
        Desc.iShaderPath = iShaderPath;
        memcpy(&Desc.vPostion, fPosition, sizeof(_float3));
        memcpy(&Desc.vRotation, fRotation, sizeof(_float3));
        memcpy(&Desc.vScale, fScale, sizeof(_float3));
        Desc.eLevel = m_eCurLevel;
		
		if (IsPart)
		{
			_wstring wstrObjTag = TEXT("Prototype_GameObject_Actor_");
			_wstring wStrActorName = StringToWString(textObject);

			wstrObjTag += wStrActorName;

			CAnimationActor* pActor = m_AnimationActors[wstrObjTag];
			if (nullptr == pActor)
			{
				MSG_BOX("Anim Actor Not Exist");
				return;
			}
			Desc.pParentActor = pActor;


			Desc.pParentTransform = pActor->Get_Transform();
			Desc.strBoneName = textBone;
		}

        _wstring wstrObjTag = TEXT("Prototype_GameObject_Actor_");
        

        size_t last_dot_pos = m_wSelected_PrototypeModelTag.find_last_of('_');
        if (last_dot_pos != std::string::npos) {
            wstrObjTag += m_wSelected_PrototypeModelTag.substr(last_dot_pos + 1, m_wSelected_PrototypeModelTag.size());
        }
        else
        {
            MSG_BOX("경로에 없습니다.");
            return;
        }

		static _uint iID = 0;
		_bool IsExist = { false };
		for (auto& animActor : m_AnimationActors)
		{
			_wstring strTag = animActor.first;
			if (strTag == wstrObjTag)
				IsExist = true;
		}


		if (!IsExist)
		{
			if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(m_eCurLevel)
				, wstrObjTag
				, CAnimationActor::Create(m_pDevice, m_pContext))))
			{
				MSG_BOX("Animation Actor Prototype");
				return;
			}
			iID = 0;
		}
        

        CAnimationActor* pActor = dynamic_cast<CAnimationActor*>(
            m_pGameInstance->Clone_Prototype(ENUM_CLASS(m_eCurLevel)
            , wstrObjTag, PROTOTYPE::GAMEOBJECT, &Desc));
        ASSERT_CRASH(pActor);


        if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(m_eCurLevel)
            , TEXT("Layer_Actor"), pActor)))
        {
            MSG_BOX("Animation Actor 없습니다. ");
            return; 
        }

		if (IsExist)
		{
			iID += 1;
			wstrObjTag += to_wstring(iID);
		}
        m_ActorNames.emplace_back(WStringToString(wstrObjTag));

        Safe_AddRef(pActor);
        m_AnimationActors.emplace(wstrObjTag, pActor);        
    }

	ImGui::Separator();


    ImGui::EndChild();
}

void CAnimationTool::Render_EditModel()
{

	if (m_wSelected_AnimActorTag.empty())
		return;

	m_AnimationActors.at(m_wSelected_AnimActorTag)->Set_PlayAnimation(false);

	ImGui::BeginChild("Right pane", ImVec2(500, 0), true);

	static float fPosition[3] = { 0.f, 0.f, 0.f };
	ImGui::InputFloat3("Position", fPosition);
	
	CTransform* pTransformCom = m_AnimationActors.at(m_wSelected_AnimActorTag)->Get_Transform();
	m_pGameInstance->Use_Gizmo(pTransformCom);

	if (ImGui::Button("Apply Position"))
	{
		_float3 vP = { };
		memcpy(&vP, fPosition, sizeof(_float3));

		_vector vPos = XMVectorSetW(XMLoadFloat3(&vP), 1.f);
		pTransformCom->Set_State(STATE::POSITION, vPos);
	}

	static float fScale[3] = { 0.f, 0.f, 0.f };
	ImGui::InputFloat3("Scale", fScale);

	if (ImGui::Button("Apply Scale"))
	{
		_float3 vS = {};
		memcpy(&vS, fScale, sizeof(_float3));

		pTransformCom->Scale(vS);
	}


	static float fDegree[3] = { 0.f, 0.f, 0.f };
	ImGui::InputFloat3("Degree", fDegree);

	if (ImGui::Button("Apply Rotation"))
	{
		_float3 vR = {};
		memcpy(&vR, fDegree, sizeof(_float3));
		
		vR.x = XMConvertToRadians(vR.x);
		vR.y = XMConvertToRadians(vR.y);
		vR.z = XMConvertToRadians(vR.z);
		
		pTransformCom->Rotation_Quaternion(vR);
	}

	_string strBoneName = {};
	static char szName[MAX_PATH] = {};
	ImGui::InputText("Bone Name", szName, sizeof(szName));

	if (ImGui::Button("Change Bone"))
	{
		strBoneName = szName;
		m_AnimationActors.at(m_wSelected_AnimActorTag)->Change_BoneMatrixPtr(strBoneName);
	}
	

	_float4 vDebugPos = {};
	XMStoreFloat4(&vDebugPos, pTransformCom->Get_State(STATE::POSITION));
	_string strPos = {};
	strPos = "Transform (x, y, z, w) : " + to_string(vDebugPos.x) + ", " 
		+ to_string(vDebugPos.y) + ", " 
		+ to_string(vDebugPos.z) + ", " 
		+ to_string(vDebugPos.w);

	ImGui::Text(strPos.c_str());

	ImGui::EndChild();
}

#endif // _DEBUG

void CAnimationTool::Render_Animation_Detail()
{
#ifdef _DEBUG

    if (!m_Selected_AnimationTag.empty())
        m_fTrackPosition = *m_AnimationActors[m_wSelected_AnimActorTag]->Get_TrackPositionPtr(m_Selected_AnimationTag);
#endif

    _float minTrackPos = 0.f;
    _float maxTrackPos = m_fDuration;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 windowPos = ImVec2(0.f, g_iWinSizeY - 100.f);
    ImVec2 windowSize = ImVec2(600.f, 170.f);
    
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);
    
    ImGui::Begin("Animation Detail", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Animation Name : %s", m_Selected_AnimationTag.c_str());


	
	
	
	
	static float fAnimSpeed = 1.f;

#ifdef _DEBUG
	if (!m_Selected_AnimationTag.empty())
	{
		ImGui::Text("Duration : %.2f", m_AnimationActors[m_wSelected_AnimActorTag]->Get_Duration(m_Selected_AnimationTag));
		ImGui::InputFloat("|", &fAnimSpeed);

		ImGui::SameLine();
		if (ImGui::Button("Apply Speed"))
		{
			m_AnimationActors[m_wSelected_AnimActorTag]->Set_AnimationSpeed(fAnimSpeed);
		}
	}
        
    if (ImGui::SliderFloat("Track Position", &m_fTrackPosition, minTrackPos, maxTrackPos))
    {
        if (!m_Selected_AnimationTag.empty())
            m_AnimationActors[m_wSelected_AnimActorTag]->Set_TrackPosition(m_fTrackPosition);
    }
#endif
    

    _bool IsChanged = { false };
    
    if (KEYSTATE::DOWN == m_pGameInstance->Get_DIKeyState(DIK_SPACE))
    {
        IsChanged = true;
        m_IsPlayAnimation = !m_IsPlayAnimation;
    }
        

    if (ImGui::Button("Stop"))
    {
        IsChanged = true;
        m_IsPlayAnimation = false;
    }
        

    ImGui::SameLine();
    if (ImGui::Button("Play"))
    {
        IsChanged = true;
        m_IsPlayAnimation = true;
    }

#ifdef _DEBUG
	if (IsChanged)
	{
		m_AnimationActors[m_wSelected_AnimActorTag]->Set_PlayAnimation(m_IsPlayAnimation);
	}
        
#endif

    if (ImGui::Button("Notify Visible"))
        m_IsVisibleNotify = !m_IsVisibleNotify;

#ifdef _DEBUG
    ImGui::SameLine();

    if (ImGui::Button("Export Anim To Effect"))
    {
        Export_AnimationData(m_pEffectController);
    }


	if (!m_wSelected_AnimActorTag.empty())
	{
		Render_Animation_ChildDetail();
	}


#endif // _DEBUG

    ImGui::End();
}

#ifdef _DEBUG
void CAnimationTool::Render_Animation_ChildDetail()
{
	if (m_AnimationActors[m_wSelected_AnimActorTag]->Is_ChildActor())
	{
		m_AnimationActors[m_wSelected_AnimActorTag]->Child_Render();
	}
	
}

#endif // _DEBUG


#pragma endregion


#ifdef _DEBUG
void CAnimationTool::Export_StateAnimationMap_ToCSV()
{
    // 현재 애니메이션 목록을 내보냅니다. 

    if (ImGui::Button("Save AnimStateList"))
    {
        IGFD::FileDialogConfig config;
        config.path = "../../Client/Bin/Resource/Model";
        config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
        string basePathString = config.path;
        ImGuiFileDialog::Instance()->OpenDialog("Save Csv", "Export File", ".csv", config);
    }


    ImVec2 vMinSize = ImVec2(600, 400);
    ImVec2 vMaxSize = ImVec2(800, 400);

    if (ImGuiFileDialog::Instance()->Display(
        "Save Csv", ImGuiWindowFlags_NoCollapse
        , vMinSize
        , vMaxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            _string strFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            
            // 1. 파일 열기
            std::ofstream outFile(strFilePath);

            _string strStateTag = "StateName";
            _string strAnimTag = "AnimName";
            _string strTimeTag = "Duration";
            outFile << strStateTag << "," << strAnimTag << "," << strTimeTag << "\n";

            for (auto& animName : m_AnimationActors[m_wSelected_AnimActorTag]->Get_AnimationNames())
                outFile << animName << "," << animName << "," << to_string(m_AnimationActors[m_wSelected_AnimActorTag]->Get_Duration(animName)) <<"\n";
            // 2. 파일 쓰기.


            outFile.close();

        }
        ImGuiFileDialog::Instance()->Close();
    }


}


void CAnimationTool::Export_StateTransition_To_CSV()
{
    if (ImGui::Button("Save Transition"))
    {
        IGFD::FileDialogConfig config;
        config.path = "../../Client/Bin/Resource/Model";
        config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
        string basePathString = config.path;
        //ImGuiFileDialog::Instance()->OpenDialog("Save Csv", "Export File", ".csv", config);
        ImGuiFileDialog::Instance()->OpenDialog("Save Json", "Export File", ".json", config);
    }

    ImVec2 vMinSize = ImVec2(600, 400);
    ImVec2 vMaxSize = ImVec2(800, 400);

    if (ImGuiFileDialog::Instance()->Display(
        /*""Save Csv"*/"Save Json", ImGuiWindowFlags_NoCollapse
        , vMinSize
        , vMaxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            _string strFilePath = ImGuiFileDialog::Instance()->GetFilePathName();

            // 1. 파일 열기
            std::ofstream outFile(strFilePath);

            // 2. 파일 쓰기.
            
            //outFile << "From State" << "," << "ToState" << "\n";
            //
            //for (auto& strTransition : m_StateTransitions)
            //{
            //    outFile << strTransition;
            //}
            //
            //for (auto& animName : m_AnimationActors[m_wSelected_AnimActorTag]->Get_AnimationNames())
            //    outFile << animName << "," << animName << "\n";

            json Output;
            m_pAnimMachineCom->Save_AnimDatas(Output);

            Output["Transitions"] = json::array();

            for(auto& tTransitionData : m_TransitionDatas)
            {
                json Transition;
                Transition["From"] = tTransitionData.strFrom;
                Transition["To"] = tTransitionData.strTo;
                Transition["Priority"] = tTransitionData.iPriority;
                //Transition["Target State"] = /*tTransitionData.iTargetState == 0 ? 0 : */(1 << (tTransitionData.iTargetState));
                Transition["Target State"] = (tTransitionData.iTargetState);
                //다음 애니메이션의 해당 트랙 위치로 변환
                Transition["Transit Target Pos"] = tTransitionData.fTargetTrackPos;
                Transition["Transit Enable Pos"] = tTransitionData.fTransitEnablePos;
                Transition["Condition Type"] = tTransitionData.eType;

                Output["Transitions"].push_back(Transition);
            }

            Output["AnyState"] = json::array();
            for(auto& tTransitionData : m_AnyStateTransitions)
            {
                json Transition;
                Transition["To"] = tTransitionData.strTo;
                Transition["Priority"] = tTransitionData.iPriority;
                Transition["Target State"] = (tTransitionData.iTargetState);
                //다음 애니메이션의 해당 트랙 위치로 변환
                Transition["Transit Target Pos"] = tTransitionData.fTargetTrackPos;
                Transition["Transit Enable Pos"] = tTransitionData.fTransitEnablePos;
                Transition["Condition Type"] = tTransitionData.eType;
                Output["AnyState"].push_back(Transition);
            }
            outFile << Output.dump(4);

            outFile.close();

        }
        ImGuiFileDialog::Instance()->Close();
    }
}
void CAnimationTool::Import_StateTransition_From_Json()
{
    IGFD::FileDialogConfig config;
    config.path = "../../Client/Bin/Resource/Model";
    config.flags = ImGuiFileDialogFlags_ReadOnlyFileNameField;
    ImGuiFileDialog::Instance()->OpenDialog("ASM File Load", "Import File", ".json", config);
    ImVec2 vMinSize = ImVec2(600, 400);
    ImVec2 vMaxSize = ImVec2(800, 400);
    if(ImGuiFileDialog::Instance()->Display(
        "ASM File Load", ImGuiWindowFlags_NoCollapse
        , vMinSize
        , vMaxSize))
    {
        _string strFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
        _string strFileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

        ifstream file(strFilePath.c_str());
        if(false == file.is_open())
            CRASH("Load File Open");

        json ASM_Data;
        file >> ASM_Data;
        m_pAnimMachineCom->Clear_States();
        m_pAnimMachineCom->Load_AnimDatas(ASM_Data);

        for(auto& jsonTransition : ASM_Data["Transitions"])
        {
            TRANSITION_DATA tTransition{
                jsonTransition["From"],
                jsonTransition["To"],
                jsonTransition["Priority"],
                jsonTransition["Target State"],
                jsonTransition["Transit Target Pos"],
                jsonTransition["Transit Enable Pos"],
                jsonTransition["Condition Type"]
            };
            m_TransitionDatas.push_back(tTransition);
        }

        for(auto& jsonTransition : ASM_Data["AnyState"])
        {
            TRANSITION_DATA tTransition{
                "",
                jsonTransition["To"],
                jsonTransition["Priority"],
                jsonTransition["Target State"],
                jsonTransition["Transit Target Pos"],
                jsonTransition["Transit Enable Pos"],
                jsonTransition["Condition Type"]
            };
            m_AnyStateTransitions.push_back(tTransition);
        }

        file.close();
        ImGuiFileDialog::Instance()->Close();
        m_isShowImport_ST_Dialog = false;
    }
}
#endif

HRESULT CAnimationTool::Add_Prototype_AnimModel(_wstring strPrototypeName, MODELTYPE eType, _fmatrix PreTransformMatrix, const _char* pFilePath)
{
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(m_eCurLevel)
        , strPrototypeName
        , CModel::Create(m_pDevice, m_pContext, eType, PreTransformMatrix, pFilePath))))
        return E_FAIL;

    return S_OK;
}


CAnimationTool* CAnimationTool::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLevel)
{
    CAnimationTool* pInstance = new CAnimationTool(pDevice, pContext);

    if (FAILED(pInstance->Initialize(eLevel)))
    {
        MSG_BOX("Failed to Create : CAnimationTool");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CAnimationTool::Free()
{
    CBase::Free();
    Safe_Release(m_pLoader);
	Safe_Release(m_pGltfLoader);
    Safe_Release(m_pAnimNotifyTool);
    Safe_Release(m_pAnimMachineCom);
    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
    Safe_Release(m_pGameInstance);

    for (auto& pair : m_AnimationActors)
        Safe_Release(pair.second);
    m_AnimationActors.clear();

    m_ModelDirPaths.clear();
    
    m_ModelNames.clear();
    m_ActorNames.clear();
    
}

