

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
#include "Effekseer.ManagerImplemented.h"

#include "Effekseer.InstanceGroup.h"
#include "Effekseer.Instance.h"
#include "Effekseer.InstanceContainer.h"
#include "Effekseer.InstanceGlobal.h"

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
namespace Effekseer
{
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
InstanceGroup::InstanceGroup( Manager* manager, EffectNode* effectNode, InstanceContainer* container, InstanceGlobal* global )
	: m_manager		( (ManagerImplemented*)manager )
	, m_effectNode((EffectNodeImplemented*) effectNode)
	, m_container	( container )
	, m_global		( global )
	, m_time		( 0 )
	, IsReferencedFromInstance	( true )
	, NextUsedByInstance	( NULL )
	, NextUsedByContainer	( NULL )
{

}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
InstanceGroup::~InstanceGroup()
{
	RemoveForcibly();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void InstanceGroup::RemoveInvalidInstances()
{
	auto it = m_removingInstances.begin();

	while( it != m_removingInstances.end() )
	{
		auto instance = *it;

		if( instance->m_State == INSTANCE_STATE_ACTIVE )
		{
			assert(0);
		}
		else if( instance->m_State == INSTANCE_STATE_REMOVING )
		{
			// 削除中処理
			instance->m_State = INSTANCE_STATE_REMOVED;
			it++;
		}
		else if( instance->m_State == INSTANCE_STATE_REMOVED )
		{
			it = m_removingInstances.erase( it );

			// 削除処理
			if( instance->m_pEffectNode->GetType() == EFFECT_NODE_TYPE_ROOT )
			{
				delete instance;
			}
			else
			{
				instance->~Instance();
				m_manager->PushInstance( instance );
			}

			m_global->DecInstanceCount();
		}
	}
}
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
Instance* InstanceGroup::CreateInstance()
{
	Instance* instance = NULL;

	if( m_effectNode->GetType() == EFFECT_NODE_TYPE_ROOT )
	{
		instance = new Instance( m_manager, m_effectNode, m_container );
	}
	else
	{
		Instance* buf = m_manager->PopInstance();
		if( buf == NULL ) return NULL;
		instance = new(buf)Instance( m_manager, m_effectNode, m_container );
	}
	
	m_instances.push_back( instance );
	m_global->IncInstanceCount();
	return instance;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
Instance* InstanceGroup::GetFirst()
{
	if( m_instances.size() > 0 )
	{
		return m_instances.front();
	}
	return NULL;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
int InstanceGroup::GetInstanceCount() const
{
	return (int32_t)m_instances.size();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
int InstanceGroup::GetRemovingInstanceCount() const
{
	return (int32_t)m_removingInstances.size();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void InstanceGroup::Update( float deltaFrame, bool shown )
{
	RemoveInvalidInstances();

	// Sync
	if(true)
	{
		auto it = m_instances.begin();

		while (it != m_instances.end())
		{
			auto instance = *it;

			if (instance->m_State == INSTANCE_STATE_ACTIVE)
			{
				// 更新処理
				instance->Update(deltaFrame, shown);

				// 破棄チェック
				if (instance->m_State != INSTANCE_STATE_ACTIVE)
				{
					it = m_instances.erase(it);
					m_removingInstances.push_back(instance);
				}
				else
				{
					it++;
				}
			}
		}
	}
	else
	{
		std::array<IntrusiveList<Instance>::Iterator, 4> its_begin;
		std::array<IntrusiveList<Instance>::Iterator, 4> its_end;

		auto size = m_instances.size() / 4;
		
		auto it = m_instances.begin();

		for (int32_t tnum = 0; tnum < 4; tnum++)
		{
			its_begin[tnum] = it;

			for (int32_t i = 0; i < size; i++)
			{
				it++;
			}

			its_end[tnum] = it;
		}

		its_end[3] = m_instances.end();

		it = m_instances.begin();

		while (it != m_instances.end())
		{
			auto instance = *it;

			if (instance->m_State == INSTANCE_STATE_ACTIVE)
			{
				instance->StartToUpdateAsync(deltaFrame, shown);
			}

			it++;
		}

		auto m = (ManagerImplemented*)this->m_manager;
		for (int32_t i = 0; i < 4; i++)
		{
			auto ind = i;
			m->GetInternalThreadPool()->PushTask([&, ind] {
			
				auto it_ = its_begin[ind];

				while (it_ != its_end[ind])
				{
					auto instance = *it_;

					if (instance->m_State == INSTANCE_STATE_ACTIVE)
					{
						instance->UpdateAsync(deltaFrame, shown);
					}

					it_++;
				}
			});
		}

		m->GetInternalThreadPool()->WaitAll();
		//Sleep(10);

		it = m_instances.begin();

		while (it != m_instances.end())
		{
			auto instance = *it;

			if (instance->m_State == INSTANCE_STATE_ACTIVE)
			{
				instance->FinishToUpdateAsync(instance->m_LivingTime - deltaFrame, deltaFrame, shown);

				if (instance->m_State != INSTANCE_STATE_ACTIVE)
				{
					it = m_instances.erase(it);
					m_removingInstances.push_back(instance);
				}
				else
				{
					it++;
				}
			}
		}
	}


	m_time++;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void InstanceGroup::SetBaseMatrix( const Matrix43& mat )
{
	for (auto instance : m_instances)
	{
		if (instance->m_State == INSTANCE_STATE_ACTIVE)
		{
			Matrix43::Multiple(instance->m_GlobalMatrix43, instance->m_GlobalMatrix43, mat);
		}
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void InstanceGroup::RemoveForcibly()
{
	KillAllInstances();

	RemoveInvalidInstances();
	RemoveInvalidInstances();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void InstanceGroup::KillAllInstances()
{
	while (!m_instances.empty())
	{
		auto instance = m_instances.front();
		m_instances.pop_front();

		if (instance->GetState() == INSTANCE_STATE_ACTIVE)
		{
			instance->Kill();
			m_removingInstances.push_back(instance);
		}
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------