#include "EditorPch.h"
#include "AnimationActor.h"
#include "Model.h"

#include "SpringCamera_Edit.h"
#include "EditorPropWeapon.h"

CAnimationActor::CAnimationActor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CContainerObject{ pDevice, pContext }
{
}

CAnimationActor::CAnimationActor(const CAnimationActor& Prototype)
    : CContainerObject(Prototype)
{
}

HRESULT CAnimationActor::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CAnimationActor::Initialize_Clone(void* pArg)
{
    ANIMATION_ACTOR_DESC* pDesc = static_cast<ANIMATION_ACTOR_DESC*>(pArg);
    if (FAILED(CContainerObject::Initialize_Clone(pDesc)))
        return E_FAIL;

	m_IsFacial = pDesc->IsFacial;
    m_eCurLevel = pDesc->eLevel;

    m_iShaderPath = pDesc->iShaderPath;

    _fvector vPos = XMVectorSetW(XMLoadFloat3(&pDesc->vPostion), 1.f);
    m_pTransformCom->Set_State(STATE::POSITION, vPos);
    m_pTransformCom->Scale(pDesc->vScale);
    
    _float3 vRadian = { 
        XMConvertToRadians(pDesc->vRotation.x),
        XMConvertToRadians(pDesc->vRotation.y),
        XMConvertToRadians(pDesc->vRotation.z) };
    m_pTransformCom->Rotation_Quaternion(vRadian);

    // Model의 Dat Folder Path
    m_strModelDatPath = pDesc->strModelDatPath;

#ifdef _DEBUG
	if (!(pDesc->strBoneName.empty()) && nullptr != pDesc->pParentTransform)
	{

		m_pParentTransform = pDesc->pParentTransform;
		m_pParentActor = pDesc->pParentActor;
		m_pSocketMatrix = m_pParentActor->Get_BoneMatrix(pDesc->strBoneName);

		//m_pParentActor->Set_ChildActor(this);
		m_pParentActor->Set_ChildActors(this);
	}

#endif // _DEBUG



    if (FAILED(Ready_Components(pDesc)))
    {
        CRASH("Failed Ready_Components");
        return E_FAIL;
    }



    // Default는 0번 애니메이션 실행.
#ifdef _DEBUG
    m_strCurrentAnimation = m_pModelCom->Get_AnimationNames()[0];
#endif // _DEBUG

    
	

    m_IsPlayAnimation = true;
	//m_strCurrentAnimation = "Blend_BasePose";
    //m_pModelCom->Play_Animation_GPU(m_pComputeShaderCom, "Blend_BasePose", 0.f, &m_fTrackPosition, true, 0.01f);

	ANIMATION_PLAY_DESC playDesc{};
	playDesc.fTimeDelta = 0.f;
	playDesc.strAnimationName = "Blend_BasePose";
	playDesc.pTrackPosition = &m_fTrackPosition;
	playDesc.isFacial = false;

	ROOTMOTION_DESC rootMotionDesc{};
	rootMotionDesc.fRate = 0.01f;
	rootMotionDesc.isEnable = true;
	rootMotionDesc.isRotate = true;
	rootMotionDesc.isTranslate = true;

    m_pModelCom->Play_Animation_GPU(m_pComputeShaderCom, playDesc, rootMotionDesc);

#ifdef _DEBUG
	XMStoreFloat4(&m_vInitPosition, m_pTransformCom->Get_State(STATE::POSITION));
#endif // _DEBUG

    

    //m_pTransformCom->Scale(pDesc->vScale);
    // Look 벡터 설정한 방향으로 잘갑니다 지금.

	// ParentActor가 있으면 아래 Ready_Camera 실행하지않음.
	if (m_pParentActor != nullptr)
		return S_OK;

	if (FAILED(Ready_Camera()))
		CRASH("Camera");
	m_fOffsetY = 1.f;

    return S_OK;
}

void CAnimationActor::Priority_Update(_float fTimeDelta)
{
    CContainerObject::Priority_Update(fTimeDelta);

#ifdef _DEBUG
    // PartObjects 갱신
    for (auto& pPart : m_PartObjects)
    {
        if (pPart.second->IsActivate())
            pPart.second->Priority_Update(fTimeDelta);
    }
#endif
}

