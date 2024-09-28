enum FLINCH_KillMethod 
{
  DELETE,
  HEART_ATTACK,
  LIGHTNING,
  EXPLOSION,	
}

modded class SCR_AIGroupUtilityComponent : SCR_AIBaseUtilityComponent
{
	// Kill all remaining group members for group sizes >10 when below this threshold.
	float m_groupSizeAbove10KillThreshold = 0.10;
	
	// How should all remaining group members be killed?
	FLINCH_KillMethod m_killMethod = FLINCH_KillMethod.HEART_ATTACK;
	
	// The index of the below array represents the group size from 0-10 people.
	ref array<float> m_groupSizeKillThresholds = {
        0.0, // 0 agents in the group, kill no one
		0.0, // 1 agent in the group, kill no one
		0.0, // 2 agents in the group, kill no one
		0.34, // 3 agents in the group, kill 1 member
		0.26, // 4 agents in the group, kill 1 member
		0.21, // 5 agents in the group, kill 1 member
		0.17, // 6 agents in the group, kill 1 member
		0.15, // 7 agents in the group, kill 1 member
		0.13, // 8 agents in the group, kill 1 member
		0.12, // 9 agents in the group, kill 1 member
		0.11, // 10 agents in the group, kill 1 member
    };
	
	[Attribute("{A8FFCD3F3697D77F}Prefabs/Editor/Lightning/LightningStrike.et")]
	protected ResourceName m_lightningPrefab;

	[Attribute("{2F690C7C59FB4DBF}Prefabs/Weapons/Warheads/Explosions/Explosion_Tnt_Small.et")]
	protected ResourceName m_explosionPrefab;
	
	protected bool m_latchTriggeredForGroup = false;
	
	override void OnAgentLifeStateChanged(AIAgent incapacitatedAgent, SCR_AIInfoComponent infoIncap, IEntity vehicle, ECharacterLifeState lifeState)
	{
		// Do regular death stuff.
		super.OnAgentLifeStateChanged(incapacitatedAgent, infoIncap, vehicle, lifeState);
		
		// Check if we should cleanup the remaining agents in the group when any group member dies.
		// Note that we should only do this once per group.
		if (!m_latchTriggeredForGroup && lifeState == ECharacterLifeState.DEAD) 
		{
			TryGroupCleanup(incapacitatedAgent);
		}		
	}
	
	void TryGroupCleanup(AIAgent incapacitatedAgent)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(incapacitatedAgent.GetParentGroup());
		if (!group)
		{
			// Critial error, bail.
			return;
		}
		
		// Subtract one here as this function is evaluated before the GetAgentsCount() method is updated to return
		// the alive count, minus the newly incapacitated agent.
		int groupMembersAlive = group.GetAgentsCount() - 1;
		int groupSize = group.GetNumberOfMembersToSpawn();
		float groupMembersAliveRatio = groupMembersAlive / groupSize;
		
		if (groupMembersAliveRatio < CalculateSmartThreshold(groupSize)) 
		{
			// Latch here to prevent triggering group cleanup multiple times when killing each group member.
			m_latchTriggeredForGroup = true;
			
			// Note: we can't pass in the entire group as the newly incapacitated agent will still be in it!
			// Hence, we need to filter this newly dead agent out.
			array<AIAgent> agentsRemainingInGroup = {};
			group.GetAgents(agentsRemainingInGroup);
			agentsRemainingInGroup.RemoveItem(incapacitatedAgent);	
			KillAllAgentsInGroup(agentsRemainingInGroup);
		}		
	}
	
	float CalculateSmartThreshold(int groupSize) 
	{    
	    if (groupSize <= m_groupSizeKillThresholds.Count() - 1) {
	        return m_groupSizeKillThresholds.Get(groupSize);
	    } 
		
		// For group sizes > 10
	    return m_groupSizeAbove10KillThreshold;
	}
	
	void KillAllAgentsInGroup(array<AIAgent> agentsInGroup)
	{
		int agentsKilled = 0;
		foreach (AIAgent agent : agentsInGroup) 
		{
			agentsKilled += KillAgent(agent);
		}
		
		// If somehow we still have agents alive in the group, we need to delete them all.
		if (agentsKilled < agentsInGroup.Count()) 
		{
			// TODO: remove
			AudioSystem.PlaySound("{E89D9A1F4BA63CDC}Sounds/Props/Furniture/Piano/Samples/Props_Piano_Jingle_1.wav"); 

			// Something has gone wrong when killing the agents, so we run the fallback option here which, just 
			// deletes the remaining agents.
			foreach (AIAgent agent : agentsInGroup) 
			{
				DeleteEntity(agent.GetControlledEntity());
			}
		}
	}
	
	int KillAgent(AIAgent agent) 
	{
		IEntity controlledEntity = agent.GetControlledEntity();
		if (!controlledEntity)
		{
			return 0;
		}
		
		ChimeraCharacter character = ChimeraCharacter.Cast(controlledEntity);
		if (!character)
		{
			return 0;
		}
		
		switch (m_killMethod) {
		 	case FLINCH_KillMethod.HEART_ATTACK:
				return TryKillWithHeartAttack(character);
		  	case FLINCH_KillMethod.LIGHTNING:
				return TryKillWithLightingStrike(character);
		  	case FLINCH_KillMethod.EXPLOSION:
				return TryKillWithExplosion(character);
		  	case FLINCH_KillMethod.DELETE:
		  	default:
				return DeleteEntity(character);		
		}	
		
		return 0;
	}
	
	
	int TryKillWithHeartAttack(ChimeraCharacter character)
	{
		character.GetDamageManager().Kill(Instigator.CreateInstigator(character));
		return 1;
	}	

	int TryKillWithLightingStrike(ChimeraCharacter character)
	{				
		vector agentTransform[4];
		character.GetWorldTransform(agentTransform);
		
		Resource resource = Resource.Load(m_lightningPrefab);
		if (!resource) 
		{
			return 0;
		}
		
		// Spawn at feet of character or sea position
		EntitySpawnParams spawnParams = new EntitySpawnParams;
		spawnParams.Transform[3] = agentTransform[3];
		SCR_TerrainHelper.SnapToTerrain(spawnParams.Transform, GetGame().GetWorld(), true);
	
		// Random rotation
		vector angles = Math3D.MatrixToAngles(spawnParams.Transform);
		angles[0] = Math.RandomFloat(0, 360);	
		Math3D.AnglesToMatrix(angles, spawnParams.Transform);
		
		GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		return 1;
	}
	
	int TryKillWithExplosion(ChimeraCharacter character)
	{
		vector agentTransform[4];
		character.GetWorldTransform(agentTransform);
		
		Resource resource = Resource.Load(m_explosionPrefab);
		if (!resource) 
		{
			return 0;
		}
		
		// Spawn at feet of character or sea position
		EntitySpawnParams spawnParams = new EntitySpawnParams;
		spawnParams.Transform[3] = agentTransform[3];
		SCR_TerrainHelper.SnapToTerrain(spawnParams.Transform, GetGame().GetWorld(), true);
	
		GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		return TryKillWithHeartAttack(character);
	}	
	
	int DeleteEntity(IEntity entity)
	{
		RplComponent.DeleteRplEntity(entity, false);
		return 1;
	}	
}