/**
	\file ServerSentryLBGroupsFlagpoleActionService.c
	\brief Provides shared Flagpole state and radius-item operations.
*/

/**
	\brief Resolves and manipulates vanilla DayZ territory Flagpoles.
*/
class ServerSentryLBGroupsFlagpoleActionService
{
    static const float MIN_DELETE_RADIUS = 1.0;
    static const float MAX_DELETE_RADIUS = 500.0;

    /**
    	\brief Resolves a selected live object as a territory Flagpole.
    	\param targetObject \p selected live object.
    	\param flagpole \p receives the resolved Flagpole.
    	\param message \p receives a human-readable error message.
    	\return \p true when a Flagpole was resolved; otherwise false.
    */
    static bool TryResolveFlagpole(EntityAI targetObject, out TerritoryFlag flagpole, out string message)
    {
        flagpole = TerritoryFlag.Cast(targetObject);
        if (!flagpole)
        {
            message = "The selected live object is not a DayZ territory Flagpole.";
            return false;
        }

        message = "";
        return true;
    }

    /**
    	\brief Validates that a Flagpole has a flag attached.
    	\param flagpole \p Flagpole to validate.
    	\param message \p receives a human-readable error message.
    	\return \p true when a flag is attached; otherwise false.
    */
    static bool RequireAttachedFlag(TerritoryFlag flagpole, out string message)
    {
        if (!flagpole)
        {
            message = "Flagpole is not available.";
            return false;
        }

        if (!flagpole.FindAttachmentBySlotName("Material_FPole_Flag"))
        {
            message = "The selected Flagpole has no flag attached.";
            return false;
        }

        message = "";
        return true;
    }

    /**
    	\brief Raises a territory flag fully and restores its refresher lifetime.
    	\param flagpole \p Flagpole to manipulate.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the requested state was applied; otherwise false.
    */
    static bool RaiseFlag(TerritoryFlag flagpole, out string message)
    {
        if (!RequireAttachedFlag(flagpole, message))
        {
            return false;
        }

        bool alreadyRaised = flagpole.GetAnimationPhase("flag_mast") <= 0.001;
        flagpole.AnimateFlagEx(0);
        flagpole.AddRefresherTime01(1);

        if (alreadyRaised)
        {
            message = "Flag was already fully raised; its territory refresher lifetime was restored.";
        }
        else
        {
            message = "Raised the flag and restored its territory refresher lifetime.";
        }
        return true;
    }

    /**
    	\brief Lowers a territory flag fully and disables its refresher lifetime.
    	\param flagpole \p Flagpole to manipulate.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the requested state was applied; otherwise false.
    */
    static bool LowerFlag(TerritoryFlag flagpole, out string message)
    {
        if (!RequireAttachedFlag(flagpole, message))
        {
            return false;
        }

        bool alreadyLowered = flagpole.GetAnimationPhase("flag_mast") >= 0.999;
        flagpole.AnimateFlagEx(1);
        flagpole.AddRefresherTime01(-1);

        if (alreadyLowered)
        {
            message = "Flag was already fully lowered; its territory refresher is disabled.";
        }
        else
        {
            message = "Lowered the flag and disabled its territory refresher.";
        }
        return true;
    }

    /**
    	\brief Deletes supported root world entities inside a radius, including vehicles and Flagpoles, while protecting players.
    	\param center \p center of the deletion radius.
    	\param radius \p radius in meters.
    	\param deletedCount \p receives the number of root entities submitted for deletion.
    	\param skippedOccupiedVehicleCount \p receives the number of occupied vehicles skipped to protect their crew.
    	\param message \p receives a human-readable error message.
    	\return \p true when the deletion scan completed; otherwise false.
    */
    static bool DeleteItemsInRadius(vector center, float radius, out int deletedCount, out int skippedOccupiedVehicleCount, out string message)
    {
        deletedCount = 0;
        skippedOccupiedVehicleCount = 0;
        message = "";

        if (!GetGame() || !GetGame().IsServer())
        {
            message = "Radius item deletion is available only on the server.";
            return false;
        }

        if (!ValidateDeleteRadius(radius, message))
        {
            return false;
        }

        array<Object> objects = new array<Object>();
        array<CargoBase> proxyCargos = new array<CargoBase>();
        GetGame().GetObjectsAtPosition(center, radius, objects, proxyCargos);

        set<EntityAI> rootEntities = new set<EntityAI>();
        set<Transport> occupiedVehicles = new set<Transport>();
        foreach (Object candidateObject : objects)
        {
            EntityAI candidateEntity = EntityAI.Cast(candidateObject);
            if (!candidateEntity || candidateEntity.IsSetForDeletion())
            {
                continue;
            }

            if (candidateEntity.GetHierarchyRootPlayer())
            {
                continue;
            }

            EntityAI rootEntity = candidateEntity.GetHierarchyRoot();
            if (!rootEntity)
            {
                rootEntity = candidateEntity;
            }

            if (rootEntity.IsSetForDeletion() || rootEntity.GetHierarchyRootPlayer() || PlayerBase.Cast(rootEntity))
            {
                continue;
            }

            if (!ItemBase.Cast(rootEntity) && !Transport.Cast(rootEntity) && !IsFlagpole(rootEntity))
            {
                continue;
            }

            if (HorizontalDistance(rootEntity.GetPosition(), center) > radius)
            {
                continue;
            }

            Transport rootTransport = Transport.Cast(rootEntity);
            if (rootTransport && HasCrew(rootTransport))
            {
                occupiedVehicles.Insert(rootTransport);
                continue;
            }

            rootEntities.Insert(rootEntity);
        }

        foreach (EntityAI rootEntityToDelete : rootEntities)
        {
            if (!rootEntityToDelete || rootEntityToDelete.IsSetForDeletion())
            {
                continue;
            }

            rootEntityToDelete.DeleteSafe();
            deletedCount++;
        }

        skippedOccupiedVehicleCount = occupiedVehicles.Count();
        return true;
    }