void CAnimationActor::Update(_float fTimeDelta)
{
    CContainerObject::Update(fTimeDelta);

    m_fTimeDelta = fTimeDelta;

#ifdef _DEBUG
    // PartObjects 갱신
    for (auto& pPart : m_PartObjects)
    {
        if (pPart.second->IsActivate())
            pPart.second->Update(fTimeDelta);
    }
#endif
   
    /*if (m_IsPlayAnimation)
        m_pModelCom->Play_RibAnimation_GPU(strRibAnimation, fTimeDelta);*/


    _bool IsAnimationEnd = { false };
	
	ANIMATION_PLAY_DESC playDesc{};
	playDesc.fTimeDelta = fTimeDelta * m_fAnimationSpeed;
	playDesc.strAnimationName = m_strCurrentAnimation;
	playDesc.pTrackPosition = &m_fTrackPosition;

	ROOTMOTION_DESC rootMotionDesc{};
	rootMotionDesc.fRate = 1.f;
	rootMotionDesc.isEnable = true;
	rootMotionDesc.isRotate = true;
	rootMotionDesc.isTranslate = true;
    if (m_IsPlayAnimation)
    {
		if (m_IsFacial)
		{
			playDesc.isFacial = true;

			
			IsAnimationEnd = m_pModelCom->Play_Animation_GPU(m_pComputeShaderCom, m_pMorphComputeShaderCom, playDesc, rootMotionDesc);
		}
		else
		{
			playDesc.isFacial = false; 
			/*if (m_IsGPU)
			{
				IsAnimationEnd = m_pModelCom->Play_Animation_GPU(m_pComputeShaderCom, playDesc, rootMotionDesc);
			}
			else
			{
				IsAnimationEnd = m_pModelCom->Play_Animation_CPU(m_strCurrentAnimation, fTimeDelta * m_fAnimationSpeed, &m_fTrackPosition, false, true, false, true, 1.f);
			}*/
			
			IsAnimationEnd = m_pModelCom->Play_Animation_CPU(m_strCurrentAnimation, fTimeDelta * m_fAnimationSpeed, &m_fTrackPosition, false, true, false, true, 1.f);
		}

        m_pModelCom->Sync_RootNode(m_pTransformCom, fTimeDelta);
    }

#ifdef _DEBUG
    // Jump Second F
    //const _float4x4* RootMatrix = m_pModelCom->Get_BoneMatrixPtr("Root");
    //const _float4x4* HairMatrix = m_pModelCom->Get_BoneMatrixPtr("Bone_Hair001_M");
    //OutPutDebugMatrix(TEXT("Root"), *RootMatrix);
    //OutPutDebugMatrix(TEXT("Bone_Hair001_M"), *HairMatrix);
#endif // _DEBUG


    if (IsAnimationEnd)
        m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4(&m_vInitPosition));
  

    //m_pModelCom->Sync_RootNode(m_pTransformCom, 0.f);
        
#ifdef _DEBUG
	_int iBoneIndex = 0;
    //m_pModelCom->Bind_Bone_to_GUI(iBoneIndex, m_pTransformCom->Get_WorldMatrix());
	//
    //m_pModelCom->Render_Gizmo(m_pTransformCom->Get_WorldMatrix());
#endif // _DEBUG

	if (nullptr != m_pSpringCamera)
	{
		ImGui::Begin("Offset");
		ImGui::Text("OffsetY : ");
		ImGui::SameLine();
		ImGui::InputFloat("##", &m_fOffsetY);
		ImGui::End();
		m_pSpringCamera->Update_Target(m_pTransformCom->Get_State(STATE::POSITION), m_fOffsetY);
	}
	
}

void CAnimationActor::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pSocketMatrix && nullptr != m_pParentTransform)
	{
		XMStoreFloat4x4(&m_CombinedMatrix,
			m_pTransformCom->Get_WorldMatrix() *
			XMLoadFloat4x4(m_pSocketMatrix) *
			m_pParentTransform->Get_WorldMatrix());
	}
	else
	{
		XMStoreFloat4x4(&m_CombinedMatrix, m_pTransformCom->Get_WorldMatrix());
	}
	

    CContainerObject::Late_Update(fTimeDelta);

