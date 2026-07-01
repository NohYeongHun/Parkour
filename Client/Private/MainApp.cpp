#include "ClientPch.h"
#include "MainApp.h"
#include "GameSystem.h"

#include "Event_Level.h"
#include "SpringCamera.h"
#include "Mouse.h"

#include "Level_Loading.h"
#include "Level_Test.h"


CMainApp::CMainApp()
	: m_pGameInstance{ CGameInstance::GetInstance() }
	, m_pGameSystem { CGameSystem::GetInstance()}
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pGameSystem);
}

HRESULT CMainApp::Initialize()
{
	srand(static_cast<_uint>(time(nullptr)));

	// Engine Init
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

	// ImGui Context Setting
	ImGui::SetCurrentContext(m_pGameInstance->Get_ImGuiContext());

	// Jolt Collision Layer SetUp
	SetUp_CollisionLayer();

	// Jolt Physics SetUp
	m_pGameInstance->SetUp_PhysicsSystem();

	m_pGameSystem->Ready_GameSystem(m_pDevice, m_pContext);

	Ready_Prototype_ForStatic();

	Ready_Event();
	Start_Level();

	return S_OK;
}

void CMainApp::Post_Update()
{
	if (true == m_isChangeLevel)
	{
		// Wait Thread End
		m_pGameInstance->Wait_Thread_End();

		m_isChangeLevel = false;

		if (true == m_isLoad)
		{
			// PhysicX Update시 Remove ID 수집 중지
			m_pGameInstance->IsChangeLevel_ForPhysicX(true);

			// Memory Clear (Sound, Camera, Light, ETC)
			if (FAILED(m_pGameInstance->Clear_Memory()))
				CRASH("Clear");

			// GameSystem Clear
			m_pGameSystem->Clear_Resource();

			if (FAILED(m_pGameInstance->Clear_CurrentLevel_Resources(ENUM_CLASS(LEVEL::LOADING))))
				CRASH("Clear Resource");

			m_pGameInstance->Open_Level(ENUM_CLASS(LEVEL::LOADING), CLevel_Loading::Create(m_pDevice, m_pContext, m_eNextLevel));
		}
		else
		{
			if (FAILED(m_pGameInstance->Clear_CurrentLevel_Resources(ENUM_CLASS(m_eNextLevel))))
				CRASH("Clear Resource");

			CLevel* pLevel = { nullptr };
			_float fFadeDuration = {};

			switch (m_eNextLevel)
			{
			case LEVEL::TEST:
				pLevel = CLevel_Test::Create(m_pDevice, m_pContext);
				break;
			}
			ASSERT_CRASH(pLevel);

			m_pGameInstance->Open_Level(ENUM_CLASS(m_eNextLevel), pLevel);

			m_pGameInstance->IsChangeLevel_ForPhysicX(false);
			m_pGameInstance->OnFade(FADE::FADE_IN, fFadeDuration, nullptr);
		}
	}
}

void CMainApp::Update(_float fTimeDelta)
{
	m_pGameInstance->Update_Engine(fTimeDelta);

	// ImGui Frame 체크 => 레벨에서 테스트 할게 있을 때 사용 예정.
	ImGuiID DockingID = ImGui::GetID("Dock");
	ImGui::DockSpaceOverViewport(DockingID, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::Begin("Frame");
	_char szFrame[MAX_PATH] = {};
	sprintf_s(szFrame, MAX_PATH, "Frame : %d", m_iFrame);
	ImGui::Text(szFrame);
	ImGui::End();

	m_fTimeAcc += fTimeDelta;
	++m_iCnt;
	if (m_fTimeAcc > 1.f)
	{
		m_fTimeAcc = 0.f;
		m_iFrame = m_iCnt;
		m_iCnt = 0;
	}
}

void CMainApp::Render()
{
	_float4 vClearColor = _float4(0.f, 0.f, 1.f, 1.f);
	m_pGameInstance->Render_Begin(&vClearColor);
	m_pGameInstance->Draw();
	m_pGameInstance->Render_End();
}

void CMainApp::Ready_Event()
{
	m_pGameInstance->Subscribe<CHANGE_LEVEL_EVENT>(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), [this](const CHANGE_LEVEL_EVENT& event) {
		m_isChangeLevel = true;
		m_eNextLevel = event.eNextLevel;
		m_isLoad = event.isLoad;
		});
}

