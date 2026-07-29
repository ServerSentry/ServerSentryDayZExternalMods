/**
	\file ServerSentryLBGroupsLowerFlagAction.c
	\brief Implements the ServerSentry Lower flag live action.
*/

/**
	\brief Lowers the selected DayZ territory flag fully.
*/
class ServerSentryLBGroupsLowerFlagAction : ServerSentryFlagPoleModActionHandler
{
    /**
    	\brief Returns the metadata descriptor published for this live action.
    	\return \p The resulting live mod action descriptor DTO instance, or null when unavailable.
    */
    override ServerSentryLiveModActionDescriptorDto GetDescriptor()
    {
        ServerSentryLiveModActionDescriptorDto descriptor = new ServerSentryLiveModActionDescriptorDto();
        descriptor.id = ServerSentryLBGroupsActionIds.FLAGPOLE_LOWER;
        descriptor.targetType = "base_flagpole";
        descriptor.label = "Lower flag";
        descriptor.icon = "FlagOff";
        descriptor.description = "Lowers the selected DayZ territory flag fully and disables its lifetime refresher.";
        descriptor.AddScope("live");
        descriptor.SetDangerous("Lower this flag and disable its territory lifetime refresher?");
        return descriptor;
    }

    /**
    	\brief Executes the live action for the resolved world object.
    	\param ctx \p action context containing request, target, and input data.
    	\param flagPole \p selected live Flagpole.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the flag was lowered; otherwise false.
    */
    override bool ExecuteFlagPole(ServerSentryModActionContext ctx, EntityAI flagPole, out string message)
    {
        TerritoryFlag flagpole;
        if (!ServerSentryLBGroupsFlagpoleActionService.TryResolveFlagpole(flagPole, flagpole, message))
        {
            return false;
        }

        bool succeeded = ServerSentryLBGroupsFlagpoleActionService.LowerFlag(flagpole, message);
        if (succeeded)
        {
            Print("[ServerSentry][LBGroups][Flagpole] action=lower requestId=" + ctx.requestId + " requestedBy=" + ctx.requestedBy + " objectId=" + flagpole.ToString() + " class=" + flagpole.GetType() + " position=" + flagpole.GetPosition().ToString());
        }
        return succeeded;
    }
}