#ifdef _DEBUG
    // PartObjects 갱신
    for (auto& pPart : m_PartObjects)
    {
        if (pPart.second->IsActivate())
            pPart.second->Late_Update(fTimeDelta);
    }
#endif

    if (FAILED(m_pGameInstance->Add_Render_Object(RENDERGROUP::DYNAMIC, this)))
        return;
}

void CAnimationActor::Render()
{
    Bind_Resources();

	if (m_IsFacial)
		Render_Facial();
	else
		Render_Default();

#ifdef _DEBUG
	//Print_WorldMatrix();
#endif // _DEBUG
}

void CAnimationActor::Render_Shadow()
{

}

#ifdef _DEBUG
const vector<_string>& CAnimationActor::Get_AnimationNames() const
{
    ASSERT_CRASH(m_pModelCom);
    return m_pModelCom->Get_AnimationNames();
}

_float* CAnimationActor::Get_TrackPositionPtr(const _string& strAnimName)
{
    ASSERT_CRASH(m_pModelCom);
    return m_pModelCom->Get_TrackPositionPtr(strAnimName);
}

_float CAnimationActor::Get_Duration(const _string& strAnimName)
{
    ASSERT_CRASH(m_pModelCom);
    return m_pModelCom->Get_Duration(strAnimName);
}

// Notify에서 사용할 현재 선택된 애니메이션 이름
const _string& CAnimationActor::Get_CurrentAnimationNames() const
{
    ASSERT_CRASH(m_pModelCom);
    return m_strCurrentAnimation;
}

// Notify에서 사용할 현재 선택된 애니메이션의 최대 TrackPosition
const _float CAnimationActor::Get_CurrentAnimationDuration() const
{
    ASSERT_CRASH(m_pModelCom);
    return m_pModelCom->Get_Duration(m_strCurrentAnimation);
}

HRESULT CAnimationActor::Bind_Bone_to_GUI()
{
	_int iBoneIndex = 0;
    if(FAILED(m_pModelCom->Bind_Bone_to_GUI(iBoneIndex, m_pTransformCom->Get_WorldMatrix())))
        return E_FAIL;
    return S_OK;
}


// Notify에서 사용할 현재 선택된 애니메이션의 최대 프레임 정보?

void CAnimationActor::Change_CurrentAnimation(_string strAnimName)
{
	if (!strAnimName.empty())
	{
		if (!m_pModelCom->Find_Animation(strAnimName))  // 만약 없는 애니메이션이면? 아무것도 하지마라.
			return;
		m_strCurrentAnimation = strAnimName;
		
		
		if (nullptr != m_pChildActor)
			m_pChildActor->Change_CurrentAnimation(strAnimName);
	}
}

