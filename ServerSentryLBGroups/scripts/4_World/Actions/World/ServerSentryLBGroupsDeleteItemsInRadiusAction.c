/**
	\file ServerSentryLBGroupsDeleteItemsInRadiusAction.c
	\brief Implements the ServerSentry world-context radius item deletion action.
*/

/**
	\brief Deletes root world items inside an administrator-selected map radius.
*/
class ServerSentryLBGroupsDeleteItemsInRadiusAction : ServerSentryContextMenuModActionHandler
{
    /**
    	\brief Returns the metadata descriptor published for this live action.
    	\return \p The resulting live mod action descriptor DTO instance, or null when unavailable.
    */
    override ServerSentryLiveModActionDescriptorDto GetDescriptor()
    {
        ServerSentryLiveModActionDescriptorDto descriptor = new ServerSentryLiveModActionDescriptorDto();
        descriptor.id = ServerSentryLBGroupsActionIds.WORLD_DELETE_ITEMS_RADIUS;
        descriptor.targetType = "contextMenu";
        descriptor.label = "Delete Items in radius";
        descriptor.icon = "Trash2";
        descriptor.description = "Deletes root world items and vehicles around the selected map position, including storage, tents, fences, and Flagpoles. Occupied vehicles are skipped only to protect their crew. Static map structures and AI are not targeted.";
        descriptor.AddScope("live");
        descriptor.SetDangerous("Delete storage, tents, fences, vehicles, Flagpoles, and all other supported world items in the selected radius? Occupied vehicles are skipped to protect players.");

        string radiusHelp = "Deletion radius in meters. The server accepts values from 1 to 500. Large radii in dense bases can briefly affect server performance.";
        ServerSentryLiveModActionInputDto radiusInput = ServerSentryLiveModActionInputFactory.Number("radius", "Radius", true, 1, 500, 1, "60", radiusHelp);
        descriptor.AddInput(radiusInput);

        return descriptor;
    }

    /**
    	\brief Executes the world-context radius deletion action.
    	\param ctx \p action context containing request, target, and input data.
    	\param targetPosition \p terrain-snapped position selected on the Live Map.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the radius scan completed; otherwise false.
    */
    override bool ExecuteAtPosition(ServerSentryModActionContext ctx, vector targetPosition, out string message)
    {
        if (!ctx)
        {
            message = "Missing action context.";
            return false;
        }

        float radius = ctx.GetFloat("radius", GameConstants.REFRESHER_RADIUS);

        if (!ServerSentryLBGroupsFlagpoleActionService.ValidateDeleteRadius(radius, message))
        {
            return false;
        }

        vector center = targetPosition;
        string reason = "Delete world items at " + center.ToString() + " within " + radius.ToString() + " m";
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

        actionMessage = "Deleted " + deletedCount.ToString() + " root world item(s) and vehicle(s) within " + ServerSentryLBGroupsFlagpoleActionService.RadiusText(radius) + " of X " + Math.Round(center[0]).ToString() + " / Z " + Math.Round(center[2]).ToString() + ", including Flagpoles.";
        if (skippedOccupiedVehicleCount > 0)
        {
            actionMessage += " Skipped " + skippedOccupiedVehicleCount.ToString() + " occupied vehicle(s) to protect their crew.";
        }
        Print("[ServerSentry][LBGroups][RadiusDelete] action=world requestId=" + ctx.requestId + " requestedBy=" + ctx.requestedBy + " center=" + center.ToString() + " radius=" + radius.ToString() + " deletedRoots=" + deletedCount.ToString() + " skippedOccupiedVehicles=" + skippedOccupiedVehicleCount.ToString());
        return ServerSentryShopFulfillment.Finish(fulfillment, true, actionMessage, message);
    }
}
