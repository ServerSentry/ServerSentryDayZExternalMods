modded class LB_Webhook_Manager_
{
    override void SendMessage(string type, TStringArray arguments, string steamid = "")
    {
        // Preserve the original webhook behavior first. ServerSentry observes
        // the completed call and never replaces LBmaster banking logic.
        super.SendMessage(type, arguments, steamid);
        ServerSentryLBATMWebhookCapture.Emit(type, arguments, steamid);
    }
}
