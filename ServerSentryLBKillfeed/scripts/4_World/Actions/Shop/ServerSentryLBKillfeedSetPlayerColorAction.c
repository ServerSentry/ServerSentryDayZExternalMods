/**
	\file ServerSentryLBKillfeedSetPlayerColorAction.c
	\brief Implements shop fulfillment support for LB Killfeed set player color action.
*/

/**
	\brief Represents parsed input for LB Killfeed player color.
*/
class ServerSentryLBKillfeedPlayerColorInput
{
    /**
    	\brief Stores the killfeed color.
    */
    string killfeedColor;
    /**
    	\brief Indicates whether the action removes a product effect.
    */
    bool removeProduct;

    /**
    	\brief Initializes a new ServerSentryLBKillfeedPlayerColorInput instance.
    */
    void ServerSentryLBKillfeedPlayerColorInput()
    {
        killfeedColor = "";
        removeProduct = false;
    }
}

/**
	\brief Assigns or removes an LB Killfeed player color for a shop product.
*/
class ServerSentryLBKillfeedSetPlayerColorAction : ServerSentryModActionHandler
{
    /**
    	\brief Returns the metadata descriptor published for this live action.
    	\return \p The resulting live mod action descriptor DTO instance, or null when unavailable.
    */
    override ServerSentryLiveModActionDescriptorDto GetDescriptor()
    {
        string descriptorDescription = "Sets or removes the buyer's individual LBmaster Killfeed name color.";
        ServerSentryLiveModActionDescriptorDto descriptor = ServerSentryLiveModActionDescriptorFactory.ShopPlayerLive("shop.player.killfeed.color", "LB Killfeed: Player Color");
        descriptor.icon = "Palette";
        descriptor.description = descriptorDescription;

        string colorHelp = "Choose the player's individual killfeed color with the website color picker.";
        ServerSentryLiveModActionInputDto colorInput = ServerSentryLiveModActionInputFactory.Color("killfeedColor", "Killfeed Color");
        colorInput.required = true;
        colorInput.placeholder = "#00FF00";
        colorInput.defaultValue = "#00FF00";
        colorInput.helpText = colorHelp;
        descriptor.AddInput(colorInput);

        string removeHelp = "Removes this product color only when it is still the player's current killfeed color.";
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

        ServerSentryLBKillfeedPlayerColorInput input;
        string inputError;
        if (!JsonFileLoader<ServerSentryLBKillfeedPlayerColorInput>.LoadData(ctx.inputJson, input, inputError))
        {
            message = "Invalid killfeed color input JSON: " + inputError;
            return false;
        }

        if (!input)
        {
            message = "Killfeed color input is missing.";
            return false;
        }

        bool removeProduct = input.removeProduct;
        string killfeedColor = input.killfeedColor;
        ServerSentryRgbColor color;
        string validationMessage;
        if (!ServerSentryLiveActionInput.TryParseHexColorValue(killfeedColor, color, validationMessage))
        {
            message = validationMessage;
            return false;
        }

        string reason = "Remove LBmaster Killfeed player color " + color.hex;
        if (!removeProduct)
        {
            reason = "LBmaster Killfeed player color " + color.hex;
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

        if (!ServerSentryLBKillfeedShopFulfillment.IsAvailable(validationMessage))
        {
            return ServerSentryShopFulfillment.Finish(fulfillment, false, validationMessage, message);
        }

        string actionMessage;
        bool actionSucceeded;
        if (removeProduct)
        {
            actionSucceeded = ServerSentryLBKillfeedShopFulfillment.RemovePlayerColor(ctx.steamId, color.red, color.green, color.blue, color.hex, actionMessage);
        }
        else
        {
            actionSucceeded = ServerSentryLBKillfeedShopFulfillment.SetPlayerColor(ctx.steamId, color.red, color.green, color.blue, color.hex, actionMessage);
        }

        return ServerSentryShopFulfillment.Finish(fulfillment, actionSucceeded, actionMessage, message);
    }
}
