/**
	\file ServerSentryDrJonesTrader.c
	\brief Connects ServerSentry to DayZ or third-party hooks for Dr jones trader.
*/

#ifdef TRADER

/**
	\brief Provides ServerSentry behavior for Dr jones trader capture.
*/
class ServerSentryDrJonesTraderCapture
{
    /**
    	\brief Emits from trade log.
    	\param traderPlayer \p trader player used by the operation.
    	\param message \p receives a human-readable result or error message.
    */
    static void EmitFromTradeLog(DayZPlayerImplement traderPlayer, string message)
    {
        if (!GetGame() || !GetGame().IsServer() || !traderPlayer || message == "")
        {
            return;
        }

        string transactionType = ResolveTransactionType(message);
        if (transactionType == "")
        {
            return;
        }

        ServerSentryMarketTransactionData data = new ServerSentryMarketTransactionData();
        data.sourceMod = "Trader";
        data.transactionType = transactionType;
        data.result = "success";
        data.rawLog = message;
        data.rawTraderLog = message;
        data.rawExpansionLog = message; // Kept for older consumers that already read this field.
        data.currency = traderPlayer.m_Trader_CurrencyName;

        FillActor(data, traderPlayer);

        int traderIndex = FindNearestTraderIndex(traderPlayer);
        FillTrader(data, traderPlayer, traderIndex);

        string parsedClassName = "";
        string persistentId = "";
        if (transactionType == "buy")
        {
            parsedClassName = ParseBuyClassName(message);
        }
        else
        {
            persistentId = ParseSellPersistentId(message);
            data.persistentId = persistentId;
            parsedClassName = ResolveSellClassName(traderPlayer, persistentId, traderIndex);
        }

        if (parsedClassName != "")
        {
            data.itemClassName = parsedClassName;
            data.itemDisplayName = ResolveDisplayName(parsedClassName);
        }

        int itemIndex = FindBestItemIndex(traderPlayer, parsedClassName, persistentId, traderIndex, transactionType);
        FillConfiguredItem(data, traderPlayer, itemIndex, transactionType);

        if (data.itemDisplayName == "" && data.itemClassName != "")
        {
            data.itemDisplayName = ResolveDisplayName(data.itemClassName);
        }

        ServerSentryMarketTransactionItem line = new ServerSentryMarketTransactionItem();
        CopyConfiguredItemToLine(data, line);
        line.objectId = persistentId;
        data.items.Insert(line);

        ServerSentryApi.EmitMarketTransaction(data);
    }

    /**
    	\brief Resolves transaction type.
    	\param message \p receives a human-readable result or error message.
    	\return \p The resulting string value.
    */
    protected static string ResolveTransactionType(string message)
    {
        if (StartsWith(message, "bought "))
        {
            return "buy";
        }

        if (StartsWith(message, "sold "))
        {
            return "sell";
        }

        return "";
    }

    /**
    	\brief Processes the starts with operation.
    	\param text \p text used by the operation.
    	\param prefix \p prefix used by the operation.
    	\return \p true when the operation succeeds; otherwise false.
    */
    protected static bool StartsWith(string text, string prefix)
    {
        if (text.Length() < prefix.Length())
        {
            return false;
        }

        return text.Substring(0, prefix.Length()) == prefix;
    }

    /**
    	\brief Finds last char.
    	\param text \p text used by the operation.
    	\param token \p token used by the operation.
    	\return \p The resulting integer value or status code.
    */
    protected static int FindLastChar(string text, string token)
    {
        for (int i = text.Length() - 1; i >= 0; i--)
        {
            if (text.Get(i) == token)
            {
                return i;
            }
        }

        return -1;
    }

    /**
    	\brief Parses buy class name.
    	\param message \p receives a human-readable result or error message.
    	\return \p The resulting string value.
    */
    protected static string ParseBuyClassName(string message)
    {
        int openIdx = FindLastChar(message, "(");
        int closeIdx = FindLastChar(message, ")");
        if (openIdx < 0 || closeIdx <= openIdx)
        {
            return "";
        }

        return message.Substring(openIdx + 1, closeIdx - openIdx - 1);
    }

