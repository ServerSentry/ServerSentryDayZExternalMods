#ifdef EXPANSIONMODCHAT

class ServerSentryExpansionChatCapture
{
    static void Emit(PlayerIdentity sender, ExpansionChatMessageEventParams params, string originalText)
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        ServerSentryChatProviderRegistry.RegisterExternalChatProvider("DayZExpansion_Chat");

        if (!ServerSentryDefaults.LogExpansionChatEvents())
        {
            return;
        }

        if (!sender || !params)
        {
            return;
        }

        string message = ServerSentryChatUtils.NormalizeMessage(originalText);
        if (message == "")
        {
            return;
        }

        ServerSentryChatMessageData data = new ServerSentryChatMessageData();
        data.sourceMod = "DayZExpansion_Chat";
        data.sourceHook = "ExpansionGlobalChatModule.AddChatMessage_Server";
        data.result = "success";
        data.channelId = params.param1;
        data.channelName = ExpansionGlobalChatModule.GetChannelName(params.param1);
        data.channelType = ResolveExpansionChannelType(params.param1);
        data.senderSteamId = sender.GetPlainId();
        data.senderBohemiaId = sender.GetId();
        data.senderName = sender.GetName();
        data.message = message;
        data.originalMessage = message;
        data.filteredMessage = message;
        data.isCommand = ServerSentryChatUtils.IsCommand(message);
        FillFlags(data, params.param1);

#ifdef EXPANSIONMODGROUPS
        data.groupTag = params.param6;
#endif

        PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
        if (player)
        {
            data.senderPosition.FromVector(player.GetPosition());
        }

        ServerSentryApi.EmitChatMessage(data, DayZPlayer.Cast(player));
    }

    static void FillFlags(ServerSentryChatMessageData data, ExpansionChatChannels channel)
    {
        if (!data)
        {
            return;
        }

        data.isDirect = channel == ExpansionChatChannels.CCLocal || channel == ExpansionChatChannels.CCDirect || channel == ExpansionChatChannels.CCMegaphone || channel == ExpansionChatChannels.CCTransmitter || channel == ExpansionChatChannels.CCPublicAddressSystem;
        data.isGlobal = channel == ExpansionChatChannels.CCGlobal;
        data.isGroup = channel == ExpansionChatChannels.CCTeam;
        data.isVehicle = channel == ExpansionChatChannels.CCTransport;
        data.isAdmin = channel == ExpansionChatChannels.CCAdmin;
        data.isSystem = channel == ExpansionChatChannels.CCSystem || channel == ExpansionChatChannels.CCBattlEye;
    }

    static string ResolveExpansionChannelType(ExpansionChatChannels channel)
    {
        switch (channel)
        {
            case ExpansionChatChannels.CCLocal:
            case ExpansionChatChannels.CCDirect: return "direct";
            case ExpansionChatChannels.CCSystem: return "system";
            case ExpansionChatChannels.CCAdmin: return "admin";
            case ExpansionChatChannels.CCMegaphone: return "megaphone";
            case ExpansionChatChannels.CCTransmitter: return "radio";
            case ExpansionChatChannels.CCPublicAddressSystem: return "pas";
            case ExpansionChatChannels.CCBattlEye: return "battleye";
            case ExpansionChatChannels.CCTransport: return "transport";
            case ExpansionChatChannels.CCGlobal: return "global";
            case ExpansionChatChannels.CCTeam: return "team";
        }

        return "unknown";
    }
}

modded class ExpansionGlobalChatModule
{
    override void AddChatMessage_Server(PlayerIdentity sender, Object target, ParamsReadContext ctx, ExpansionChatMessageEventParams data)
    {
        string originalText = "";
        if (data)
        {
            originalText = data.param3;
        }

        // Let Expansion validate, filter, and distribute the message first.
        super.AddChatMessage_Server(sender, target, ctx, data);
        ServerSentryExpansionChatCapture.Emit(sender, data, originalText);
    }
}

#endif // EXPANSIONMODCHAT