    /**
    	\brief Returns whether a vehicle currently contains at least one crew member.
    	\param vehicle \p vehicle to inspect.
    	\return \p true when any crew seat is occupied; otherwise false.
    */
    static bool HasCrew(Transport vehicle)
    {
        if (!vehicle)
        {
            return false;
        }

        for (int crewIndex = 0; crewIndex < vehicle.CrewSize(); crewIndex++)
        {
            if (vehicle.CrewMember(crewIndex))
            {
                return true;
            }
        }

        return false;
    }

    /**
    	\brief Identifies every placed Flagpole class recognized by ServerSentry's live-map classifier.
    	\param entity \p entity to inspect.
    	\return \p true when the entity represents a placed Flagpole; otherwise false.
    */
    static bool IsFlagpole(EntityAI entity)
    {
        if (!entity)
        {
            return false;
        }

        if (TerritoryFlag.Cast(entity))
        {
            return true;
        }

        string className = entity.GetType();
        if (className == "" || ContainsCI(className, "Kit") || ContainsCI(className, "Hologram") || ContainsCI(className, "Placing") || ContainsCI(className, "Blueprint") || ContainsCI(className, "Packed") || ContainsCI(className, "Projection"))
        {
            return false;
        }

        if (className == "FlagPole")
        {
            return true;
        }

        if (ContainsCI(className, "TerritoryFlag") || ContainsCI(className, "FlagPole") || ContainsCI(className, "Flag_Pole") || ContainsCI(className, "BaseFlag"))
        {
            return true;
        }

        return ContainsCI(className, "Flag") && ContainsCI(className, "Territory");
    }

    /**
    	\brief Validates a requested radius before reserving a destructive operation.
    	\param radius \p radius in meters.
    	\param message \p receives a human-readable error message.
    	\return \p true when the radius is accepted; otherwise false.
    */
    static bool ValidateDeleteRadius(float radius, out string message)
    {
        if (radius < MIN_DELETE_RADIUS || radius > MAX_DELETE_RADIUS)
        {
            message = "Radius must be between 1 and 500 meters.";
            return false;
        }

        message = "";
        return true;
    }

    /**
    	\brief Formats a radius value for action result messages.
    	\param radius \p radius in meters.
    	\return \p rounded radius text.
    */
    static string RadiusText(float radius)
    {
        return Math.Round(radius).ToString() + " m";
    }

    /**
    	\brief Calculates horizontal distance for a live-map radius.
    	\param first \p first world position.
    	\param second \p second world position.
    	\return \p horizontal distance in meters.
    */
    static float HorizontalDistance(vector first, vector second)
    {
        first[1] = 0;
        second[1] = 0;
        return vector.Distance(first, second);
    }

    /**
    	\brief Performs a case-insensitive substring check.
    	\param text \p text to inspect.
    	\param needle \p substring to locate.
    	\return \p true when the substring occurs; otherwise false.
    */
    static bool ContainsCI(string text, string needle)
    {
        if (text == "" || needle == "")
        {
            return false;
        }

        string normalizedText = text;
        string normalizedNeedle = needle;
        normalizedText.ToLower();
        normalizedNeedle.ToLower();
        return normalizedText.IndexOf(normalizedNeedle) != -1;
    }
}