void CMainApp::SetUp_CollisionLayer()
{
	// 1. Object To BroadPhase
	// => 왜 넣는가? 물리 브로드페이즈 BVH 트리 중 어디에 속할지 정해야함.
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::NONE), ENUM_CLASS(BPLAYER::NONE));

	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(BPLAYER::MOVE));

	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::MAP), ENUM_CLASS(BPLAYER::NON_MOVE));

	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(BPLAYER::SENSOR));
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::GRAB), ENUM_CLASS(BPLAYER::SENSOR));
	m_pGameInstance->SetUp_ObjectToBP(ENUM_CLASS(COLLISIONLAYER::SLIDE), ENUM_CLASS(BPLAYER::SENSOR));

	// Object VS Object
	// => 어떤 오브젝트 레이어끼리 실제로 충돌해야 하는지 세밀하게 정의합니다.
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(COLLISIONLAYER::MAP));
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(COLLISIONLAYER::GRAB));
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(COLLISIONLAYER::SLIDE));

	// 장애물 탐지
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(COLLISIONLAYER::PLAYER));
	m_pGameInstance->SetUp_ObjectFilter(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(COLLISIONLAYER::GRAB));

	// Object VS BroadPhase
	// => 사전 브로드 페이즈 시 등록하지 않은 것들은 검사조차 하지 않는다.
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(BPLAYER::NON_MOVE));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(BPLAYER::MOVE));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::PLAYER), ENUM_CLASS(BPLAYER::SENSOR));

	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(BPLAYER::MOVE));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::DETECT), ENUM_CLASS(BPLAYER::SENSOR));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::GRAB), ENUM_CLASS(BPLAYER::MOVE));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::GRAB), ENUM_CLASS(BPLAYER::SENSOR));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::SLIDE), ENUM_CLASS(BPLAYER::MOVE));
	m_pGameInstance->SetUp_ObjectVsBPFilter(ENUM_CLASS(COLLISIONLAYER::SLIDE), ENUM_CLASS(BPLAYER::SENSOR));
	

}

void CMainApp::Ready_Prototype_ForStatic()
{
#pragma region SHADER
	// Shader_VtxMesh
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxMesh"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxMesh.hlsl"), VTXMESH::Elements, VTXMESH::iNumElements))))
		CRASH("Shader_VtxMesh");

	// Shader_VtxMesh_Instance
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxMesh_Instance"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxMesh_Instance.hlsl"), VTXMESHINSTANCE::Elements, VTXMESHINSTANCE::iNumElements))))
		CRASH("Shader_VtxMesh");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxAnimMesh.hlsl"), VTXANIMMESH::Elements, VTXANIMMESH::iNumElements))))
		CRASH("Shader_VtxAnimMesh");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxSkyBox"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxSkyBox.hlsl"), VTXMESH::Elements, VTXMESH::iNumElements))))
		CRASH("Shader_VtxSkyBox");
#pragma endregion

#pragma region COLLIDER
	// Rigidbody
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Rigidbody"),
		CRigidbody::Create(m_pDevice, m_pContext))))
		CRASH("Rigidbody");

	// Collider
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider"),
		CCollider::Create(m_pDevice, m_pContext))))
		CRASH("Collider");
#pragma endregion

#pragma region VIBUFFER
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Sphere"),
		CVIBuffer_Sphere::Create(m_pDevice, m_pContext))))
		CRASH("Failed to Add Prototype VIBuffer Sphere");

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Componnent_VIBuffer_Cube"),
		CVIBuffer_Cube::Create(m_pDevice, m_pContext))))
		CRASH("Failed to Add Prototype VIBuffer Cube");
#pragma endregion


	// SpringCamera
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_GameObject_SpringCamera"),
		CSpringCamera::Create(m_pDevice, m_pContext))))
		CRASH("SpringCamera");

	// Mouse
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_GameObject_Mouse"),
		CMouse::Create(m_pDevice, m_pContext))))
		CRASH("Mouse");

}

void CMainApp::Start_Level()
{
	//CHANGE_LEVEL_EVENT event{ LEVEL::LOGO, true };
	CHANGE_LEVEL_EVENT event{ LEVEL::TEST, true };
	m_pGameInstance->Publish(ENUM_CLASS(STATIC::STATIC), TEXT("Event_Change_Level"), event);
}

CMainApp* CMainApp::Create()
{
	CMainApp* pInstance = new CMainApp();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : MainApp");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMainApp::Free()
{
	__super::Free();

	m_pGameSystem->Release_System();
	Safe_Release(m_pGameSystem);

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
	m_pGameInstance->Release_Engine();
	Safe_Release(m_pGameInstance);
}
