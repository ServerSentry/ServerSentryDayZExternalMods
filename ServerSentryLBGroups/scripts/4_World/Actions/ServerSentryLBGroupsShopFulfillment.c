/**
	\file ServerSentryLBGroupsShopFulfillment.c
	\brief Implements live-action support for LB Groups shop fulfillment.
*/

/**
	\brief Applies and removes LB Groups product effects safely.
*/
class ServerSentryLBGroupsShopFulfillment
{
    /**
    	\brief Finds persistent group.
    	\param steamId \p Steam ID of the target player.
    	\return \p The resulting LB group instance, or null when unavailable.
    */
    static LBGroup FindPersistentGroup(string steamId)
    {
        if (steamId == "")
        {
            return null;
        }

        LBGroupManager manager = LBGroupManager.Get();
        if (!manager)
        {
            return null;
        }

        LBGroup group = manager.GetPlayersOriginalGroup(steamId);
        if (!group)
        {
            group = manager.GetPlayersGroup(steamId);
        }

        return group;
    }

#ifndef LB_DISABLE_CHAT
    /**
    	\brief Normalizes chat tag text.
    	\param rawText \p raw text used by the operation.
    	\param normalizedText \p receives the normalized text used by the operation.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the operation succeeds; otherwise false.
    */
    static bool NormalizeChatTagText(string rawText, out string normalizedText, out string message)
    {
        normalizedText = rawText;
        normalizedText.TrimInPlace();
        message = "";

        if (normalizedText == "")
        {
            message = "Chat tag text must not be empty.";
            return false;
        }

        if (normalizedText.Length() > 48)
        {
            message = "Chat tag text is too long. Maximum length is 48 characters.";
            return false;
        }

        if (normalizedText.IndexOf("\n") != -1 || normalizedText.IndexOf("\r") != -1 || normalizedText.IndexOf("\t") != -1)
        {
            message = "Chat tag text must not contain line breaks or tab characters.";
            return false;
        }

        // LB Groups recommends a trailing space so the prefix and player name
        // are visually separated. Product creators enter only the visible text.
        normalizedText = normalizedText + " ";
        return true;
    }

    /**
    	\brief Finds exact chat prefix.
    	\param normalizedText \p normalized text used by the operation.
    	\param red \p red RGB channel in the range 0 to 255.
    	\param green \p green RGB channel in the range 0 to 255.
    	\param blue \p blue RGB channel in the range 0 to 255.
    	\return \p The resulting prefix group instance, or null when unavailable.
    */
    static PrefixGroup FindExactChatPrefix(string normalizedText, int red, int green, int blue)
    {
        ChatConfig_ config = ChatConfig.Get;
        if (!config || !config.prefixGroups)
        {
            return null;
        }

        foreach (PrefixGroup prefixGroup : config.prefixGroups)
        {
            if (!prefixGroup)
            {
                continue;
            }

            if (prefixGroup.prefix == normalizedText && prefixGroup.colorR == red && prefixGroup.colorG == green && prefixGroup.colorB == blue)
            {
                return prefixGroup;
            }
        }

        return null;
    }

    /**
    	\brief Processes the assign product chat tag operation.
    	\param steamId \p Steam ID of the target player.
    	\param rawText \p raw text used by the operation.
    	\param red \p red RGB channel in the range 0 to 255.
    	\param green \p green RGB channel in the range 0 to 255.
    	\param blue \p blue RGB channel in the range 0 to 255.
    	\param normalizedColor \p normalized color used by the operation.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the operation succeeds; otherwise false.
    */
    static bool AssignProductChatTag(string steamId, string rawText, int red, int green, int blue, string normalizedColor, out string message)
    {
        message = "";

        if (steamId == "")
        {
            message = "Missing buyer SteamID64.";
            return false;
        }

        string normalizedText;
        if (!NormalizeChatTagText(rawText, normalizedText, message))
        {
            return false;
        }

        ChatConfig_ config = ChatConfig.Get;
        if (!config)
        {
            message = "LB Groups ChatConfig is not available.";
            return false;
        }

        if (!config.prefixGroups)
        {
            config.prefixGroups = new array<ref PrefixGroup>();
        }

        PrefixGroup targetPrefix = FindExactChatPrefix(normalizedText, red, green, blue);
        bool changed = false;

        if (!targetPrefix)
        {
            targetPrefix = PrefixGroup.Init(normalizedText, red, green, blue, "");
            config.prefixGroups.Insert(targetPrefix);
            changed = true;
        }

        // A player can only have one effective direct PrefixGroup because LB
        // Groups returns the first matching group. Remove direct memberships
        // first so the purchased tag becomes active deterministically.
        foreach (PrefixGroup prefixGroup : config.prefixGroups)
        {
            if (!prefixGroup || !prefixGroup.members)
            {
                continue;
            }

            int memberIndex = prefixGroup.members.Find(steamId);
            while (memberIndex != -1)
            {
                prefixGroup.members.Remove(memberIndex);
                changed = true;
                memberIndex = prefixGroup.members.Find(steamId);
            }
        }

        if (!targetPrefix.members)
        {
            targetPrefix.members = new TStringArray();
        }

        targetPrefix.members.Insert(steamId);
        changed = true;

        if (changed)
        {
            ChatConfig.Loader.Save();
        }

        string displayText = normalizedText;
        displayText.TrimInPlace();
        message = "Assigned LB Groups chat tag '" + displayText + "' with color " + normalizedColor + " to SteamID " + steamId + ".";
        return true;
    }

    /**
    	\brief Removes product chat tag.
    	\param steamId \p Steam ID of the target player.
    	\param rawText \p raw text used by the operation.
    	\param red \p red RGB channel in the range 0 to 255.
    	\param green \p green RGB channel in the range 0 to 255.
    	\param blue \p blue RGB channel in the range 0 to 255.
    	\param normalizedColor \p normalized color used by the operation.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the operation succeeds; otherwise false.
    */
    static bool RemoveProductChatTag(string steamId, string rawText, int red, int green, int blue, string normalizedColor, out string message)
    {
        message = "";

        if (steamId == "")
        {
            message = "Missing buyer SteamID64.";
            return false;
        }

        string normalizedText;
        if (!NormalizeChatTagText(rawText, normalizedText, message))
        {
            return false;
        }

        ChatConfig_ config = ChatConfig.Get;
        if (!config)
        {
            message = "LB Groups ChatConfig is not available.";
            return false;
        }

        PrefixGroup targetPrefix = FindExactChatPrefix(normalizedText, red, green, blue);
        if (!targetPrefix || !targetPrefix.members)
        {
            message = "LB Groups chat tag product is already absent for SteamID " + steamId + ".";
            return true;
        }

        bool changed = false;
        int memberIndex = targetPrefix.members.Find(steamId);
        while (memberIndex != -1)
        {
            targetPrefix.members.Remove(memberIndex);
            changed = true;
            memberIndex = targetPrefix.members.Find(steamId);
        }

        if (changed)
        {
            ChatConfig.Loader.Save();
        }

        string displayText = normalizedText;
        displayText.TrimInPlace();
        if (changed)
        {
            message = "Removed LB Groups chat tag '" + displayText + "' with color " + normalizedColor + " from SteamID " + steamId + ".";
        }
        else
        {
            message = "LB Groups chat tag product is already absent for SteamID " + steamId + ".";
        }
        return true;
    }
#endif
}
