/**
	\file ServerSentryLBGroupsDeleteFlagpoleRadiusItemsAction.c
	\brief Implements the ServerSentry Flagpole radius item deletion live action.
*/

/**
	\brief Deletes root world items inside the selected Flagpole's territory radius.
*/
class ServerSentryLBGroupsDeleteFlagpoleRadiusItemsAction : ServerSentryFlagPoleModActionHandler
{
    /**
    	\brief Returns the metadata descriptor published for this live action.
    	\return \p The resulting live mod action descriptor DTO instance, or null when unavailable.
    */
    override ServerSentryLiveModActionDescriptorDto GetDescriptor()
    {
        ServerSentryLiveModActionDescriptorDto descriptor = new ServerSentryLiveModActionDescriptorDto();
        descriptor.id = ServerSentryLBGroupsActionIds.FLAGPOLE_DELETE_ITEMS_RADIUS;
        descriptor.targetType = "base_flagpole";
        descriptor.label = "Delete all items in flagpole radius";
        descriptor.icon = "Trash2";
        descriptor.description = "Deletes root world items and vehicles inside the selected Flagpole's vanilla territory radius, including every Flagpole in range. Occupied vehicles are skipped only to protect their crew. Static map structures and AI are not targeted.";
        descriptor.AddScope("live");
        descriptor.SetDangerous("Delete storage, tents, fences, vehicles, Flagpoles, and all other supported world items in this radius? The selected Flagpole will also be deleted. Occupied vehicles are skipped to protect players.");
        return descriptor;
    }

    /**
    	\brief Executes the live action for the resolved world object.
    	\param ctx \p action context containing request, target, and input data.
    	\param flagPole \p selected live Flagpole.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the radius scan completed; otherwise false.
    */
    override bool ExecuteFlagPole(ServerSentryModActionContext ctx, EntityAI flagPole, out string message)
    {
        EntityAI flagpole = flagPole;
        if (!ServerSentryLBGroupsFlagpoleActionService.IsFlagpole(flagpole))
        {
            message = "The selected live object is not a placed Flagpole.";
            return false;
        }

        float radius = GameConstants.REFRESHER_RADIUS;
        vector center = flagpole.GetPosition();
        string flagpoleObjectId = flagpole.ToString();
        string flagpoleClass = flagpole.GetType();
        string reason = "Delete Flagpole radius items at " + center.ToString() + " within " + radius.ToString() + " m";
        ServerSentryShopFulfillmentOperation fulfillment;
        string operation = "live:" + ctx.actionId + ":" + ctx.requestId;
        int beginStatus = ServerSentryShopFulfillment.Begin(ctx, operation, reason, fulfillment, message, false);
        if (beginStatus == ServerSentryShopFulfillment.BEGIN_ALREADY_FULFILLED)
        {
            return true;
        }
        if (beginStatus != ServerSentryShopFulfillment.BEGIN_READY)
        {
            return false;
        }

        int deletedCount;
        int skippedOccupiedVehicleCount;
        string actionMessage;
        bool deletionSucceeded = ServerSentryLBGroupsFlagpoleActionService.DeleteItemsInRadius(center, radius, deletedCount, skippedOccupiedVehicleCount, actionMessage);
        if (!deletionSucceeded)
        {
            return ServerSentryShopFulfillment.Finish(fulfillment, false, actionMessage, message);
        }

        actionMessage = "Deleted " + deletedCount.ToString() + " root world item(s) and vehicle(s) inside the Flagpole radius of " + ServerSentryLBGroupsFlagpoleActionService.RadiusText(radius) + ", including Flagpoles.";
        if (skippedOccupiedVehicleCount > 0)
        {
            actionMessage += " Skipped " + skippedOccupiedVehicleCount.ToString() + " occupied vehicle(s) to protect their crew.";
        }
        Print("[ServerSentry][LBGroups][RadiusDelete] action=flagpole requestId=" + ctx.requestId + " requestedBy=" + ctx.requestedBy + " objectId=" + flagpoleObjectId + " class=" + flagpoleClass + " center=" + center.ToString() + " radius=" + radius.ToString() + " deletedRoots=" + deletedCount.ToString() + " skippedOccupiedVehicles=" + skippedOccupiedVehicleCount.ToString());
        return ServerSentryShopFulfillment.Finish(fulfillment, true, actionMessage, message);
    }
}
