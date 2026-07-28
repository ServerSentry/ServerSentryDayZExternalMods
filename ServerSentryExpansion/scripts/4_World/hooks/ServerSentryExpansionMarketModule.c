class ServerSentryExpansionMarketCaptureContext
{
    ref ServerSentryMarketTransactionData data;
    PlayerBase player;
    ExpansionMarketTraderZone zone;
    bool emitted;
    bool sawFailureSignal;
    bool sawSuccessSignal;
    int startedAt;
    string combinedLog;

    void ServerSentryExpansionMarketCaptureContext()
    {
        data = new ServerSentryMarketTransactionData();
        data.sourceMod = "DayZExpansion_Market";
        data.currency = "ExpansionMarket";
        emitted = false;
        sawFailureSignal = false;
        sawSuccessSignal = false;
        startedAt = 0;
        combinedLog = "";
    }
}

class ServerSentryExpansionMarketCapture
{
    protected static ref ServerSentryExpansionMarketCaptureContext s_Current;
    const static int MAX_CONTEXT_AGE_MS = 30000;

    static void BeginPurchase(PlayerBase player, ExpansionMarketReserve reserve, string itemClassName, bool includeAttachments, int skinIndex)
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        End();
        ServerSentryExpansionMarketCaptureContext ctx = new ServerSentryExpansionMarketCaptureContext();
        ctx.player = player;
        ctx.data.transactionType = "buy";
        ctx.data.includeAttachments = includeAttachments;
        ctx.data.skinIndex = skinIndex;
        FillActor(ctx.data, player);

        if (reserve)
        {
            ctx.data.totalPrice = reserve.Price;
            ctx.data.totalAmount = reserve.TotalAmount;
            FillMarketItem(ctx.data, reserve.RootItem);

            if (reserve.Trader)
            {
                FillTrader(ctx.data, reserve.Trader);
                ctx.zone = reserve.Trader.GetTraderZone();
            }

            if (ctx.zone)
            {
                FillZone(ctx.data, ctx.zone);
                ctx.data.stockBefore = SafeGetStock(ctx.zone, itemClassName);
            }

            if (reserve.Reserved)
            {
                foreach (ExpansionMarketReserveItem reserveItem: reserve.Reserved)
                {
                    ServerSentryMarketTransactionItem line = new ServerSentryMarketTransactionItem();
                    line.className = reserveItem.ClassName;
                    line.amount = reserveItem.Amount;
                    line.price = reserveItem.Price;
                    FillLineMarketItem(line, reserveItem.m_Item);
                    if (ctx.zone)
                    {
                        line.stockBefore = SafeGetStock(ctx.zone, reserveItem.ClassName);
                    }
                    ctx.data.items.Insert(line);
                }
            }
        }
        else
        {
            ctx.data.itemClassName = itemClassName;
        }

