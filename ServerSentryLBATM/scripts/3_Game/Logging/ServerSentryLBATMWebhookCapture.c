class ServerSentryLBATMWebhookCapture
{
    static bool ShouldLogType(string type)
    {
        if (!ServerSentryLBATMConsts.LOG_ATM_EVENTS)
        {
            return false;
        }

        if (!ServerSentryDefaults.LogLBATMEvents())
        {
            return false;
        }

        if (type == "ATMDepositPersonal") return ServerSentryLBATMConsts.LOG_PERSONAL_DEPOSIT;
        if (type == "ATMWithdrawPersonal") return ServerSentryLBATMConsts.LOG_PERSONAL_WITHDRAW;
        if (type == "ATMDepositGroup") return ServerSentryLBATMConsts.LOG_GROUP_DEPOSIT;
        if (type == "ATMWithdrawGroup") return ServerSentryLBATMConsts.LOG_GROUP_WITHDRAW;
        if (type == "ATMGroupAccountCreated") return ServerSentryLBATMConsts.LOG_GROUP_ACCOUNT_CREATED;
        if (type == "ATMDepositCreditcard") return ServerSentryLBATMConsts.LOG_CREDIT_CARD_DEPOSIT;
        if (type == "ATMWithdrawCreditcard") return ServerSentryLBATMConsts.LOG_CREDIT_CARD_WITHDRAW;
        if (type == "CreditCardwrongPinEntered") return ServerSentryLBATMConsts.LOG_CREDIT_CARD_WRONG_PIN;
        if (type == "CreditCardPinChanged") return ServerSentryLBATMConsts.LOG_CREDIT_CARD_PIN_CHANGED;
        if (type == "ATMMoneyTransfer") return ServerSentryLBATMConsts.LOG_PLAYER_TRANSFER;
        if (type == "ATMMoneyDrop") return ServerSentryLBATMConsts.LOG_ADMIN_MONEYDROP;
        if (type == "ATMPlayerAccountChanged") return ServerSentryLBATMConsts.LOG_ADMIN_PLAYER_ACCOUNT_CHANGE;
        if (type == "ATMGroupAccountChanged") return ServerSentryLBATMConsts.LOG_ADMIN_GROUP_ACCOUNT_CHANGE;
        if (type == "ATMEdited") return ServerSentryLBATMConsts.LOG_ADMIN_ATM_EDIT;
        if (type == "ATMDeleted") return ServerSentryLBATMConsts.LOG_ADMIN_ATM_DELETE;
        if (type == "ATMResetRobbers") return ServerSentryLBATMConsts.LOG_ADMIN_RESET_ROBBERS;
        if (type == "ATMRobbingStarted") return ServerSentryLBATMConsts.LOG_ROBBING_STARTED;
        if (type == "ATMRobbingStopped") return ServerSentryLBATMConsts.LOG_ROBBING_STOPPED;
        if (type == "ATMRobbingSuccess") return ServerSentryLBATMConsts.LOG_ROBBING_SUCCESS;

        return ServerSentryLBATMConsts.LOG_UNKNOWN_ATM_WEBHOOKS;
    }

    static void Emit(string type, TStringArray arguments, string steamid = "")
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        if (!ShouldLogType(type))
        {
            return;
        }

        ServerSentryLBATMEventData data = Build(type, arguments, steamid);
        if (!data)
        {
            return;
        }

        if (data.actorSteamId == "" && steamid != "")
        {
            data.actorSteamId = steamid;
        }

        ServerSentryApi.EmitLBATMEvent(data, null);
    }

    static ServerSentryLBATMEventData Build(string type, TStringArray arguments, string steamid = "")
    {
        ServerSentryLBATMEventData data = new ServerSentryLBATMEventData();
        data.sourceMod = "LBmaster_Groups_ATM";
        data.sourceHook = "LB_Webhook_Manager_.SendMessage";
        data.webhookType = type;
        data.raw = BuildSafeRawArguments(type, arguments);

        if (type == "ATMDepositPersonal")
        {
            FillActor(data, arguments, 0, 1);
            FillBalance(data, arguments, 2, 3, 4);
            data.atmAction = "personal_deposit";
            data.accountType = "personal";
            data.direction = "deposit";
            return data;
        }

        if (type == "ATMWithdrawPersonal")
        {
            FillActor(data, arguments, 0, 1);
            FillBalance(data, arguments, 2, 3, 4);
            data.atmAction = "personal_withdraw";
            data.accountType = "personal";
            data.direction = "withdraw";
            return data;
        }

        if (type == "ATMDepositGroup")
        {
            FillActor(data, arguments, 0, 1);
            FillBalance(data, arguments, 2, 3, 4);
            data.groupName = Arg(arguments, 5);
            data.groupTag = Arg(arguments, 6);
            data.atmAction = "group_deposit";
            data.accountType = "group";
            data.direction = "deposit";
            return data;
        }

        if (type == "ATMWithdrawGroup")
        {
            FillActor(data, arguments, 0, 1);
            FillBalance(data, arguments, 2, 3, 4);
            data.groupName = Arg(arguments, 5);
            data.groupTag = Arg(arguments, 6);
            data.atmAction = "group_withdraw";
            data.accountType = "group";
            data.direction = "withdraw";
            return data;
        }

        if (type == "ATMDepositCreditcard")
        {
            FillActor(data, arguments, 0, 1);
            FillBalance(data, arguments, 2, 3, 4);
            data.cardUid = Arg(arguments, 5);
            data.atmAction = "credit_card_deposit";
            data.accountType = "credit_card";
            data.direction = "deposit";
            return data;
        }

        if (type == "ATMWithdrawCreditcard")
        {
            FillActor(data, arguments, 0, 1);
            FillBalance(data, arguments, 2, 3, 4);
            data.cardUid = Arg(arguments, 5);
            data.atmAction = "credit_card_withdraw";
            data.accountType = "credit_card";
            data.direction = "withdraw";
            return data;
        }

        if (type == "ATMMoneyTransfer")
        {
            FillActor(data, arguments, 0, 1);
            data.recipientName = Arg(arguments, 2);
            data.recipientSteamId = Arg(arguments, 3);
            data.amountText = Arg(arguments, 4);
            data.amount = ParseMoney(data.amountText);
            data.feeText = Arg(arguments, 5);
            data.fee = ParseMoney(data.feeText);
            data.atmAction = "player_transfer";
            data.accountType = "personal";
            data.direction = "transfer";
            if (!ServerSentryLBATMConsts.LOG_TRANSFER_FEE)
            {
                data.feeText = "";
                data.fee = 0;
            }
            return data;
        }

        if (type == "ATMMoneyDrop")
        {
            FillActor(data, arguments, 0, 1);
            data.destination = Arg(arguments, 2);
            data.playerCount = Arg(arguments, 3).ToInt();
            data.amountText = Arg(arguments, 4);
            data.amount = ParseMoney(data.amountText);
            data.atmAction = "admin_moneydrop";
            data.accountType = "admin";
            data.direction = "moneydrop";
            data.isAdmin = true;
            return data;
        }

        if (type == "ATMGroupAccountCreated")
        {
            FillActor(data, arguments, 0, 1);
            data.groupName = Arg(arguments, 2);
            data.groupTag = Arg(arguments, 3);
            data.amountText = Arg(arguments, 4);
            data.amount = ParseMoney(data.amountText);
            data.atmAction = "group_account_created";
            data.accountType = "group";
            data.direction = "create_account";
            return data;
        }

        if (type == "ATMPlayerAccountChanged")
        {
            FillActor(data, arguments, 0, 1);
            data.targetName = Arg(arguments, 2);
            data.targetSteamId = Arg(arguments, 3);
            data.balanceBeforeText = Arg(arguments, 4);
            data.balanceAfterText = Arg(arguments, 5);
            data.balanceBefore = ParseMoney(data.balanceBeforeText);
            data.balanceAfter = ParseMoney(data.balanceAfterText);
            data.maxMoneyBonusBeforeText = Arg(arguments, 6);
            data.maxMoneyBonusAfterText = Arg(arguments, 7);
            data.paycheckBonusBeforeText = Arg(arguments, 8);
            data.paycheckBonusAfterText = Arg(arguments, 9);
            data.ignoreTransferFeeBefore = Arg(arguments, 10);
            data.ignoreTransferFeeAfter = Arg(arguments, 11);
            data.atmAction = "admin_player_account_changed";
            data.accountType = "personal";
            data.direction = "admin_change";
            data.isAdmin = true;
            return data;
        }

        if (type == "ATMGroupAccountChanged")
        {
            FillActor(data, arguments, 0, 1);
            data.groupName = Arg(arguments, 2);
            data.groupTag = Arg(arguments, 3);
            data.balanceBeforeText = Arg(arguments, 4);
            data.balanceAfterText = Arg(arguments, 5);
            data.balanceBefore = ParseMoney(data.balanceBeforeText);
            data.balanceAfter = ParseMoney(data.balanceAfterText);
            data.hadAccountBefore = Arg(arguments, 6);
            data.hasAccountAfter = Arg(arguments, 7);
            data.atmAction = "admin_group_account_changed";
            data.accountType = "group";
            data.direction = "admin_change";
            data.isAdmin = true;
            return data;
        }

        if (type == "ATMEdited")
        {
            FillActor(data, arguments, 0, 1);
            data.atmName = Arg(arguments, 2);
            FillATMPosition(data, arguments, 3, 4, 5);
            data.atmAction = "admin_atm_edit";
            data.accountType = "atm";
            data.direction = "admin_edit";
            data.isAdmin = true;
            return data;
        }

        if (type == "ATMDeleted")
        {
            FillActor(data, arguments, 0, 1);
            data.atmName = Arg(arguments, 2);
            FillATMPosition(data, arguments, 3, 4, 5);
            data.atmAction = "admin_atm_delete";
            data.accountType = "atm";
            data.direction = "admin_delete";
            data.isAdmin = true;
            return data;
        }

        if (type == "ATMResetRobbers")
        {
            FillActor(data, arguments, 0, 1);
            data.atmName = Arg(arguments, 2);
            FillATMPosition(data, arguments, 3, 4, 5);
            data.atmAction = "admin_reset_robbers";
            data.accountType = "atm";
            data.direction = "admin_reset";
            data.isAdmin = true;
            return data;
        }

        if (type == "ATMRobbingStarted")
        {
            FillActor(data, arguments, 0, 1);
            data.robberyGroupName = Arg(arguments, 2);
            data.robberyGroupTag = Arg(arguments, 3);
            data.groupName = data.robberyGroupName;
            data.groupTag = data.robberyGroupTag;
            data.atmName = Arg(arguments, 4);
            FillATMPosition(data, arguments, 5, 6, 7);
            data.robbingTimeMinutes = Arg(arguments, 8).ToInt();
            data.atmAction = "robbery_started";
            data.accountType = "atm";
            data.direction = "robbery";
            return data;
        }

        if (type == "ATMRobbingStopped")
        {
            data.atmName = Arg(arguments, 0);
            FillATMPosition(data, arguments, 1, 2, 3);
            data.atmAction = "robbery_stopped";
            data.accountType = "atm";
            data.direction = "robbery";
            data.result = "cancelled";
            data.actorName = "server";
            return data;
        }

        if (type == "ATMRobbingSuccess")
        {
            FillActor(data, arguments, 0, 1);
            data.robberyGroupName = Arg(arguments, 2);
            data.robberyGroupTag = Arg(arguments, 3);
            data.groupName = data.robberyGroupName;
            data.groupTag = data.robberyGroupTag;
            data.atmName = Arg(arguments, 4);
            FillATMPosition(data, arguments, 5, 6, 7);
            data.amountText = Arg(arguments, 8);
            data.amount = ParseMoney(data.amountText);
            data.atmAction = "robbery_success";
            data.accountType = "atm";
            data.direction = "robbery";
            return data;
        }

        if (type == "CreditCardwrongPinEntered")
        {
            // Treat the complete argument array as secret-bearing. The separate
            // steamid parameter is the only value trusted for attribution.
            data.actorSteamId = steamid;
            data.pinRedacted = true;
            data.atmAction = "credit_card_wrong_pin";
            data.accountType = "credit_card";
            data.direction = "security";
            data.result = "denied";
            data.reason = "wrong_pin";
            data.extra = "All webhook arguments were redacted.";
            return data;
        }

        if (type == "CreditCardPinChanged")
        {
            data.actorSteamId = steamid;
            data.pinRedacted = true;
            data.atmAction = "credit_card_pin_changed";
            data.accountType = "credit_card";
            data.direction = "security";
            data.reason = "pin_redacted";
            data.extra = "All webhook arguments were redacted.";
            return data;
        }

        if (IsSensitiveWebhookType(type))
        {
            data.actorSteamId = steamid;
            data.pinRedacted = true;
            data.atmAction = "unknown_sensitive_webhook";
            data.accountType = "security";
            data.direction = "security";
            data.reason = type;
            data.extra = "All webhook arguments were redacted.";
            return data;
        }

        FillActor(data, arguments, 0, 1);
        data.atmAction = "unknown_webhook";
        data.reason = type;
        return data;
    }

    static void FillActor(ServerSentryLBATMEventData data, TStringArray args, int nameIndex, int steamIndex)
    {
        data.actorName = Arg(args, nameIndex);
        data.actorSteamId = Arg(args, steamIndex);
    }

    static void FillBalance(ServerSentryLBATMEventData data, TStringArray args, int beforeIndex, int afterIndex, int amountIndex)
    {
        data.balanceBeforeText = Arg(args, beforeIndex);
        data.balanceAfterText = Arg(args, afterIndex);
        data.amountText = Arg(args, amountIndex);
        data.balanceBefore = ParseMoney(data.balanceBeforeText);
        data.balanceAfter = ParseMoney(data.balanceAfterText);
        data.amount = ParseMoney(data.amountText);
        data.balanceDelta = data.balanceAfter - data.balanceBefore;

        if (data.amount == 0 && data.balanceDelta != 0)
        {
            if (data.balanceDelta < 0)
            {
                data.amount = -data.balanceDelta;
            }
            else
            {
                data.amount = data.balanceDelta;
            }

            data.amountText = "inferred:" + data.amount.ToString();
            data.amountInferredFromBalanceDelta = true;
        }
    }

    static void FillATMPosition(ServerSentryLBATMEventData data, TStringArray args, int xIndex, int yIndex, int zIndex)
    {
        data.atmPosition.x = Arg(args, xIndex).ToFloat();
        data.atmPosition.y = Arg(args, yIndex).ToFloat();
        data.atmPosition.z = Arg(args, zIndex).ToFloat();
    }

    static string Arg(TStringArray args, int index)
    {
        if (!args || index < 0 || index >= args.Count())
        {
            return "";
        }

        return args.Get(index);
    }

    static int ParseMoney(string value)
    {
        if (value == "")
        {
            return 0;
        }

        string clean = value;
        clean.Replace("$", "");
        clean.Replace("€", "");
        clean.Replace("£", "");
        clean.Replace("₽", "");
        clean.Replace(",", "");
        clean.Replace(".", "");
        clean.Replace(" ", "");
        clean.Replace("Money", "");
        return clean.ToInt();
    }

    static bool IsSensitiveWebhookType(string type)
    {
        string normalized = type;
        normalized.ToLower();
        if (normalized.IndexOf("pin") != -1)
        {
            return true;
        }

        if (normalized.IndexOf("passcode") != -1)
        {
            return true;
        }

        if (normalized.IndexOf("password") != -1)
        {
            return true;
        }

        if (normalized.IndexOf("secret") != -1)
        {
            return true;
        }

        return false;
    }

    static string BuildSafeRawArguments(string type, TStringArray args)
    {
        // Secret-bearing webhook payloads are never copied into raw telemetry.
        // This remains true even if LBmaster changes argument order or adds a
        // new event name containing PIN/password/passcode terminology.
        if (IsSensitiveWebhookType(type))
        {
            return "[redacted]";
        }

        if (!args)
        {
            return "";
        }

        string raw = "";
        for (int i = 0; i < args.Count(); i++)
        {
            if (i > 0)
            {
                raw = raw + "|";
            }
            raw = raw + args.Get(i);

            // Bound webhook telemetry so a malformed or unusually verbose
            // third-party payload cannot grow one event without limit.
            if (raw.Length() >= 4096)
            {
                return raw.Substring(0, 4096);
            }
        }
        return raw;
    }
}