void CAnimationActor::Set_TrackPosition(_float fTrackPosition)
{
    ASSERT_CRASH(m_pModelCom);
    // 어차피 현재거 설정하니까 매개변수로 가져올 필요가 없을 듯.
    m_pModelCom->Set_TrackPosition(m_strCurrentAnimation, fTrackPosition);

    // TrackPosition을 설정하면서 만약 Stop인 경우에도 확인할 수 있게 Play Animation을 실행합니다.
   /* if (!m_IsPlayAnimation)
        m_pModelCom->Play_Animation(m_strCurrentAnimation, m_fTimeDelta, &m_fTrackPosition, false);*/

	ANIMATION_PLAY_DESC playDesc{};
	playDesc.fTimeDelta = 0.f;
	playDesc.strAnimationName = m_strCurrentAnimation;
	playDesc.pTrackPosition = &m_fTrackPosition;
	playDesc.isFacial = false;

	ROOTMOTION_DESC rootMotionDesc{};
	rootMotionDesc.fRate = 0.1f;
	rootMotionDesc.isEnable = true;
	rootMotionDesc.isRotate = true;
	rootMotionDesc.isTranslate = true;

    if (!m_IsPlayAnimation)
    {
        // 3. m_fTrackPosition을 방금 설정한 값으로 업데이트합니다.
        //    (Play_Animation_GPU가 이 값을 참조하기 때문)
        m_fTrackPosition = fTrackPosition;

        // 4. fTimeDelta = 0.f로 GPU 업데이트를 1회 실행합니다.
        //    (기존 주석 코드를 GPU 버전으로 변경)
		//m_pModelCom->Play_Animation_CPU(m_strCurrentAnimation, 0.f, &m_fTrackPosition, false);
        m_pModelCom->Play_Animation_GPU(m_pComputeShaderCom,
			playDesc, rootMotionDesc);

       // 5. 루트 모션도 멈춘 위치에서 동기화합니다.
       // m_pModelCom->Sync_RootNode(m_pTransformCom, 0.f);
    }

}
void CAnimationActor::Set_PlayAnimation(_bool IsPlay)
{
    m_IsPlayAnimation = IsPlay;
	for (auto& pChildActor : m_ChildActors)
	{
		if (nullptr != pChildActor)
			pChildActor->Set_PlayAnimation(m_IsPlayAnimation);
	}
		
}

// 폴더에 존재하는 모든 애니메이션 json을 읽어와서 등록합니다.
void CAnimationActor::Register_AllNotifies(const _string& strFolderPath)
{

    auto colliderCallback = [this](const _wstring& tag, bool active) {
        this->Collider_Active(tag, active); // 'this->'는 생략 가능
    };

    auto effectCallBack = [this](const _wstring& tag) {
        this->Effect_Active(tag);
    };
	auto objectCallBack = [this](const _wstring& tag) {
		this->Object_Func(tag);
		};

#ifdef _DEBUG
	m_pModelCom->Clear_AllNotifies();
#endif // _DEBUG

	
	m_pModelCom->Register_AllNotifies(strFolderPath, colliderCallback, effectCallBack, objectCallBack);
    
}
void CAnimationActor::Collider_Active(const _wstring&, _bool)
{	

}
void CAnimationActor::Effect_Active(const _wstring& tag)
{
    _matrix matWorld = m_pTransformCom->Get_WorldMatrix();
    m_pGameInstance->Spawn_PoolingObject(tag, matWorld, m_pModelCom);
}

void CAnimationActor::Object_Func(const _wstring& tag)
{
	_wstring var1, var2, var3, var4;
	wstringstream wss(tag);

	getline(wss, var1, L'|');
	getline(wss, var2, L'|');
	getline(wss, var3, L'|');
	getline(wss, var4, L'|');

	if (var1 == TEXT("Sound"))
	{
		_wstring strSoundType = var2; // Sound Type
		_wstring strSoundTag = var3; // Sound Tag
		_float fVolume = stof(var4); // Volume 크기.

		if (var2 == TEXT("Voice"))
			m_pGameInstance->Play_Sound(strSoundTag, ENUM_CLASS(CHANNEL::PLAYER_VOICE), fVolume);
		else if (var2 == TEXT("Action"))
			m_pGameInstance->Play_Sound(strSoundTag, ENUM_CLASS(CHANNEL::PLAYER_ACTION), fVolume);
		else if (var2 == TEXT("QTE"))
			m_pGameInstance->Play_Sound(strSoundTag, ENUM_CLASS(CHANNEL::PLAYER_ACTION), fVolume);
	}

}

const _float4x4* CAnimationActor::Get_BoneMatrix(const _string& strBoneName)
{
    if (nullptr == m_pModelCom)
    {
        MSG_BOX("Model nullptr");
        return nullptr;
    }
        
    const _float4x4* pBoneMatrix = m_pModelCom->Get_BoneMatrixPtr(strBoneName.c_str());

    if (nullptr == pBoneMatrix)
    {
        MSG_BOX("Bone Name Error");
        return nullptr;
    }

    return pBoneMatrix;
}
const _float4x4* CAnimationActor::Get_WorldMatrixPtr()
{
    return m_pTransformCom->Get_WorldMatrixPtr();
}

