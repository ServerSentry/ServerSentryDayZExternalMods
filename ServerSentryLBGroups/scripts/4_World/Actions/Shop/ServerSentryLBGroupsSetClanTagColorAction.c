/**
	\file ServerSentryLBGroupsSetClanTagColorAction.c
	\brief Implements shop fulfillment support for LB Groups set clan tag color action.
*/

/**
	\brief Represents parsed input for LB Groups clan tag color.
*/
class ServerSentryLBGroupsClanTagColorInput
{
    /**
    	\brief Stores the hex color.
    */
    string hexColor;
    /**
    	\brief Indicates whether the action removes a product effect.
    */
    bool removeProduct;

    /**
    	\brief Initializes a new ServerSentryLBGroupsClanTagColorInput instance.
    */
    void ServerSentryLBGroupsClanTagColorInput()
    {
        hexColor = "";
        removeProduct = false;
    }
}

/**
	\brief Assigns or removes an LB Groups clan-tag color for a shop product.
*/
class ServerSentryLBGroupsSetClanTagColorAction : ServerSentryModActionHandler
{
    /**
    	\brief Returns the metadata descriptor published for this live action.
    	\return \p The resulting live mod action descriptor DTO instance, or null when unavailable.
    */
    override ServerSentryLiveModActionDescriptorDto GetDescriptor()
    {
        string descriptorDescription = "Changes or removes the color of the buyer's current LB Groups clan tag.";
        ServerSentryLiveModActionDescriptorDto descriptor = ServerSentryLiveModActionDescriptorFactory.ShopPlayerLive("shop.player.group.clanTag.color", "LB Groups: Clan Tag Color");
        descriptor.icon = "Palette";
        descriptor.description = descriptorDescription;

        string colorHelp = "Choose the clan-tag color with the website color picker.";
        ServerSentryLiveModActionInputDto colorInput = ServerSentryLiveModActionInputFactory.Color("hexColor", "Clan Tag Color");
        colorInput.required = true;
        colorInput.placeholder = "#FF0000";
        colorInput.defaultValue = "#FF0000";
        colorInput.helpText = colorHelp;
        descriptor.AddInput(colorInput);

        string removeHelp = "Removes this custom clan-tag color instead of applying it. A different color set later is not overwritten.";
        ServerSentryLiveModActionInputDto removeInput = ServerSentryLiveModActionInputFactory.RemoveProduct();
        removeInput.label = "Remove Product Effect";
        removeInput.helpText = removeHelp;
        descriptor.AddInput(removeInput);

        return descriptor;
    }

    /**
    	\brief Executes the live action using the supplied request context.
    	\param ctx \p action context containing request, target, and input data.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the operation succeeds; otherwise false.
    */
    override bool Execute(ServerSentryModActionContext ctx, out string message)
    {
        if (!ctx)
        {
            message = "Missing action context.";
            return false;
        }

        ServerSentryLBGroupsClanTagColorInput input;
        string inputError;
        if (!JsonFileLoader<ServerSentryLBGroupsClanTagColorInput>.LoadData(ctx.inputJson, input, inputError))
        {
            message = "Invalid clan-tag color input JSON: " + inputError;
            return false;
        }

        if (!input)
        {
            message = "Clan-tag color input is missing.";
            return false;
        }

        bool removeProduct = input.removeProduct;
        string hexColor = input.hexColor;
        ServerSentryRgbColor color;
        string parseMessage;
        if (!ServerSentryLiveActionInput.TryParseHexColorValue(hexColor, color, parseMessage))
        {
            message = parseMessage;
            return false;
        }

        string reason = "Remove LB Groups clan tag color " + color.hex + " for buyer " + ctx.steamId;
        if (!removeProduct)
        {
            reason = "LB Groups clan tag color " + color.hex + " for buyer " + ctx.steamId;
        }

        ServerSentryShopFulfillmentOperation fulfillment;
        int beginStatus = ServerSentryShopFulfillment.BeginProductOperation(ctx, removeProduct, reason, fulfillment, message);
        if (beginStatus == ServerSentryShopFulfillment.BEGIN_ALREADY_FULFILLED)
        {
            return true;
        }
        if (beginStatus != ServerSentryShopFulfillment.BEGIN_READY)
        {
            return false;
        }

        LBGroup group = ServerSentryLBGroupsShopFulfillment.FindPersistentGroup(ctx.steamId);
        if (!group)
        {
            return ServerSentryShopFulfillment.Finish(fulfillment, false, "Buyer " + ctx.steamId + " is not a member of an LB Groups group.", message);
        }

        if (!group.GetMemberBySteamid(ctx.steamId))
        {
            return ServerSentryShopFulfillment.Finish(fulfillment, false, "Buyer " + ctx.steamId + " was not found in the resolved LB Groups member list.", message);
        }

        string actionMessage;
        bool actionSucceeded = true;
        if (removeProduct)
        {
            if (!group.hasCustomChatColor)
            {
                actionMessage = "LB Groups clan tag color is already absent for group [" + group.shortname + "].";
            }
            else if (group.chatColorR != color.red || group.chatColorG != color.green || group.chatColorB != color.blue)
            {
                actionMessage = "Did not remove LB Groups clan tag color for group [" + group.shortname + "] because the current color no longer matches product color " + color.hex + ".";
            }
            else
            {
                group.ChangeTagColor(false, group.chatColorR, group.chatColorG, group.chatColorB, false);
                actionSucceeded = !group.hasCustomChatColor;
                if (actionSucceeded)
                {
                    actionMessage = "Removed LB Groups clan tag color " + color.hex + " from group [" + group.shortname + "] for buyer " + ctx.steamId + ".";
                }
                else
                {
                    actionMessage = "LB Groups did not clear clan tag color " + color.hex + " for group [" + group.shortname + "].";
                }
            }
        }
        else
        {
            group.ChangeTagColor(true, color.red, color.green, color.blue, false);
            actionSucceeded = group.hasCustomChatColor && group.chatColorR == color.red && group.chatColorG == color.green && group.chatColorB == color.blue;
            if (actionSucceeded)
            {
                actionMessage = "Set LB Groups clan tag color for group [" + group.shortname + "] to " + color.hex + " for buyer " + ctx.steamId + ".";
            }
            else
            {
                actionMessage = "LB Groups did not apply clan tag color " + color.hex + " to group [" + group.shortname + "].";
            }
        }

        return ServerSentryShopFulfillment.Finish(fulfillment, actionSucceeded, actionMessage, message);
    }
}