    /**
    	\brief Parses sell persistent ID.
    	\param message \p receives a human-readable result or error message.
    	\return \p The resulting string value.
    */
    protected static string ParseSellPersistentId(string message)
    {
        int openIdx = FindLastChar(message, "(");
        int closeIdx = FindLastChar(message, ")");
        if (openIdx < 0 || closeIdx <= openIdx)
        {
            return "";
        }

        return message.Substring(openIdx + 1, closeIdx - openIdx - 1);
    }

    /**
    	\brief Resolves sell class name.
    	\param traderPlayer \p trader player used by the operation.
    	\param persistentId \p identifier of the persistent.
    	\param traderIndex \p trader index used by the operation.
    	\return \p The resulting string value.
    */
    protected static string ResolveSellClassName(DayZPlayerImplement traderPlayer, string persistentId, int traderIndex)
    {
        int bestIndex = FindBestItemIndex(traderPlayer, "", persistentId, traderIndex, "sell");
        if (bestIndex >= 0)
        {
            return traderPlayer.m_Trader_ItemsClassnames.Get(bestIndex);
        }

        int bracketIdx = persistentId.IndexOf("[");
        if (bracketIdx > 0)
        {
            return persistentId.Substring(0, bracketIdx);
        }

        return persistentId;
    }