void CAnimationActor::Set_ChildActor(CAnimationActor* pChildActor)
{
	m_pChildActor = pChildActor;;
}

void CAnimationActor::Set_ChildActors(CAnimationActor* pChildActor)
{
	m_ChildActors.emplace_back(pChildActor);
}

_bool CAnimationActor::Is_ChildActor()
{
	return m_ChildActors.size() > 0;
}

//void CAnimationActor::Child_Render()
//{
//	ASSERT_CRASH(m_pChildActor);
//	m_pChildActor->Render_Detail();
//
//}
void CAnimationActor::Child_Render()
{
	int index = 0; // 인덱스 관리 (ID 충돌 방지용)
	for (auto& pChildActor : m_ChildActors)
	{
		if (nullptr != pChildActor)
		{
			pChildActor->Render_Detail(index++); // 인자 없이 호출 가능
		}
			
	}
}

void CAnimationActor::Render_Detail()
{
	if (m_strCurrentAnimation.empty())
		return;

	_float fDuration = m_pModelCom->Get_Duration(m_strCurrentAnimation);
	_float minTrackPos = 0.f;
	_float maxTrackPos = fDuration;

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowPos = ImVec2(0.f, g_iWinSizeY - 200.f);
	ImVec2 windowSize = ImVec2(600.f, 170.f);

	static int instanceCount = 0;
	windowPos.y -= instanceCount * (windowSize.y + 10.f);
	instanceCount++;

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Once);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

	//ImGui::SetNextWindowPos(windowPos, ImGuiCond_Once);
	//ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

	ImGui::PushID(this);  // 고유 ID 푸시

	char uniqueTitle[128];
	sprintf_s(uniqueTitle, "Child Animation Detail [%p]", this);
	ImGui::Begin(uniqueTitle, nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Child Animation Name : %s", m_strCurrentAnimation.c_str());

	static float fAnimSpeed = 1.f;
	if (!m_strCurrentAnimation.empty())
	{
		ImGui::Text("Child Duration : %.2f", fDuration);
		ImGui::InputFloat("|", &fAnimSpeed);
		ImGui::SameLine();
		if (ImGui::Button("Apply Speed"))
		{
			Set_AnimationSpeed(fAnimSpeed);
		}
	}

	if (ImGui::SliderFloat("Child Track Position", &m_fTrackPosition, minTrackPos, maxTrackPos))
		Set_TrackPosition(m_fTrackPosition);

	_bool IsChanged = { false };

	if (KEYSTATE::DOWN == m_pGameInstance->Get_DIKeyState(DIK_LALT))
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

	ImGui::End();
	ImGui::PopID();  // ID 팝
}

void CAnimationActor::Render_Detail(_int iIndex)
{
	// 1. 예외 처리: 애니메이션이 없거나 모델이 없으면 리턴
	if (m_strCurrentAnimation.empty() || nullptr == m_pModelCom)
		return;

	_float fDuration = m_pModelCom->Get_Duration(m_strCurrentAnimation);
	_float minTrackPos = 0.f;
	_float maxTrackPos = fDuration;

	// 2. 윈도우 위치 계산 (화면 하단 기준, 인덱스에 따라 위로 쌓임)
	ImVec2 windowSize = ImVec2(600.f, 170.f);
	// g_iWinSizeY에서 200픽셀 띄우고, 인덱스만큼 위로 올림
	ImVec2 windowPos = ImVec2(0.f, g_iWinSizeY - 200.f - (iIndex * (windowSize.y + 10.f)));

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Once); // 위치 강제 (Always 권장)
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

	// 3. 고유 ID 푸시 (충돌 방지)
	ImGui::PushID(this);

	char uniqueTitle[128];
	sprintf_s(uniqueTitle, "Child Animation Detail [%p]", this);

	// 4. 윈도우 시작
	// Begin이 false를 반환해도(접힘 상태) End는 반드시 호출해야 하므로 if문 밖에서 End 처리
	bool isWindowOpen = ImGui::Begin(uniqueTitle, nullptr, ImGuiWindowFlags_NoCollapse);

	if (isWindowOpen)
	{
		ImGui::Text("Child Animation Name : %s", m_strCurrentAnimation.c_str());

		static float fAnimSpeed = 1.f;

		ImGui::Text("Child Duration : %.2f", fDuration);

		// InputFloat에도 ID가 필요할 수 있음 (static 변수 공유 문제 방지 위해 ## 사용)
		ImGui::InputFloat("Speed##Child", &fAnimSpeed);

		ImGui::SameLine();
		if (ImGui::Button("Apply Speed"))
		{
			Set_AnimationSpeed(fAnimSpeed);
		}

		if (ImGui::SliderFloat("Child Track Position", &m_fTrackPosition, minTrackPos, maxTrackPos))
			Set_TrackPosition(m_fTrackPosition);

		_bool IsChanged = false;

		// 키 입력 처리는 중복 방지를 위해 포커스가 있을 때만 하거나 주의 필요
		if (ImGui::IsWindowFocused() && KEYSTATE::DOWN == m_pGameInstance->Get_DIKeyState(DIK_LALT))
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
	}

	// [중요] Begin을 호출했으면 무조건 End를 호출해야 함 (Missing End 방지)
	ImGui::End();
	ImGui::PopID();
}

