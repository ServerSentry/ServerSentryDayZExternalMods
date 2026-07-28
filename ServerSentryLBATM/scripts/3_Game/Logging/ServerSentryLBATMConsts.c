class ServerSentryLBATMConsts
{
    // Master switch for this bridge.
    const static bool LOG_ATM_EVENTS = true;

    // Personal account transactions.
    const static bool LOG_PERSONAL_DEPOSIT = true;
    const static bool LOG_PERSONAL_WITHDRAW = true;

    // Group account transactions.
    const static bool LOG_GROUP_DEPOSIT = true;
    const static bool LOG_GROUP_WITHDRAW = true;
    const static bool LOG_GROUP_ACCOUNT_CREATED = true;

    // Credit-card transactions. PIN values are never logged.
    const static bool LOG_CREDIT_CARD_DEPOSIT = true;
    const static bool LOG_CREDIT_CARD_WITHDRAW = true;
    const static bool LOG_CREDIT_CARD_WRONG_PIN = true;
    const static bool LOG_CREDIT_CARD_PIN_CHANGED = true;

    // Player-to-player money transfers.
    const static bool LOG_PLAYER_TRANSFER = true;
    const static bool LOG_TRANSFER_FEE = true;

    // Admin banking actions.
    const static bool LOG_ADMIN_MONEYDROP = true;
    const static bool LOG_ADMIN_PLAYER_ACCOUNT_CHANGE = true;
    const static bool LOG_ADMIN_GROUP_ACCOUNT_CHANGE = true;
    const static bool LOG_ADMIN_ATM_EDIT = true;
    const static bool LOG_ADMIN_ATM_DELETE = true;
    const static bool LOG_ADMIN_RESET_ROBBERS = true;

    // ATM robbery events.
    const static bool LOG_ROBBING_STARTED = true;
    const static bool LOG_ROBBING_STOPPED = true;
    const static bool LOG_ROBBING_SUCCESS = true;

    // Unknown/future LB ATM webhook events. Usually keep false to avoid noise.
    const static bool LOG_UNKNOWN_ATM_WEBHOOKS = false;
}
