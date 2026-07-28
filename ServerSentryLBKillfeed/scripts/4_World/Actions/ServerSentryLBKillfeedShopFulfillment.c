/**
	\file ServerSentryLBKillfeedShopFulfillment.c
	\brief Implements live-action support for LB Killfeed shop fulfillment.
*/

/**
	\brief Applies and removes LB Killfeed product colors safely.
*/
class ServerSentryLBKillfeedShopFulfillment
{
    /**
    	\brief Returns whether available.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the condition is satisfied; otherwise false.
    */
    static bool IsAvailable(out string message)
    {
        message = "";

        KillfeedCustomColorConfig_ config = KillfeedCustomColorConfig.Get;
        if (!config)
        {
            message = "LBmaster Killfeed player-color configuration is not available.";
            return false;
        }

        return true;
    }

    /**
    	\brief Sets player color.
    	\param steamId \p Steam ID of the target player.
    	\param red \p red RGB channel in the range 0 to 255.
    	\param green \p green RGB channel in the range 0 to 255.
    	\param blue \p blue RGB channel in the range 0 to 255.
    	\param normalizedColor \p normalized color used by the operation.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the operation succeeds; otherwise false.
    */
    static bool SetPlayerColor(string steamId, int red, int green, int blue, string normalizedColor, out string message)
    {
        message = "";

        if (steamId == "")
        {
            message = "Missing buyer SteamID64.";
            return false;
        }

        KillfeedCustomColorConfig_ config = KillfeedCustomColorConfig.Get;
        if (!config)
        {
            message = "LBmaster Killfeed player-color configuration is not available.";
            return false;
        }

        if (!config.colors)
        {
            config.colors = new array<ref KillfeedCustomColor>();
        }

        KillfeedCustomColor targetColor;

        // Keep exactly one entry per SteamID. Old manually-created duplicate
        // entries are removed so the runtime cache has deterministic data.
        for (int i = config.colors.Count() - 1; i >= 0; i--)
        {
            KillfeedCustomColor entry = config.colors.Get(i);
            if (!entry)
            {
                config.colors.Remove(i);
                continue;
            }

            if (entry.steamid != steamId)
            {
                continue;
            }

            if (!targetColor)
            {
                targetColor = entry;
                continue;
            }

            config.colors.Remove(i);
        }

        if (!targetColor)
        {
            targetColor = new KillfeedCustomColor(steamId, red, green, blue);
            config.colors.Insert(targetColor);
        }
        else
        {
            targetColor.steamid = steamId;
            targetColor.r = red;
            targetColor.g = green;
            targetColor.b = blue;
        }

        // Manual edits to playercolors.json are normally only loaded on
        // restart. The shop changes the in-memory config, rebuilds its cache,
        // and persists it, so new killfeed messages can use it immediately.
        config.OnLoad();
        KillfeedCustomColorConfig.Loader.Save();

        if (!HasExactPlayerColor(config, steamId, red, green, blue))
        {
            message = "LBmaster Killfeed did not retain color " + normalizedColor + " for SteamID " + steamId + ".";
            return false;
        }

        message = "Set LBmaster Killfeed color to " + normalizedColor + " for SteamID " + steamId + ".";
        return true;
    }

    /**
    	\brief Removes player color.
    	\param steamId \p Steam ID of the target player.
    	\param expectedRed \p expected red used by the operation.
    	\param expectedGreen \p expected green used by the operation.
    	\param expectedBlue \p expected blue used by the operation.
    	\param expectedHex \p expected hex used by the operation.
    	\param message \p receives a human-readable result or error message.
    	\return \p true when the operation succeeds; otherwise false.
    */
    static bool RemovePlayerColor(string steamId, int expectedRed, int expectedGreen, int expectedBlue, string expectedHex, out string message)
    {
        message = "";

        if (steamId == "")
        {
            message = "Missing buyer SteamID64.";
            return false;
        }

        KillfeedCustomColorConfig_ config = KillfeedCustomColorConfig.Get;
        if (!config)
        {
            message = "LBmaster Killfeed player-color configuration is not available.";
            return false;
        }

        if (!config.colors)
        {
            config.colors = new array<ref KillfeedCustomColor>();
        }

        bool foundDifferentColor = false;
        bool removedExpectedColor = false;
        bool configChanged = false;

        for (int i = config.colors.Count() - 1; i >= 0; i--)
        {
            KillfeedCustomColor entry = config.colors.Get(i);
            if (!entry)
            {
                config.colors.Remove(i);
                configChanged = true;
                continue;
            }

            if (entry.steamid != steamId)
            {
                continue;
            }

            if (entry.r == expectedRed && entry.g == expectedGreen && entry.b == expectedBlue)
            {
                config.colors.Remove(i);
                removedExpectedColor = true;
                configChanged = true;
            }
            else
            {
                foundDifferentColor = true;
            }
        }

        if (configChanged)
        {
            config.OnLoad();
            KillfeedCustomColorConfig.Loader.Save();
        }

        if (removedExpectedColor)
        {
            message = "Removed LBmaster Killfeed player color " + expectedHex + " for SteamID " + steamId + ".";
            return true;
        }

        if (foundDifferentColor)
        {
            message = "Did not remove the LBmaster Killfeed player color for SteamID " + steamId + " because the current color no longer matches product color " + expectedHex + ".";
            return true;
        }

        message = "LBmaster Killfeed player color " + expectedHex + " is already absent for SteamID " + steamId + ".";
        return true;
    }

    protected static bool HasExactPlayerColor(KillfeedCustomColorConfig_ config, string steamId, int red, int green, int blue)
    {
        if (!config || !config.colors)
        {
            return false;
        }

        int matches = 0;
        foreach (KillfeedCustomColor entry : config.colors)
        {
            if (!entry || entry.steamid != steamId)
            {
                continue;
            }

            if (entry.r != red || entry.g != green || entry.b != blue)
            {
                return false;
            }

            matches++;
        }

        return matches == 1;
    }

}
