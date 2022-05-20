class SCR_UnflipVehicle : ScriptedUserAction
{	
	[Attribute("1")]
	protected bool m_bPreventInsideVehicle;
	
	[Attribute("0.01")]
	protected float m_fChanceToPlayAudio;
	
	[Attribute("unflip_sound.wav", UIWidgets.ResourceNamePicker, desc: "Unflip Audio")]
	private ResourceName m_UnflipAudio;	
	
	protected AudioHandle m_AudioHandle;
	protected IEntity m_Vehicle;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
	{	
		vector transform[4];
		pOwnerEntity.GetWorldTransform(transform);
		BaseWorld world = GetGame().GetWorld();

		if (!world)
			return;

		// Get surface basis
		vector surfaceBasis[4];
		if (!SCR_TerrainHelper.GetTerrainBasis(transform[3], surfaceBasis, world))
			return;

		// Set position to surface
		transform[3] = surfaceBasis[3];

		// Reset pitch and roll, but preserve yaw
		vector angles = Math3D.MatrixToAngles(transform);
		Math3D.AnglesToMatrix(Vector(angles[0], 0, 0), transform);

		// Combine surface and entity transformations
		Math3D.MatrixMultiply3(surfaceBasis, transform, transform);
		
		pOwnerEntity.SetWorldTransform(transform);	
		
		// Chance to play unflipping sound.
		RandomGenerator x = new RandomGenerator();
		if (x.RandFloat01() < m_fChanceToPlayAudio) 		
			m_AudioHandle = AudioSystem.PlaySound(m_UnflipAudio);
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		if (!user || !m_Vehicle)
			return false;

		// Prevent flipping when the player is still in the vehicle.
		ChimeraCharacter character = ChimeraCharacter.Cast(user);
		if (character.IsInVehicle() && m_bPreventInsideVehicle)
			return false;
		
		// Prevent flipping when anyone is still in the vehicle.
		if (EntityIsOccupied(m_Vehicle))
			return false;

		CompartmentAccessComponent compartmentAccess = character.GetCompartmentAccessComponent();
		if (!compartmentAccess)
			return false;
		
		// Prevent flipping when the vehicle isn't flipped (vehicle is accessible).
		if (compartmentAccess.IsTargetVehicleAccessible(m_Vehicle))
			return false;
		
		return true;
	}
	
	override bool GetActionNameScript(out string outName)
	{
		// TODO: translation table wasn't working, have to hardcode this for now :(
		//outName = "#AR-UserAction_UnflipVehicle";
		outName = "Unflip Vehicle";
		return true;
	}
	
	protected bool EntityIsOccupied(IEntity vehicleEntity)
	{
		BaseCompartmentManagerComponent compartmentManager = BaseCompartmentManagerComponent.Cast(GenericEntity.Cast(vehicleEntity).FindComponent(BaseCompartmentManagerComponent));	
		if (!compartmentManager) return false;
		
		array<BaseCompartmentSlot> compartments = new array<BaseCompartmentSlot>;
		compartmentManager.GetCompartments(compartments);
		
		foreach (BaseCompartmentSlot slot : compartments)
		{
			if (slot.GetOccupant() != null || slot.AttachedOccupant() != null)
			{
				return true;
			}
		}
		return false;
	}
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!Vehicle.Cast(pOwnerEntity))
		{
			m_Vehicle = pOwnerEntity.GetParent();
		}
		else
		{
			m_Vehicle = pOwnerEntity;
		}
	}

	void ~SCR_UnflipVehicle()
	{
		// Terminate unflipping sound.
		AudioSystem.TerminateSound(m_AudioHandle);
	}
};