    /**
    	\brief Finds nearest trader index.
    	\param traderPlayer \p trader player used by the operation.
    	\return \p The resulting integer value or status code.
    */
    protected static int FindNearestTraderIndex(DayZPlayerImplement traderPlayer)
    {
        if (!traderPlayer || !traderPlayer.m_Trader_TraderPositions)
        {
            return -1;
        }

        vector playerPosition = traderPlayer.GetPosition();
        int bestIndex = -1;
        float bestDistance = 9999999.0;

        for (int i = 0; i < traderPlayer.m_Trader_TraderPositions.Count(); i++)
        {
            float distance = vector.Distance(playerPosition, traderPlayer.m_Trader_TraderPositions.Get(i));
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    /**
    	\brief Returns trader ID.
    	\param traderPlayer \p trader player used by the operation.
    	\param traderIndex \p trader index used by the operation.
    	\return \p The resulting integer value or status code.
    */
    protected static int GetTraderId(DayZPlayerImplement traderPlayer, int traderIndex)
    {
        if (!traderPlayer || !traderPlayer.m_Trader_TraderIDs || traderIndex < 0 || traderIndex >= traderPlayer.m_Trader_TraderIDs.Count())
        {
            return -1;
        }

        return traderPlayer.m_Trader_TraderIDs.Get(traderIndex);
    }

    /**
    	\brief Finds best item index.
    	\param traderPlayer \p trader player used by the operation.
    	\param className \p class name used by the operation.
    	\param persistentId \p identifier of the persistent.
    	\param traderIndex \p trader index used by the operation.
    	\param transactionType \p transaction type used by the operation.
    	\return \p The resulting integer value or status code.
    */
    protected static int FindBestItemIndex(DayZPlayerImplement traderPlayer, string className, string persistentId, int traderIndex, string transactionType)
    {
        if (!traderPlayer || !traderPlayer.m_Trader_ItemsClassnames)
        {
            return -1;
        }

        string wantedClass = className;
        wantedClass.ToLower();
        string wantedPersistent = persistentId;
        wantedPersistent.ToLower();
        int traderId = GetTraderId(traderPlayer, traderIndex);

        int bestIndex = -1;
        int bestScore = -1;

        for (int i = 0; i < traderPlayer.m_Trader_ItemsClassnames.Count(); i++)
        {
            if (!ItemMatchesAction(traderPlayer, i, transactionType))
            {
                continue;
            }

            int score = 0;
            if (traderId >= 0 && traderPlayer.m_Trader_ItemsTraderId && i < traderPlayer.m_Trader_ItemsTraderId.Count())
            {
                if (traderPlayer.m_Trader_ItemsTraderId.Get(i) == traderId)
                {
                    score += 1000;
                }
                else
                {
                    continue;
                }
            }

            string candidateClass = traderPlayer.m_Trader_ItemsClassnames.Get(i);
            string candidateLower = candidateClass;
            candidateLower.ToLower();

            if (wantedClass != "")
            {
                if (candidateLower == wantedClass)
                {
                    score += 500;
                }
                else if (transactionType == "buy" && LooksLikeVehicleKeyClass(wantedClass) && GetConfiguredQuantity(traderPlayer, i) == -7)
                {
                    score += 250;
                }
                else
                {
                    continue;
                }
            }
            else if (wantedPersistent != "")
            {
                if (StartsWith(wantedPersistent, candidateLower))
                {
                    score += 500 + candidateLower.Length();
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    /**
    	\brief Processes the item matches action operation.
    	\param traderPlayer \p trader player used by the operation.
    	\param itemIndex \p item index used by the operation.
    	\param transactionType \p transaction type used by the operation.
    	\return \p true when the operation succeeds; otherwise false.
    */
    protected static bool ItemMatchesAction(DayZPlayerImplement traderPlayer, int itemIndex, string transactionType)
    {
        if (transactionType == "buy")
        {
            return traderPlayer.m_Trader_ItemsBuyValue && itemIndex < traderPlayer.m_Trader_ItemsBuyValue.Count() && traderPlayer.m_Trader_ItemsBuyValue.Get(itemIndex) >= 0;
        }

        if (transactionType == "sell")
        {
            return traderPlayer.m_Trader_ItemsSellValue && itemIndex < traderPlayer.m_Trader_ItemsSellValue.Count() && traderPlayer.m_Trader_ItemsSellValue.Get(itemIndex) >= 0;
        }

        return false;
    }

    /**
    	\brief Processes the looks like vehicle key class operation.
    	\param classNameLower \p class name lower used by the operation.
    	\return \p true when the operation succeeds; otherwise false.
    */
    protected static bool LooksLikeVehicleKeyClass(string classNameLower)
    {
        return classNameLower.Contains("vehiclekey") || classNameLower.Contains("keylada");
    }

    /**
    	\brief Returns configured quantity.
    	\param traderPlayer \p trader player used by the operation.
    	\param itemIndex \p item index used by the operation.
    	\return \p The resulting integer value or status code.
    */
    protected static int GetConfiguredQuantity(DayZPlayerImplement traderPlayer, int itemIndex)
    {
        if (!traderPlayer || !traderPlayer.m_Trader_ItemsQuantity || itemIndex < 0 || itemIndex >= traderPlayer.m_Trader_ItemsQuantity.Count())
        {
            return 0;
        }

        return traderPlayer.m_Trader_ItemsQuantity.Get(itemIndex);
    }

    /**
    	\brief Processes the fill actor operation.
    	\param data \p data processed by the operation.
    	\param traderPlayer \p trader player used by the operation.
    */
    protected static void FillActor(ServerSentryMarketTransactionData data, DayZPlayerImplement traderPlayer)
    {
        if (!data || !traderPlayer)
        {
            return;
        }

        PlayerIdentity identity = traderPlayer.GetIdentity();
        if (identity)
        {
            data.actorSteamId = identity.GetPlainId();
            data.actorBohemiaId = identity.GetId();
            data.actorName = identity.GetName();
        }

        data.actorPosition.FromVector(traderPlayer.GetPosition());
    }

    /**
    	\brief Processes the fill trader operation.
    	\param data \p data processed by the operation.
    	\param traderPlayer \p trader player used by the operation.
    	\param traderIndex \p trader index used by the operation.
    */
    protected static void FillTrader(ServerSentryMarketTransactionData data, DayZPlayerImplement traderPlayer, int traderIndex)
    {
        if (!data || !traderPlayer || traderIndex < 0)
        {
            return;
        }

        data.traderIndex = traderIndex;
        data.traderId = GetTraderId(traderPlayer, traderIndex);
        data.traderFileName = "TraderConfig.txt";
        data.traderEntityClassName = "DrJonesTrader";
        data.traderEntityId = "traderIndex:" + traderIndex.ToString();

        if (traderPlayer.m_Trader_TraderNames && data.traderId >= 0 && data.traderId < traderPlayer.m_Trader_TraderNames.Count())
        {
            data.traderDisplayName = traderPlayer.m_Trader_TraderNames.Get(data.traderId);
        }

        if (traderPlayer.m_Trader_TraderPositions && traderIndex < traderPlayer.m_Trader_TraderPositions.Count())
        {
            data.traderPosition.FromVector(traderPlayer.m_Trader_TraderPositions.Get(traderIndex));
            data.zonePosition.FromVector(traderPlayer.m_Trader_TraderPositions.Get(traderIndex));
        }

        if (traderPlayer.m_Trader_TraderSafezones && traderIndex < traderPlayer.m_Trader_TraderSafezones.Count())
        {
            data.zoneDisplayName = "Trader safezone";
            data.zoneRadius = traderPlayer.m_Trader_TraderSafezones.Get(traderIndex);
        }

        if (traderPlayer.m_Trader_TraderVehicleSpawns && traderIndex < traderPlayer.m_Trader_TraderVehicleSpawns.Count())
        {
            data.vehicleSpawnPosition.FromVector(traderPlayer.m_Trader_TraderVehicleSpawns.Get(traderIndex));
        }

        if (traderPlayer.m_Trader_TraderVehicleSpawnsOrientation && traderIndex < traderPlayer.m_Trader_TraderVehicleSpawnsOrientation.Count())
        {
            data.vehicleSpawnOrientation.FromVector(traderPlayer.m_Trader_TraderVehicleSpawnsOrientation.Get(traderIndex));
        }
    }

    /**
    	\brief Processes the fill configured item operation.
    	\param data \p data processed by the operation.
    	\param traderPlayer \p trader player used by the operation.
    	\param itemIndex \p item index used by the operation.
    	\param transactionType \p transaction type used by the operation.
    */
    protected static void FillConfiguredItem(ServerSentryMarketTransactionData data, DayZPlayerImplement traderPlayer, int itemIndex, string transactionType)
    {
        if (!data || !traderPlayer || itemIndex < 0)
        {
            return;
        }

        data.itemIndex = itemIndex;
        data.marketItemId = itemIndex;

        if (traderPlayer.m_Trader_ItemsClassnames && itemIndex < traderPlayer.m_Trader_ItemsClassnames.Count())
        {
            data.itemClassName = traderPlayer.m_Trader_ItemsClassnames.Get(itemIndex);
            data.itemDisplayName = ResolveDisplayName(data.itemClassName);
        }

        if (traderPlayer.m_Trader_ItemsCategoryId && itemIndex < traderPlayer.m_Trader_ItemsCategoryId.Count())
        {
            data.categoryId = traderPlayer.m_Trader_ItemsCategoryId.Get(itemIndex);
            if (traderPlayer.m_Trader_Categorys && data.categoryId >= 0 && data.categoryId < traderPlayer.m_Trader_Categorys.Count())
            {
                data.categoryDisplayName = traderPlayer.m_Trader_Categorys.Get(data.categoryId);
            }
        }

        if (traderPlayer.m_Trader_ItemsBuyValue && itemIndex < traderPlayer.m_Trader_ItemsBuyValue.Count())
        {
            data.configuredBuyPrice = traderPlayer.m_Trader_ItemsBuyValue.Get(itemIndex);
        }

        if (traderPlayer.m_Trader_ItemsSellValue && itemIndex < traderPlayer.m_Trader_ItemsSellValue.Count())
        {
            data.configuredSellPrice = traderPlayer.m_Trader_ItemsSellValue.Get(itemIndex);
        }

        int quantity = GetConfiguredQuantity(traderPlayer, itemIndex);
        data.configuredQuantity = quantity;
        data.quantityMode = ResolveQuantityMode(quantity);
        data.totalAmount = QuantityToLoggedAmount(quantity);
        data.isVehicle = quantity == -2 || quantity == -6;
        data.isKeyService = quantity == -7 || quantity == -8;

        if (transactionType == "buy")
        {
            data.totalPrice = data.configuredBuyPrice;
        }
        else
        {
            data.totalPrice = data.configuredSellPrice;
        }
    }

    /**
    	\brief Copies configured item to line.
    	\param data \p data processed by the operation.
    	\param line \p line used by the operation.
    */
    protected static void CopyConfiguredItemToLine(ServerSentryMarketTransactionData data, ServerSentryMarketTransactionItem line)
    {
        if (!data || !line)
        {
            return;
        }

        line.className = data.itemClassName;
        line.displayName = data.itemDisplayName;
        line.marketItemId = data.marketItemId;
        line.categoryId = data.categoryId;
        line.amount = data.totalAmount;
        line.price = data.totalPrice;
        line.isEntity = data.isVehicle;
    }

    /**
    	\brief Processes the quantity to logged amount operation.
    	\param quantity \p quantity used by the operation.
    	\return \p The resulting integer value or status code.
    */
    protected static int QuantityToLoggedAmount(int quantity)
    {
        if (quantity > 0)
        {
            return quantity;
        }

        return 1;
    }

    /**
    	\brief Resolves quantity mode.
    	\param quantity \p quantity used by the operation.
    	\return \p The resulting string value.
    */
    protected static string ResolveQuantityMode(int quantity)
    {
        if (quantity > 0)
        {
            return "fixed";
        }

        if (quantity == -2)
        {
            return "vehicle_with_key";
        }

        if (quantity == -3)
        {
            return "magazine_empty";
        }

        if (quantity == -4)
        {
            return "weapon_empty";
        }

        if (quantity == -5)
        {
            return "random_quantity";
        }

        if (quantity == -6)
        {
            return "vehicle_no_key";
        }

        if (quantity == -7)
        {
            return "duplicate_key";
        }

        if (quantity == -8)
        {
            return "lost_key";
        }

        return "unknown";
    }

    /**
    	\brief Resolves display name.
    	\param className \p class name used by the operation.
    	\return \p The resulting string value.
    */
    protected static string ResolveDisplayName(string className)
    {
        if (!GetGame() || className == "")
        {
            return "";
        }

        string displayName;
        string path;

        path = "CfgVehicles " + className + " displayName";
        if (GetGame().ConfigIsExisting(path))
        {
            GetGame().ConfigGetText(path, displayName);
            return displayName;
        }

        path = "CfgWeapons " + className + " displayName";
        if (GetGame().ConfigIsExisting(path))
        {
            GetGame().ConfigGetText(path, displayName);
            return displayName;
        }

        path = "CfgMagazines " + className + " displayName";
        if (GetGame().ConfigIsExisting(path))
        {
            GetGame().ConfigGetText(path, displayName);
            return displayName;
        }

        path = "CfgAmmo " + className + " displayName";
        if (GetGame().ConfigIsExisting(path))
        {
            GetGame().ConfigGetText(path, displayName);
            return displayName;
        }

        return className;
    }
}

/**
	\brief Extends the DayZ Day z player implement class with ServerSentry integration.
*/
modded class DayZPlayerImplement
{
    /**
    	\brief Processes the trader trades log operation.
    	\param message \p receives a human-readable result or error message.
    */
    override void traderTradesLog(string message)
    {
        super.traderTradesLog(message);
        ServerSentryDrJonesTraderCapture.EmitFromTradeLog(this, message);
    }
}

#endif // TRADER