        if (GetGame())
        {
            ctx.startedAt = GetGame().GetTime();
        }
        s_Current = ctx;
    }

    static void BeginSell(PlayerBase player, ExpansionMarketSell sell, string itemClassName)
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        End();
        ServerSentryExpansionMarketCaptureContext ctx = new ServerSentryExpansionMarketCaptureContext();
        ctx.player = player;
        ctx.data.transactionType = "sell";
        ctx.data.itemClassName = itemClassName;
        FillActor(ctx.data, player);

        if (sell)
        {
            ctx.data.totalPrice = sell.Price;
            ctx.data.totalAmount = sell.TotalAmount;
            FillMarketItem(ctx.data, sell.Item);

            if (sell.Trader)
            {
                FillTrader(ctx.data, sell.Trader);
                ctx.zone = sell.Trader.GetTraderZone();
            }

            if (ctx.zone)
            {
                FillZone(ctx.data, ctx.zone);
                ctx.data.stockBefore = SafeGetStock(ctx.zone, itemClassName);
            }

            if (sell.Sell)
            {
                foreach (ExpansionMarketSellItem sellItem: sell.Sell)
                {
                    ServerSentryMarketTransactionItem line = new ServerSentryMarketTransactionItem();
                    line.className = sellItem.ClassName;
                    line.amount = sellItem.SoldAmount;
                    line.takenAmount = sellItem.TakenAmount;
                    line.soldAmount = sellItem.SoldAmount;
                    line.addStockAmount = sellItem.AddStockAmount;
                    line.price = sellItem.Price;
                    line.isEntity = sellItem.IsEntity;
                    FillLineMarketItem(line, sellItem.m_Item);
                    FillLineEntity(line, sellItem.ItemRep);
                    if (ctx.zone)
                    {
                        line.stockBefore = SafeGetStock(ctx.zone, sellItem.ClassName);
                    }
                    ctx.data.items.Insert(line);
                }
            }
        }
        else
        {
            ctx.data.itemClassName = itemClassName;
        }

        if (GetGame())
        {
            ctx.startedAt = GetGame().GetTime();
        }
        s_Current = ctx;
    }

    static void RecordMarketLog(string message)
    {
        if (!IsActive() || !s_Current.data)
        {
            return;
        }

        string normalized = message;
        normalized.TrimInPlace();
        if (normalized == "")
        {
            return;
        }

        if (s_Current.combinedLog != "")
        {
            s_Current.combinedLog = s_Current.combinedLog + " | ";
        }
        s_Current.combinedLog = s_Current.combinedLog + normalized;

        // Keep telemetry bounded even if Expansion writes unusually verbose
        // diagnostics during one transaction.
        if (s_Current.combinedLog.Length() > 4096)
        {
            s_Current.combinedLog = s_Current.combinedLog.Substring(0, 4096);
        }

        string lower = normalized;
        lower.ToLower();

        if (ContainsAny(lower, "failed", "failure", "error", "denied", "not enough", "invalid", "could not"))
        {
            s_Current.sawFailureSignal = true;
        }

        if (ContainsAny(lower, "success", "successfully", "purchased", "bought", "sold", "completed", "fulfilled"))
        {
            s_Current.sawSuccessSignal = true;
        }
    }

    static void Complete()
    {
        if (!IsActive() || !s_Current.data || s_Current.emitted)
        {
            return;
        }

        s_Current.emitted = true;
        ServerSentryMarketTransactionData data = s_Current.data;
        data.rawExpansionLog = s_Current.combinedLog;
        data.rawLog = s_Current.combinedLog;

        bool stockChanged = false;
        if (s_Current.zone)
        {
            data.stockAfter = SafeGetStock(s_Current.zone, data.itemClassName);
            stockChanged = data.stockAfter != data.stockBefore;

            foreach (ServerSentryMarketTransactionItem line: data.items)
            {
                line.stockAfter = SafeGetStock(s_Current.zone, line.className);
                if (line.stockAfter != line.stockBefore)
                {
                    stockChanged = true;
                }
            }
        }

        if (s_Current.sawFailureSignal)
        {
            data.result = "failed";
        }
        else if (stockChanged || s_Current.sawSuccessSignal)
        {
            data.result = "success";
        }
        else
        {
            // The private Expansion method returned normally, but no stable
            // public success callback or stock delta was available.
            data.result = "observed";
        }

        DayZPlayer dayZPlayer = DayZPlayer.Cast(s_Current.player);
        ServerSentryApi.EmitMarketTransaction(data, dayZPlayer);
    }

    protected static bool IsActive()
    {
        if (!s_Current)
        {
            return false;
        }

        if (GetGame() && s_Current.startedAt > 0)
        {
            int now = GetGame().GetTime();
            if (now < s_Current.startedAt || (now - s_Current.startedAt) > MAX_CONTEXT_AGE_MS)
            {
                End();
                return false;
            }
        }

        return true;
    }

    protected static bool ContainsAny(string value, string a, string b, string c, string d, string e, string f, string g)
    {
        if (value.IndexOf(a) != -1) return true;
        if (value.IndexOf(b) != -1) return true;
        if (value.IndexOf(c) != -1) return true;
        if (value.IndexOf(d) != -1) return true;
        if (value.IndexOf(e) != -1) return true;
        if (value.IndexOf(f) != -1) return true;
        if (value.IndexOf(g) != -1) return true;
        return false;
    }

    static void End()
    {
        s_Current = null;
    }

    protected static void FillActor(ServerSentryMarketTransactionData data, PlayerBase player)
    {
        if (!data || !player)
        {
            return;
        }

        PlayerIdentity identity = player.GetIdentity();
        if (identity)
        {
            data.actorSteamId = identity.GetPlainId();
            data.actorBohemiaId = identity.GetId();
            data.actorName = identity.GetName();
        }

        data.actorPosition.FromVector(player.GetPosition());
    }

    protected static void FillTrader(ServerSentryMarketTransactionData data, ExpansionTraderObjectBase trader)
    {
        if (!data || !trader)
        {
            return;
        }

        ExpansionMarketTrader traderMarket = trader.GetTraderMarket();
        if (traderMarket)
        {
            data.traderFileName = traderMarket.m_FileName;
        }

        data.traderDisplayName = trader.GetDisplayName();

        EntityAI traderEntity = trader.GetTraderEntity();
        if (traderEntity)
        {
            data.traderEntityClassName = traderEntity.GetType();
            data.traderEntityId = traderEntity.ToString();
            data.traderPosition.FromVector(traderEntity.GetPosition());
        }
    }

    protected static void FillZone(ServerSentryMarketTransactionData data, ExpansionMarketTraderZone zone)
    {
        if (!data || !zone)
        {
            return;
        }

        data.zoneFileName = zone.m_FileName;
        data.zoneDisplayName = zone.m_DisplayName;
        data.zonePosition.FromVector(zone.Position);
        data.zoneRadius = zone.Radius;
        data.zoneBuyPricePercent = zone.BuyPricePercent;
        data.zoneSellPricePercent = zone.SellPricePercent;
    }

    protected static void FillMarketItem(ServerSentryMarketTransactionData data, ExpansionMarketItem item)
    {
        if (!data || !item)
        {
            return;
        }

        data.itemClassName = item.ClassName;
        data.itemDisplayName = ResolveDisplayName(item.ClassName);
        data.marketItemId = item.ItemID;
        data.categoryId = item.CategoryID;
        data.minPriceThreshold = item.MinPriceThreshold;
        data.maxPriceThreshold = item.MaxPriceThreshold;
        data.minStockThreshold = item.MinStockThreshold;
        data.maxStockThreshold = item.MaxStockThreshold;
        data.quantityPercent = item.QuantityPercent;
        data.sellPricePercent = item.SellPricePercent;

        if (item.Category)
        {
            data.categoryFileName = item.Category.m_FileName;
            data.categoryDisplayName = item.Category.DisplayName;
        }

        CopyStringArray(item.SpawnAttachments, data.spawnAttachments);
        CopyStringArray(item.Variants, data.variants);
    }

    protected static void FillLineMarketItem(ServerSentryMarketTransactionItem line, ExpansionMarketItem item)
    {
        if (!line || !item)
        {
            return;
        }

        line.className = item.ClassName;
        line.displayName = ResolveDisplayName(item.ClassName);
        line.marketItemId = item.ItemID;
        line.categoryId = item.CategoryID;
        line.minPriceThreshold = item.MinPriceThreshold;
        line.maxPriceThreshold = item.MaxPriceThreshold;
        line.minStockThreshold = item.MinStockThreshold;
        line.maxStockThreshold = item.MaxStockThreshold;
        line.quantityPercent = item.QuantityPercent;
        line.sellPricePercent = item.SellPricePercent;
        CopyStringArray(item.SpawnAttachments, line.spawnAttachments);
        CopyStringArray(item.Variants, line.variants);
    }

    protected static void FillLineEntity(ServerSentryMarketTransactionItem line, EntityAI entity)
    {
        if (!line || !entity)
        {
            return;
        }

        line.objectId = entity.ToString();
        line.displayName = entity.GetDisplayName();
        line.health = entity.GetHealth("", "Health");
        line.quantity = GetEntityQuantity(entity);
    }

    protected static float GetEntityQuantity(EntityAI entity)
    {
        if (!entity)
        {
            return 0;
        }

        ItemBase itemBase = ItemBase.Cast(entity);
        if (itemBase)
        {
            return itemBase.GetQuantity();
        }

        Magazine magazine = Magazine.Cast(entity);
        if (magazine)
        {
            return magazine.GetAmmoCount();
        }

        return 0;
    }

    protected static int SafeGetStock(ExpansionMarketTraderZone zone, string className)
    {
        if (!zone || className == "")
        {
            return 0;
        }

        string lookup = className;
        lookup.ToLower();
        if (!zone.ItemExists(lookup))
        {
            return 0;
        }

        return zone.GetStock(lookup, true);
    }


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

        return "";
    }

    protected static void CopyStringArray(array<string> source, array<string> target)
    {
        if (!source || !target)
        {
            return;
        }

        foreach (string value: source)
        {
            target.Insert(value);
        }
    }
}

modded class ExpansionMarketModule
{
    override private void Exec_ConfirmPurchase(notnull PlayerBase player, string itemClassName, bool includeAttachments = true, int skinIndex = -1)
    {
        ServerSentryExpansionMarketCapture.BeginPurchase(player, player.GetMarketReserve(), itemClassName, includeAttachments, skinIndex);
        super.Exec_ConfirmPurchase(player, itemClassName, includeAttachments, skinIndex);
        ServerSentryExpansionMarketCapture.Complete();
        ServerSentryExpansionMarketCapture.End();
    }

    override private void Exec_ConfirmSell(notnull PlayerBase player, string itemClassName)
    {
        ServerSentryExpansionMarketCapture.BeginSell(player, player.GetMarketSell(), itemClassName);
        super.Exec_ConfirmSell(player, itemClassName);
        ServerSentryExpansionMarketCapture.Complete();
        ServerSentryExpansionMarketCapture.End();
    }

    override private void ExpansionLogMarket(string message)
    {
        super.ExpansionLogMarket(message);
        ServerSentryExpansionMarketCapture.RecordMarketLog(message);
    }
}

