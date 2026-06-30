#include "EditorPch.h"
#include "EditorApp.h"

#include "BoneReadbackProfiler.h"
#include "Event_Level.h"

#include "Level_Edit.h"
#include "Level_Shader.h"
#include "Level_Animation.h"
#include "Level_Effect.h"
#include "Level_Map.h"
#include "Level_UI.h"
#include "Level_ASM.h"
#include "Level_Camera.h"
#include "Level_SFX.h"
//Dummy
#include "EditDummy_Wolf.h"
#include "EditDummy_Augusta.h"
#include "EditDummy_Map.h"
#include "EditDummy_Target.h"

CEditorApp::CEditorApp()
	: m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CEditorApp::Initialize()
{
	srand(static_cast<_uint>(time(nullptr)));

	ENGINE_DESC EngineDesc = {};
	EngineDesc.hWnd = g_hWnd;
	EngineDesc.hInst = g_hInst;
	EngineDesc.eMode = WINMODE::WIN;
	EngineDesc.iSizeX = g_iWinSizeX;
	EngineDesc.iSizeY = g_iWinSizeY;
	EngineDesc.iNumLevel = ENUM_CLASS(LEVEL::END);
	EngineDesc.iNumChannel = FMOD_CHANNEL_MAX;
	EngineDesc.iNumCollisionLayer = ENUM_CLASS(COLLISIONLAYER::END);

	if (FAILED(m_pGameInstance->Ready_Engine(EngineDesc, &m_pDevice, &m_pContext)))
		return E_FAIL;

	// ImGui Context ?곕룞
	ImGui::SetCurrentContext(m_pGameInstance->Get_ImGuiContext());

	// Jolt Collision Layer SetUp
	SetUp_CollisionLayer();
	// Jolt PhysicsSystem SetUp
	m_pGameInstance->SetUp_PhysicsSystem();

	Ready_Event();
	Ready_Prototype_ForStatic();
	Start_Level();
	Ready_Dummies();
	Ready_Sound();



	return S_OK;
}

void CEditorApp::Post_Update()
{
	if (true == m_isChangeLevel)
	{
		// Wait Thread End
		m_pGameInstance->Wait_Thread_End();

		m_isChangeLevel = false;

		if (FAILED(m_pGameInstance->Clear_Memory()))
			return;

		if (FAILED(m_pGameInstance->Clear_CurrentLevel_Resources(ENUM_CLASS(m_eNextLevel))))
			CRASH("Clear Resource");

		CLevel* pLevel = { nullptr };

		switch (m_eNextLevel)
		{
		case LEVEL::EDIT:
			pLevel = CLevel_Edit::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::SHADER:
			pLevel = CLevel_Shader::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::ANIMATION:
			pLevel = CLevel_Animation::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::EFFECT:
			pLevel = CLevel_Effect::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::MAP:
			pLevel = CLevel_Map::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::UI:
			pLevel = CLevel_UI::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::CAMERA:
			pLevel = CLevel_Camera::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::STATEMACHINE:
			pLevel = CLevel_ASM::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::SFX:
			pLevel = CLevel_SFX::Create(m_pDevice, m_pContext);
			break;
		}

		if (nullptr == pLevel)
			CRASH("Level Create Fail");

		m_pGameInstance->Open_Level(ENUM_CLASS(m_eNextLevel), pLevel);
		
	}
}

void CEditorApp::Update(_float fTimeDelta)
{
	//BoneReadbackProfiler::BeginFrame();

	m_pGameInstance->Update_Engine(fTimeDelta);

	ImGui::Begin("Level");


	if (ImGui::Button("Shader", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{ LEVEL::SHADER, true };
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	if (ImGui::Button("Animation", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{ LEVEL::ANIMATION, true };
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	if (ImGui::Button("Effect", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{ LEVEL::EFFECT, true };
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	if (ImGui::Button("Map", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{ LEVEL::MAP, true };
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	if (ImGui::Button("UI", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{ LEVEL::UI, true };
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	if (ImGui::Button("Camera", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{ LEVEL::CAMERA, true };
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	if(ImGui::Button("State Machine", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{LEVEL::STATEMACHINE, true};
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	if (ImGui::Button("SFX", ImVec2(100.f, 50.f)))
	{
		CHANGE_LEVEL_EVENT event{ LEVEL::SFX, true };
		m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
	}
	
	ImGui::End();

	ImGui::Begin("Frame");
	_char szFrame[MAX_PATH] = {};
	sprintf_s(szFrame, MAX_PATH, "Frame : %d", m_iFrame);
	ImGui::Text(szFrame);
	ImGui::End();

	//BoneReadbackProfiler::DrawImGui();

	m_fTimeAcc += fTimeDelta;
	++m_iCnt;
	if (m_fTimeAcc > 1.f)
	{
		m_fTimeAcc = 0.f;
		m_iFrame = m_iCnt;
		m_iCnt = 0;
	}
}

void CEditorApp::Render()
{
	//_float4 vClearColor = _float4(0.f, 0.f, 1.f, 1.f);
	_float4 vClearColor = _float4(0.f, 0.f, 1.f, 1.f);
	m_pGameInstance->Render_Begin(&vClearColor);
	m_pGameInstance->Draw();
	m_pGameInstance->Render_End();

	BoneReadbackProfiler::EndFrame();
}

void CEditorApp::SetUp_CollisionLayer()
{
	// Object To BroadPhase
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::NONE), ENUM_CLASS(BPLAYER::NONE));
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::MAP), ENUM_CLASS(BPLAYER::NON_MOVE));
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(BPLAYER::MOVE));
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(BPLAYER::SENSOR));
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::ENEMY), ENUM_CLASS(BPLAYER::MOVE));

	// Object VS Object
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(COLLISIONLAYER::ENEMY));
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(COLLISIONLAYER::ENEMY));
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(COLLISIONLAYER::CAMERA));

	// Object VS BroadPhase
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(BPLAYER::MOVE));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(BPLAYER::MOVE));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::ENEMY), ENUM_CLASS(BPLAYER::SENSOR));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::ENEMY), ENUM_CLASS(BPLAYER::MOVE));
}

