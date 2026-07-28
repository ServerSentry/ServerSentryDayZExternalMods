/**
	\file ServerSentryLBGroupsAssignChatTagAction.c
	\brief Implements shop fulfillment support for LB Groups assign chat tag action.
*/

#ifndef LB_DISABLE_CHAT
/**
	\brief Represents parsed input for LB Groups chat tag.
*/
class ServerSentryLBGroupsChatTagInput
{
    /**
    	\brief Stores the chat tag text.
    */
    string chatTagText;
    /**
    	\brief Stores the chat tag color.
    */
    string chatTagColor;
    /**
    	\brief Indicates whether the action removes a product effect.
    */
    bool removeProduct;

    /**
    	\brief Initializes a new ServerSentryLBGroupsChatTagInput instance.
    */
    void ServerSentryLBGroupsChatTagInput()
    {
        chatTagText = "";
        chatTagColor = "";
        removeProduct = false;
    }
}

/**
	\brief Assigns or removes an LB Groups chat prefix for a shop product.
*/
class ServerSentryLBGroupsAssignChatTagAction : ServerSentryModActionHandler
{
    /**
    	\brief Returns the metadata descriptor published for this live action.
    	\return \p The resulting live mod action descriptor DTO instance, or null when unavailable.
    */
    override ServerSentryLiveModActionDescriptorDto GetDescriptor()
    {
        string descriptorDescription = "Creates, assigns, or removes a normal LB Groups chat prefix using the text and color configured on the shop product.";
        ServerSentryLiveModActionDescriptorDto descriptor = ServerSentryLiveModActionDescriptorFactory.ShopPlayerLive("shop.player.chatTag.assign", "LB Groups: Chat Tag");
        descriptor.icon = "Tags";
        descriptor.description = descriptorDescription;

        string textHelp = "Text shown before the player's name. A separating space is added automatically.";
        ServerSentryLiveModActionInputDto textInput = ServerSentryLiveModActionInputFactory.Text("chatTagText", "Chat Tag Text");
        textInput.required = true;
        textInput.placeholder = "[VIP]";
        textInput.helpText = textHelp;
        descriptor.AddInput(textInput);

        string colorHelp = "Choose the fixed color for this shop product with the website color picker.";
        ServerSentryLiveModActionInputDto colorInput = ServerSentryLiveModActionInputFactory.Color("chatTagColor", "Chat Tag Color");
        colorInput.required = true;
        colorInput.placeholder = "#FFD700";
        colorInput.defaultValue = "#FFD700";
        colorInput.helpText = colorHelp;
        descriptor.AddInput(colorInput);

        string removeHelp = "Removes this exact product prefix from the player instead of assigning it. Other prefixes are not removed.";
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

        ServerSentryLBGroupsChatTagInput input;
        string inputError;
        if (!JsonFileLoader<ServerSentryLBGroupsChatTagInput>.LoadData(ctx.inputJson, input, inputError))
        {
            message = "Invalid chat-tag input JSON: " + inputError;
            return false;
        }

        if (!input)
        {
            message = "Chat-tag input is missing.";
            return false;
        }

        bool removeProduct = input.removeProduct;
        string chatTagText = input.chatTagText;
        string chatTagColor = input.chatTagColor;

        string normalizedText;
        string validationMessage;
        if (!ServerSentryLBGroupsShopFulfillment.NormalizeChatTagText(chatTagText, normalizedText, validationMessage))
        {
            message = validationMessage;
            return false;
        }

        ServerSentryRgbColor color;
        if (!ServerSentryLiveActionInput.TryParseHexColorValue(chatTagColor, color, validationMessage))
        {
            message = validationMessage;
            return false;
        }

        string reasonText = normalizedText;
        reasonText.TrimInPlace();
        string reason = "LB Groups chat tag " + reasonText + " " + color.hex;
        if (removeProduct)
        {
            reason = "Remove " + reason;
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

        string actionMessage;
        bool actionSucceeded;
        if (removeProduct)
        {
            actionSucceeded = ServerSentryLBGroupsShopFulfillment.RemoveProductChatTag(ctx.steamId, chatTagText, color.red, color.green, color.blue, color.hex, actionMessage);
        }
        else
        {
            actionSucceeded = ServerSentryLBGroupsShopFulfillment.AssignProductChatTag(ctx.steamId, chatTagText, color.red, color.green, color.blue, color.hex, actionMessage);
        }

        return ServerSentryShopFulfillment.Finish(fulfillment, actionSucceeded, actionMessage, message);
    }
}
#endif
