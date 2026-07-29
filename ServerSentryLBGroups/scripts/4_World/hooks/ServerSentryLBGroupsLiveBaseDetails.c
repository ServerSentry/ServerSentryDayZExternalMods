/**
    \file ServerSentryLBGroupsLiveBaseDetails.c
    \brief Adds LB Groups ownership information to ServerSentry Live Map flag-pole details.
*/

/**
    \brief Supplies Advanced Groups ownership details for claimed TerritoryFlag objects.
*/
class ServerSentryLBGroupsFlagPoleDetailProvider: ServerSentryLiveBaseDetailProvider
{
    protected static bool s_LoggedFirstApply;
    protected static bool s_LoggedFirstResolvedGroup;

    /**
        \brief Returns whether the current Live Map base is a TerritoryFlag.
    */
    override bool Supports(EntityAI entity, ServerSentryLiveBaseDto dto)
    {
        return TerritoryFlag.Cast(entity) != null;
    }

    /**
        \brief Adds the owning group's name, tag, level, and member count.
    */
    override void ApplyDetails(EntityAI entity, ServerSentryLiveBaseDto dto)
    {
        TerritoryFlag flag = TerritoryFlag.Cast(entity);
        if (!flag || !dto)
        {
            return;
        }

        if (!s_LoggedFirstApply)
        {
            s_LoggedFirstApply = true;
            Print("[ServerSentryLBGroups][LiveMap] Flag-pole detail provider is active. First flag owner hash=" + flag.ownerGroupTagHash.ToString() + " tag='" + flag.ownerGroupTag + "'.");
        }

        LBGroup group = ResolveOwnerGroup(flag);
        if (!group)
        {
            AddMissingOwnerDetails(flag, dto);
            return;
        }

        if (!s_LoggedFirstResolvedGroup)
        {
            s_LoggedFirstResolvedGroup = true;
            Print("[ServerSentryLBGroups][LiveMap] Resolved flag owner group '" + group.name + "' (" + group.shortname + ").");
        }

        string groupName = group.name;
        if (groupName == "")
        {
            groupName = group.shortname;
        }

        ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupName", "Group", groupName);
        ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupTag", "Group tag", group.shortname);
        ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupLevel", "Group level", group.level.ToString());
        ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupMembers", "Group members", FormatMemberCount(group));
    }

    /**
        \brief Resolves the flag owner by hash and falls back to its persisted tag.
    */
    protected LBGroup ResolveOwnerGroup(TerritoryFlag flag)
    {
        if (!flag)
        {
            return null;
        }

        LBGroup group = flag.GetOwnerGroup();
        if (group)
        {
            return group;
        }

        if (flag.ownerGroupTag == "")
        {
            return null;
        }

        LBGroupManager manager = LBGroupManager.Get();
        if (!manager)
        {
            return null;
        }

        return manager.GetGroupByShortName(flag.ownerGroupTag);
    }

    /**
        \brief Formats the current and configured maximum group size.
    */
    protected string FormatMemberCount(LBGroup group)
    {
        if (!group || !group.members)
        {
            return "Unknown";
        }

        string value = group.members.Count().ToString();
        if (group.maxPlayers > 0)
        {
            value = value + " / " + group.maxPlayers.ToString();
        }

        return value;
    }

    /**
        \brief Shows an unclaimed state or a persisted owner tag whose group is unavailable.
    */
    protected void AddMissingOwnerDetails(TerritoryFlag flag, ServerSentryLiveBaseDto dto)
    {
        if (flag.ownerGroupTagHash == 0 && flag.ownerGroupTag == "")
        {
            ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupName", "Group", "Unclaimed");
            return;
        }

        ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupName", "Group", "Unavailable");

        if (flag.ownerGroupTag != "")
        {
            ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupTag", "Group tag", flag.ownerGroupTag);
        }

        if (flag.ownerGroupTagHash != 0)
        {
            ServerSentryLiveDetails.UpsertBaseDetail(dto, "groupHash", "Group hash", flag.ownerGroupTagHash.ToString());
        }
    }
}

/**
    \brief Registers the LB Groups flag-pole detail provider through ServerSentry's extension hook.
*/
modded class ServerSentryLiveBaseDetailBootstrap
{
    override void RegisterExtensionProviders()
    {
        super.RegisterExtensionProviders();
        Print("[ServerSentryLBGroups][LiveMap] Registering LB Groups flag-pole detail provider.");
        ServerSentryLiveBaseDetailRegistry.Register(new ServerSentryLBGroupsFlagPoleDetailProvider());
    }
}
