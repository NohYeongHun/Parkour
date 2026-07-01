#include "ClientPch.h"
#include "GameSystem.h"
#include "MouseController.h"
#include "Parser.h"

IMPLEMENT_SINGLETON(CGameSystem)

CGameSystem::CGameSystem()
{
}

void CGameSystem::Ready_GameSystem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_pMouseController = CMouseController::Create();
	ASSERT_CRASH(m_pMouseController);

	m_pParser = CParser::Create(pDevice, pContext);
	ASSERT_CRASH(m_pParser);
}

void CGameSystem::Update(_float fTimeDelta)
{
}

void CGameSystem::Clear_Resource()
{
	Clear_TriggerCallBack();
}

void CGameSystem::Register_Mouse(CMouse* pMouse)
{
	m_pMouseController->Register_Mouse(pMouse);
}
void CGameSystem::Set_MouseFix(_bool isFix)
{
	m_pMouseController->Set_MouseFix(isFix);
}
_bool CGameSystem::IsFix()
{
	return m_pMouseController->IsFix();
}

void CGameSystem::Ready_Prototype_Map(const _char* pDataFilePath, LEVEL eLevel, const _char* pModelFilePath)
{
	return m_pParser->Ready_Prototype_Map(pDataFilePath, eLevel, pModelFilePath);
}

void CGameSystem::Clone_MapObjects(LEVEL eLevel)
{
	m_pParser->Clone_MapObjects(eLevel);
}

void CGameSystem::Clear_TriggerCallBack()
{
	for (auto& TriggerVector : m_TriggerEvents)
		TriggerVector.second.clear();
	m_TriggerEvents.clear();
}

void CGameSystem::Release_System()
{
	Safe_Release(m_pMouseController);
	Release();
}

void CGameSystem::Free()
{
	__super::Free();
}