void CAnimationActor::Print_WorldMatrix()
{
	_float4x4 matWorld = {};
	XMStoreFloat4x4(&matWorld, m_pTransformCom->Get_WorldMatrix());

	OutPutDebugMatrix(TEXT("AnimActor World Matrix : "), matWorld);
}

void CAnimationActor::Change_BoneMatrixPtr(const _string& strBoneName)
{
	// 1. 부모 객체가 없으면 반환
	if (nullptr == m_pParentActor)
		return;

	// 2. 부모 객체에 뼈행렬 없어도반환
	const _float4x4* pSocketMatrix = { nullptr };
	pSocketMatrix = m_pParentActor->Get_BoneMatrix(strBoneName);

	if (nullptr == pSocketMatrix)
		return;

	m_pSocketMatrix = pSocketMatrix;

}
#endif

// 1. 행렬 
void CAnimationActor::Bind_Resources()
{
	/*if (FAILED(m_pTransformCom->Bind_Matrix(m_pShaderCom, "g_WorldMatrix")))
        CRASH("Failed Bind Matrix");*/

	if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedMatrix)))
		CRASH("Failed Bind Matrix");

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_TransformState_Float4x4(D3DTS::VIEW))))
        CRASH("Failed Bind Matrix");

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_TransformState_Float4x4(D3DTS::PROJ))))
        CRASH("Failed Proj Matrix");
}

HRESULT CAnimationActor::Ready_Components(const ANIMATION_ACTOR_DESC* pDesc)
{
    // Shader
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(m_eCurLevel), pDesc->strShaderTag,
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
    {
        CRASH("Failed Ready_ComShader");
        return E_FAIL;
    }

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), pDesc->strComputeShaderTag,
        TEXT("Com_ComputeShader"), reinterpret_cast<CComponent**>(&m_pComputeShaderCom), nullptr)))
    {
        CRASH("Failed Ready_ComShader");
        return E_FAIL;
    }

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), pDesc->strMorphComputeShaderTag,
        TEXT("Com_MorphComputeShader"), reinterpret_cast<CComponent**>(&m_pMorphComputeShaderCom), nullptr)))
    {
        CRASH("Failed Ready_ComShader");
        return E_FAIL;
    }

    // Model
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(m_eCurLevel), pDesc->strModelTag,
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
    {
        CRASH("Failed Ready Com_Model");
        return E_FAIL;
    }
        

    return S_OK;
}