void CEditorApp::Ready_Event()
{
	m_pGameInstance->Subscribe<CHANGE_LEVEL_EVENT>(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), [this](const CHANGE_LEVEL_EVENT& event) {
		if (m_eNextLevel == event.eNextLevel)
			return;
			m_isChangeLevel = true;
			m_eNextLevel = event.eNextLevel;
			m_isLoad = event.isLoad;
		});
}


	void CEditorApp::Ready_Prototype_ForStatic()
{
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Rigidbody"),
		CRigidbody::Create(m_pDevice, m_pContext))))
		CRASH("Rigidbody");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider"),
		CCollider::Create(m_pDevice, m_pContext))))
		CRASH("Collider");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Componnent_VIBuffer_Rect"),
		CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
		CRASH("VIBuffer_Rect");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Componnent_VIBuffer_Rect_Instance"),
		CVIBuffer_Rect_Instance::Create(m_pDevice, m_pContext, 10))))
		CRASH("VIBuffer_Rect");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VAMesh"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../Client/Bin/ShaderFiles/Shader_VtxVaMesh.hlsl"), VTX_VAMESH::Elements, VTX_VAMESH::iNumElements))))
		CRASH("Shader_VAMesh");

	// Shader_SFX
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_SFX_Burst"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../Client/Bin/ShaderFiles/Shader_SFX_Burst.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
		CRASH("Shader_ScreenEffect");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_SFX_Burst_Instance"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../Client/Bin/ShaderFiles/Shader_SFX_Burst_Instance.hlsl"), VTXRECTINSTANCE::Elements, VTXRECTINSTANCE::iNumElements))))
		CRASH("Shader_ScreenEffect");


	SHADER_MACRO eShaderMacro = {
		{"THREAD_X", "64" }
		,{"THREAD_Y", "1" }
		,{"THREAD_Z", "1" }
		, { NULL, NULL }
	};

	string strEntryPoint = "CSMain";
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_ComputeVtxAnimMesh"),
		CComputeShader::Create(m_pDevice, m_pContext, TEXT("../../Client/Bin/ShaderFiles/Shader_ComputeVtxAnimMesh.hlsl")
			, eShaderMacro, strEntryPoint))))
		CRASH("Failed Load AnimMesh Shader");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_ComputeVtxAnimMeshCharacter"),
		CComputeShader::Create(m_pDevice, m_pContext, TEXT("../../Client/Bin/ShaderFiles/Shader_ComputeVtxAnimMeshCharacter.hlsl")
			, eShaderMacro, strEntryPoint))))
		CRASH("Failed Load AnimMesh Shader");


	eShaderMacro = {
		{"THREAD_X", "256" }
		,{"THREAD_Y", "1" }
		,{"THREAD_Z", "1" }
		, { NULL, NULL }
	};

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_ComputeVtxAnimMorph"),
		CComputeShader::Create(m_pDevice, m_pContext, TEXT("../../Client/Bin/ShaderFiles/Shader_ComputeVtxAnimMorph.hlsl")
			, eShaderMacro, strEntryPoint))))
		CRASH("Failed Load AnimMorph Shader");
}

void CEditorApp::Ready_Dummies()
{
	if(FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_GameObject_Dummy_Wolf"), CEditDummy_Wolf::Create(m_pDevice, m_pContext))))
	   CRASH("Failed Add Prototype Dummy Wolf");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_GameObject_Dummy_Augu"), CEditDummy_Augusta::Create(m_pDevice, m_pContext))))
		CRASH("Failed Add Prototype Dummy Augu");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_GameObject_Dummy_Map"), CEditDummy_Map::Create(m_pDevice, m_pContext))))
		CRASH("Failed Add Prototype Dummy Map");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_GameObject_Dummy_Target"), CEditDummy_Target::Create(m_pDevice, m_pContext))))
		CRASH("Failed Add Prototype Dummy Target");
}

void CEditorApp::Ready_Sound()
{
	/*m_pGameInstance->Load_Sound_FromFolderRecursive("../../Client/Bin/Resource/Sound/2D/", false);
	m_pGameInstance->Load_Sound_FromFolderRecursive("../../Client/Bin/Resource/Sound/3D/", true);*/
}

void CEditorApp::Start_Level()
{
	CHANGE_LEVEL_EVENT event{ LEVEL::EDIT, true };
	m_pGameInstance->Publish(ENUM_CLASS(LEVEL::STATIC), TEXT("Event_Change_Level"), event);
}

CEditorApp* CEditorApp::Create()
{
	CEditorApp* pInstance = new CEditorApp();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : MainApp");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEditorApp::Free()
{
	__super::Free();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	m_pGameInstance->Release_Engine();
	Safe_Release(m_pGameInstance);
}
