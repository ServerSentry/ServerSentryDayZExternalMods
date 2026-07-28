/**
	\file ServerSentryLBChatHandler.c
	\brief Connects ServerSentry to DayZ or third-party hooks for LB chat handler.
*/

#ifndef LB_DISABLE_CHAT
/**
	\brief Provides ServerSentry behavior for LB Groups chat capture.
*/
class ServerSentryLBGroupsChatCapture
{
    /**
    	\brief Emits the requested operation.
    	\param sender \p identity of the RPC sender.
    	\param originalChannelId \p identifier of the original channel.
    	\param cfg \p cfg used by the operation.
    	\param playerPrefix \p player prefix used by the operation.
    	\param message \p receives a human-readable result or error message.
    	\param uncensoredMessage \p receives a human-readable result or error description.
    */
    static void Emit(PlayerIdentity sender, int originalChannelId, ChannelCfg cfg, PrefixGroup playerPrefix, string message, string uncensoredMessage)
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        ServerSentryChatProviderRegistry.RegisterExternalChatProvider("LBmaster_Groups");

        if (!ServerSentryDefaults.LogLBMasterChatEvents())
        {
            return;
        }

        if (!sender || !cfg)
        {
            return;
        }

        string cleanMessage = ServerSentryChatUtils.NormalizeMessage(message);
        string cleanOriginal = ServerSentryChatUtils.NormalizeMessage(uncensoredMessage);
        if (cleanMessage == "")
        {
            return;
        }

        ServerSentryChatMessageData data = new ServerSentryChatMessageData();
        data.sourceMod = "LBmaster_Groups";
        data.sourceHook = "LBChatHandler.OnChatMessage";
        data.result = "success";
        data.channelId = originalChannelId;
        data.channelName = cfg.channelName;
        data.channelType = ResolveChannelType(cfg);
        data.senderSteamId = sender.GetPlainId();
        data.senderBohemiaId = sender.GetId();
        data.senderName = sender.GetName();
        data.message = cleanMessage;
        data.filteredMessage = cleanMessage;
        data.originalMessage = cleanOriginal;
        data.isCommand = ServerSentryChatUtils.IsCommand(cleanMessage);
        data.isDirect = cfg.directChannel;
        data.isGlobal = cfg.globalChannel;
        data.isGroup = cfg.groupChannel;
        data.isVehicle = false;
        data.isAdmin = cfg.writeChannelPermission != "";
        data.isSystem = !cfg.canReceiveMessagesFromPlayers;

        if (playerPrefix)
        {
            data.prefix = playerPrefix.prefix;
        }

        PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
        if (player)
        {
            data.senderPosition.FromVector(player.GetPosition());
        }

        ServerSentryApi.EmitChatMessage(data, DayZPlayer.Cast(player));
    }

    /**
    	\brief Resolves channel type.
    	\param cfg \p cfg used by the operation.
    	\return \p The resulting string value.
    */
    static string ResolveChannelType(ChannelCfg cfg)
    {
        if (!cfg)
        {
            return "unknown";
        }

        if (cfg.groupChannel && !cfg.globalChannel && !cfg.directChannel)
        {
            return "group";
        }

        if (cfg.directChannel && !cfg.globalChannel && !cfg.groupChannel)
        {
            return "direct";
        }

        if (cfg.globalChannel && !cfg.groupChannel && !cfg.directChannel)
        {
            return "global";
        }

        if (!cfg.canReceiveMessagesFromPlayers)
        {
            return "system";
        }

        return "custom";
    }
}


/**
	\brief Registers components for LB Groups chat.
*/
class ServerSentryLBGroupsChatBootstrap
{
    /**
    	\brief Registers an action handler and caches its descriptor.
    */
    static void Register()
    {
        ServerSentryChatProviderRegistry.RegisterExternalChatProvider("LBmaster_Groups");
    }
}

/**
	\brief Extends the DayZ Mission server class with ServerSentry integration.
*/
modded class MissionServer
{
    /**
    	\brief Handles the DayZ Init callback.
    */
    override void OnInit()
    {
        super.OnInit();
        ServerSentryLBGroupsChatBootstrap.Register();
    }
}

/**
	\brief Extends the DayZ LB chat handler class with ServerSentry integration.
*/
modded class LBChatHandler
{
    /**
    	\brief Handles the DayZ Chat message callback.
    	\param sender \p identity of the RPC sender.
    	\param originalChannelId \p identifier of the original channel.
    	\param cfg \p cfg used by the operation.
    	\param playerPrefix \p player prefix used by the operation.
    	\param message \p receives a human-readable result or error message.
    	\param uncensoredMessage \p receives a human-readable result or error description.
    */
    override void OnChatMessage(PlayerIdentity sender, int originalChannelId, ChannelCfg cfg, PrefixGroup playerPrefix, string message, string uncensoredMessage)
    {
        // Preserve LBmaster filtering, permission checks, and delivery first.
        super.OnChatMessage(sender, originalChannelId, cfg, playerPrefix, message, uncensoredMessage);
        ServerSentryLBGroupsChatCapture.Emit(sender, originalChannelId, cfg, playerPrefix, message, uncensoredMessage);
    }
}
#endif