HRESULT CAnimationActor::Ready_Camera()
{
	m_pSpringCamera = CSpringCamera_Edit::Create(m_pDevice, m_pContext);
	ASSERT_CRASH(m_pSpringCamera);

	CSpringCamera_Edit::CAMERA_DESC CameraDesc = {};
	CameraDesc.fSpeedPerSec = 100.f;
	CameraDesc.fRotationPerSec = XMConvertToRadians(90.f);
	CameraDesc.fFovy = XMConvertToRadians(60.f);
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 5000.f;
	CameraDesc.vEye = _float4(0.f, 200.f, -150.f, 1.f);
	CameraDesc.vAt = _float4(0.f, 0.f, 200.f, 1.f);
	CameraDesc.fMouseSensor = 0.004f;

	m_pSpringCamera->Initialize_Clone(&CameraDesc);
	
	m_pGameInstance->Add_Camera(ENUM_CLASS(m_eCurLevel), TEXT("Camera_Spring"), m_pSpringCamera);
	Safe_AddRef(m_pSpringCamera);

	m_pGameInstance->Change_MainCamera(ENUM_CLASS(m_eCurLevel), TEXT("Camera_Spring"));

	return S_OK;
}



void CAnimationActor::Render_Facial()
{
	_uint iNumMeshes = m_pModelCom->Get_NumMesh();
	for (_uint i = 0; i < iNumMeshes; i++)
	{
		// 1. 재질 기존 유지.
		if (FAILED(m_pModelCom->Bind_Materials(m_pShaderCom, "g_DiffuseTexture", i, TEXTURETYPE::DIFFUSE, 0)))
			continue;

		_bool HasNormal = { false };
		if (SUCCEEDED(m_pModelCom->Bind_Materials(m_pShaderCom, "g_NormalTexture", i, TEXTURETYPE::NORMAL, 0)))
			HasNormal = true;

		_bool HasMask = { false };
		if (SUCCEEDED(m_pModelCom->Bind_Materials(m_pShaderCom, "g_MaskTexture", i, TEXTURETYPE::MASK, 0)))
			HasMask = true;

		// 2. 뼈 행렬 (기존 유지)
		if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
			CRASH("Ready Bone Matrices Failed");
	
		if (FAILED(m_pModelCom->Bind_MorphedResult(m_pShaderCom, i, "g_MorphedVertices")))
			CRASH("Bind Morph Result Failed");

		if (FAILED(m_pShaderCom->Begin(2)))
			CRASH("Ready Shader Begin Failed");

		// Render 단위가 Mesh 단위 => Binding Buffer도 Mesh 단위로.
		if (FAILED(m_pModelCom->Render(i)))
			CRASH("Ready Render Failed");


		m_pShaderCom->UndBind_All_VS_SRV();
	}

	// 매프레임 Compute Shader에서 사용해야 하므로 바인딩 해제.

}


void CAnimationActor::Render_Default()
{
	_uint iNumMeshes = m_pModelCom->Get_NumMesh();
	for (_uint i = 0; i < iNumMeshes; i++)
	{
		// 1. 재질 기존 유지.
		if (FAILED(m_pModelCom->Bind_Materials(m_pShaderCom, "g_DiffuseTexture", i, TEXTURETYPE::DIFFUSE, 0)))
			return;
		//CRASH("Ready Diffuse Texture Failed");
		//if (FAILED(m_pModelCom->Bind_Materials(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS, 0)))
		//    return E_FAIL;


		// 2. 뼈 행렬 (기존 유지)
		if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
			CRASH("Ready Bone Matrices Failed");

		if (FAILED(m_pShaderCom->Begin(2)))
			CRASH("Ready Shader Begin Failed");

		if (FAILED(m_pModelCom->Render(i)))
			CRASH("Ready Render Failed");
	}
}



#ifdef _DEBUG

#endif

CGameObject* CAnimationActor::Clone(void* pArg)
{
    CAnimationActor* pInstance = new CAnimationActor(*this);

    if (FAILED(pInstance->Initialize_Clone(pArg)))
    {
        MSG_BOX("Clone Failed : CAnimationActor");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CAnimationActor* CAnimationActor::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CAnimationActor* pInstance = new CAnimationActor(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : CAnimationActor");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CAnimationActor::Free()
{
    CContainerObject::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
    Safe_Release(m_pComputeShaderCom);
    Safe_Release(m_pMorphComputeShaderCom);
	Safe_Release(m_pSpringCamera);

	m_ChildActors.clear();
	m_pChildActor = nullptr;

}
