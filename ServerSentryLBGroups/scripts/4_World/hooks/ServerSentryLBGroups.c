/**
	\file ServerSentryLBGroups.c
	\brief Connects ServerSentry to DayZ or third-party hooks for LB Groups.
*/

/**
	\brief Provides ServerSentry behavior for LB Groups group member snapshot.
*/
class ServerSentryLBGroupsGroupMemberSnapshot
{
    /**
    	\brief Stable runtime key used to correlate this member snapshot.
    */
    string memberKey;
    /**
    	\brief Steam ID of the target player.
    */
    string steamId;
    /**
    	\brief Stores the bohemia ID.
    */
    string bohemiaId;
    /**
    	\brief Stores the name.
    */
    string name;
    /**
    	\brief Stores the uid.
    */
    int uid;
    /**
    	\brief Stores the permission group.
    */
    int permissionGroup;
    /**
    	\brief Stores the subgroup.
    */
    int subgroup;
    /**
    	\brief Indicates whether temp member.
    */
    bool tempMember;

    /**
    	\brief Initializes a new ServerSentryLBGroupsGroupMemberSnapshot instance.
    */
    void ServerSentryLBGroupsGroupMemberSnapshot()
    {
        memberKey = "";
        steamId = "";
        bohemiaId = "";
        name = "";
        uid = -1;
        permissionGroup = -1;
        subgroup = -1;
        tempMember = false;
    }
}

/**
	\brief Provides ServerSentry behavior for LB Groups group capture.
*/
class ServerSentryLBGroupsGroupCapture
{
    /**
    	\brief Shared group snapshots state used by all instances.
    */
    protected static ref map<string, ref map<string, ref ServerSentryLBGroupsGroupMemberSnapshot>> s_GroupSnapshots;
    /**
    	\brief Shared last emit by signature state used by all instances.
    */
    protected static ref map<string, int> s_LastEmitBySignature;

    /**
    	\brief Ensures the requested operation.
    */
    static void Ensure()
    {
        if (!s_GroupSnapshots)
        {
            s_GroupSnapshots = new map<string, ref map<string, ref ServerSentryLBGroupsGroupMemberSnapshot>>();
        }

        if (!s_LastEmitBySignature)
        {
            s_LastEmitBySignature = new map<string, int>();
        }
    }

    /**
    	\brief Processes the group key operation.
    	\param group \p group used by the operation.
    	\return \p The resulting string value.
    */
    static string GroupKey(LBGroup group)
    {
        if (!group)
        {
            return "";
        }

        int tagHash = group.GetTagHash();
        if (tagHash != 0)
        {
            return "hash:" + tagHash.ToString();
        }

        if (group.shortname != "")
        {
            return "tag:" + group.shortname;
        }

        if (group.name != "")
        {
            return "name:" + group.name;
        }

        return "";
    }

    /**
    	\brief Processes the member key operation.
    	\param member \p member used by the operation.
    	\return \p The resulting string value.
    */
    static string MemberKey(LBGroupMember member)
    {
        if (!member)
        {
            return "";
        }

        string steamId = member.GetOriginalSteamid();
        if (steamId != "")
        {
            return "steam:" + steamId;
        }

        if (member.steamid != "")
        {
            return "steam:" + member.steamid;
        }

        if (member.uid >= 0)
        {
            return "uid:" + member.uid.ToString();
        }

        return "";
    }

    /**
    	\brief Builds snapshot.
    	\param member \p member used by the operation.
    	\return \p The resulting LB Groups group member snapshot instance, or null when unavailable.
    */
    static ServerSentryLBGroupsGroupMemberSnapshot BuildSnapshot(LBGroupMember member)
    {
        ServerSentryLBGroupsGroupMemberSnapshot snap = new ServerSentryLBGroupsGroupMemberSnapshot();
        if (!member)
        {
            return snap;
        }

        snap.memberKey = MemberKey(member);
        snap.steamId = member.GetOriginalSteamid();
        if (snap.steamId == "")
        {
            snap.steamId = member.steamid;
        }

        snap.name = member.name;
        snap.uid = member.uid;
        snap.permissionGroup = member.permissionGroup;
        snap.subgroup = member.currentSubgroup;
        snap.tempMember = member.tempMember;

        PlayerBase player = FindPlayerBySteamId(snap.steamId);
        if (player && player.GetIdentity())
        {
            snap.bohemiaId = player.GetIdentity().GetId();
            if (snap.name == "")
            {
                snap.name = player.GetIdentity().GetName();
            }
        }

        return snap;
    }

    /**
    	\brief Processes the capture group operation.
    	\param group \p group used by the operation.
    */
    static void CaptureGroup(LBGroup group)
    {
        if (!GetGame() || !GetGame().IsServer() || !group)
        {
            return;
        }

        Ensure();

        string groupKey = GroupKey(group);
        if (groupKey == "")
        {
            return;
        }

        ref map<string, ref ServerSentryLBGroupsGroupMemberSnapshot> members = new map<string, ref ServerSentryLBGroupsGroupMemberSnapshot>();
        if (!group.members)
        {
            s_GroupSnapshots.Set(groupKey, members);
            return;
        }

        foreach (LBGroupMember member : group.members)
        {
            if (!member)
            {
                continue;
            }

            string memberKey = MemberKey(member);
            if (memberKey == "")
            {
                continue;
            }

            members.Set(memberKey, BuildSnapshot(member));
        }

        s_GroupSnapshots.Set(groupKey, members);
    }

    /**
    	\brief Processes the capture all Groups operation.
    	\param manager \p manager used by the operation.
    */
    static void CaptureAllGroups(LBGroupManager manager)
    {
        if (!GetGame() || !GetGame().IsServer() || !manager)
        {
            return;
        }

        array<ref LBGroup> groups = manager.GetAllGroups();
        if (!groups)
        {
            return;
        }

        ref map<string, bool> seenGroupKeys = new map<string, bool>();
        foreach (LBGroup group : groups)
        {
            string key = GroupKey(group);
            if (key != "")
            {
                seenGroupKeys.Set(key, true);
            }
            CaptureGroup(group);
        }

        CleanupMissingGroupSnapshots(seenGroupKeys);
    }

    protected static void CleanupMissingGroupSnapshots(map<string, bool> seenGroupKeys)
    {
        Ensure();
        if (!seenGroupKeys)
        {
            return;
        }

        array<string> staleKeys = new array<string>();
        foreach (string groupKey, map<string, ref ServerSentryLBGroupsGroupMemberSnapshot> members : s_GroupSnapshots)
        {
            if (!seenGroupKeys.Contains(groupKey))
            {
                staleKeys.Insert(groupKey);
            }
        }

        foreach (string staleKey : staleKeys)
        {
            s_GroupSnapshots.Remove(staleKey);
        }

        // Defensive emergency bound. Clearing only causes a clean baseline on
        // the next save/load and is preferable to unbounded runtime growth.
        if (s_GroupSnapshots.Count() > 4096)
        {
            s_GroupSnapshots.Clear();
        }
    }

    /**
    	\brief Processes group diff.
    	\param group \p group used by the operation.
    	\param sourceHook \p source hook used by the operation.
    */
    static void ProcessGroupDiff(LBGroup group, string sourceHook)
    {
        if (!GetGame() || !GetGame().IsServer() || !group)
        {
            return;
        }

        Ensure();

        string groupKey = GroupKey(group);
        if (groupKey == "")
        {
            return;
        }

        ref map<string, ref ServerSentryLBGroupsGroupMemberSnapshot> oldMembers = s_GroupSnapshots.Get(groupKey);
        if (!oldMembers)
        {
            // First time seeing this group in this server runtime: baseline only, no restart spam.
            CaptureGroup(group);
            return;
        }

        // A null member collection can be a transient/unloaded state. Do not
        // mutate LBmaster-owned data and do not infer that every member left.
        if (!group.members)
        {
            return;
        }

        ref map<string, bool> seen = new map<string, bool>();
        foreach (LBGroupMember member : group.members)
        {
            if (!member)
            {
                continue;
            }

            string memberKey = MemberKey(member);
            if (memberKey == "")
            {
                continue;
            }

            seen.Set(memberKey, true);
            ServerSentryLBGroupsGroupMemberSnapshot oldSnap = oldMembers.Get(memberKey);
            ServerSentryLBGroupsGroupMemberSnapshot newSnap = BuildSnapshot(member);

            if (!oldSnap)
            {
                EmitMemberEvent(group, member, null, newSnap, "join", sourceHook, "diff_member_added");
            }
            else if (oldSnap.subgroup != newSnap.subgroup)
            {
                EmitSubgroupChange(group, member, oldSnap.subgroup, newSnap.subgroup, sourceHook, "diff_subgroup_changed");
            }
        }

        foreach (string oldMemberKey, ServerSentryLBGroupsGroupMemberSnapshot removedSnap : oldMembers)
        {
            if (!seen.Contains(oldMemberKey))
            {
                EmitMemberEvent(group, null, removedSnap, null, "leave", sourceHook, "diff_member_removed");
            }
        }

        CaptureGroup(group);
    }

    /**
    	\brief Handles the DayZ Member added callback.
    	\param group \p group used by the operation.
    	\param player \p player entity associated with the operation.
    	\param sourceHook \p source hook used by the operation.
    */
    static void OnMemberAdded(LBGroup group, PlayerBase player, string sourceHook)
    {
        if (!GetGame() || !GetGame().IsServer() || !group || !player || !player.GetIdentity())
        {
            return;
        }

        LBGroupMember member = group.GetMemberBySteamid(player.GetIdentity().GetPlainId());
        if (!member)
        {
            return;
        }

        ServerSentryLBGroupsGroupMemberSnapshot newSnap = BuildSnapshot(member);
        string groupKey = GroupKey(group);
        string memberKey = MemberKey(member);

        Ensure();
        ref map<string, ref ServerSentryLBGroupsGroupMemberSnapshot> oldMembers = s_GroupSnapshots.Get(groupKey);
        if (oldMembers && oldMembers.Contains(memberKey))
        {
            oldMembers.Set(memberKey, newSnap);
            return;
        }

        EmitMemberEvent(group, member, null, newSnap, "join", sourceHook, "add_member");
        UpdateMemberSnapshot(group, memberKey, newSnap);
    }

    /**
    	\brief Handles the DayZ Member removed callback.
    	\param group \p group used by the operation.
    	\param oldSnap \p old snap used by the operation.
    	\param sourceHook \p source hook used by the operation.
    */
    static void OnMemberRemoved(LBGroup group, ServerSentryLBGroupsGroupMemberSnapshot oldSnap, string sourceHook)
    {
        if (!GetGame() || !GetGame().IsServer() || !group || !oldSnap)
        {
            return;
        }

        EmitMemberEvent(group, null, oldSnap, null, "leave", sourceHook, "remove_member");
        RemoveMemberSnapshot(group, oldSnap.memberKey);
    }

    /**
    	\brief Handles the DayZ Subgroup set callback.
    	\param member \p member used by the operation.
    	\param oldSubgroup \p old subgroup used by the operation.
    	\param newSubgroup \p new subgroup used by the operation.
    	\param sourceHook \p source hook used by the operation.
    */
    static void OnSubgroupSet(LBGroupMember member, int oldSubgroup, int newSubgroup, string sourceHook)
    {
        if (!GetGame() || !GetGame().IsServer() || !member || oldSubgroup == newSubgroup)
        {
            return;
        }

        LBGroup group = LBGroup.Cast(member.parentGroup);
        if (!group)
        {
            return;
        }

        EmitSubgroupChange(group, member, oldSubgroup, newSubgroup, sourceHook, "set_subgroup");
        UpdateMemberSnapshot(group, MemberKey(member), BuildSnapshot(member));
    }

    /**
    	\brief Emits subgroup change.
    	\param group \p group used by the operation.
    	\param member \p member used by the operation.
    	\param oldSubgroup \p old subgroup used by the operation.
    	\param newSubgroup \p new subgroup used by the operation.
    	\param sourceHook \p source hook used by the operation.
    	\param reason \p human-readable operation description stored with fulfillment state.
    */
    static void EmitSubgroupChange(LBGroup group, LBGroupMember member, int oldSubgroup, int newSubgroup, string sourceHook, string reason)
    {
        if (!member)
        {
            return;
        }

        ServerSentryLBGroupsGroupMemberSnapshot oldSnap = BuildSnapshot(member);
        oldSnap.subgroup = oldSubgroup;
        ServerSentryLBGroupsGroupMemberSnapshot newSnap = BuildSnapshot(member);
        newSnap.subgroup = newSubgroup;
        EmitMemberEvent(group, member, oldSnap, newSnap, "subgroup_change", sourceHook, reason);
    }

    /**
    	\brief Emits member event.
    	\param group \p group used by the operation.
    	\param liveMember \p live member used by the operation.
    	\param oldSnap \p old snap used by the operation.
    	\param newSnap \p new snap used by the operation.
    	\param action \p action used by the operation.
    	\param sourceHook \p source hook used by the operation.
    	\param reason \p human-readable operation description stored with fulfillment state.
    */
    static void EmitMemberEvent(LBGroup group, LBGroupMember liveMember, ServerSentryLBGroupsGroupMemberSnapshot oldSnap, ServerSentryLBGroupsGroupMemberSnapshot newSnap, string action, string sourceHook, string reason)
    {
        if (!ServerSentryDefaults.LogLBMasterGroupEvents())
        {
            return;
        }

        if (!group)
        {
            return;
        }

        ServerSentryLBGroupsGroupMemberSnapshot activeSnap = newSnap;
        if (!activeSnap)
        {
            activeSnap = oldSnap;
        }
        if (!activeSnap)
        {
            return;
        }

        string identityKey = activeSnap.memberKey;
        if (identityKey == "")
        {
            identityKey = activeSnap.steamId;
        }

        string signature = GroupKey(group) + "|" + identityKey + "|" + action + "|" + activeSnap.subgroup.ToString();
        if (action == "subgroup_change" && oldSnap && newSnap)
        {
            signature = GroupKey(group) + "|" + identityKey + "|" + action + "|" + oldSnap.subgroup.ToString() + ">" + newSnap.subgroup.ToString();
        }

        if (!ShouldEmit(signature))
        {
            return;
        }

        ServerSentryGroupEventData data = new ServerSentryGroupEventData();
        data.sourceMod = "LBmaster_Groups";
        data.groupAction = action;
        data.result = "success";
        data.reason = reason;
        data.sourceHook = sourceHook;

        data.groupName = group.name;
        data.groupTag = group.shortname;
        data.groupHash = group.GetTagHash();
        data.groupLevel = group.level;
        data.memberCount = 0;
        if (group.members)
        {
            data.memberCount = group.members.Count();
        }

        data.memberSteamId = activeSnap.steamId;
        data.memberBohemiaId = activeSnap.bohemiaId;
        data.memberName = activeSnap.name;
        data.memberUid = activeSnap.uid;
        data.permissionGroup = activeSnap.permissionGroup;
        data.isTemporaryMember = activeSnap.tempMember;

        data.actorSteamId = activeSnap.steamId;
        data.actorBohemiaId = activeSnap.bohemiaId;
        data.actorName = activeSnap.name;

        if (oldSnap)
        {
            data.fromSubgroupIndex = oldSnap.subgroup;
            data.fromSubgroupName = ResolveSubgroupName(group, oldSnap.subgroup);
        }

        if (newSnap)
        {
            data.toSubgroupIndex = newSnap.subgroup;
            data.toSubgroupName = ResolveSubgroupName(group, newSnap.subgroup);
        }

        PlayerBase player = FindPlayerBySteamId(activeSnap.steamId);
        if (player)
        {
            data.actorPosition.FromVector(player.GetPosition());
            if (player.GetIdentity())
            {
                data.actorBohemiaId = player.GetIdentity().GetId();
                data.memberBohemiaId = data.actorBohemiaId;
                data.actorName = player.GetIdentity().GetName();
                data.memberName = data.actorName;
            }
        }

        ServerSentryApi.EmitGroupEvent(data, DayZPlayer.Cast(player));
    }

    /**
    	\brief Returns whether emit.
    	\param signature \p signature used by the operation.
    	\return \p true when the condition is satisfied; otherwise false.
    */
    static bool ShouldEmit(string signature)
    {
        Ensure();

        if (!GetGame())
        {
            return true;
        }

        int now = GetGame().GetTime();
        int last;
        if (s_LastEmitBySignature.Find(signature, last))
        {
            if (now >= last && (now - last) < 1500)
            {
                return false;
            }
        }

        s_LastEmitBySignature.Set(signature, now);
        CleanupDeduper(now);
        return true;
    }

    protected static void CleanupDeduper(int now)
    {
        if (!s_LastEmitBySignature)
        {
            return;
        }

        if (s_LastEmitBySignature.Count() > 4096)
        {
            s_LastEmitBySignature.Clear();
            return;
        }

        if (s_LastEmitBySignature.Count() < 2048)
        {
            return;
        }

        array<string> expired = new array<string>();
        foreach (string key, int timestamp : s_LastEmitBySignature)
        {
            if (now < timestamp || (now - timestamp) > 60000)
            {
                expired.Insert(key);
            }
        }

        foreach (string expiredKey : expired)
        {
            s_LastEmitBySignature.Remove(expiredKey);
        }
    }

    /**
    	\brief Updates member snapshot.
    	\param group \p group used by the operation.
    	\param memberKey \p member key used by the operation.
    	\param snap \p snap used by the operation.
    */
    static void UpdateMemberSnapshot(LBGroup group, string memberKey, ServerSentryLBGroupsGroupMemberSnapshot snap)
    {
        if (!group || memberKey == "" || !snap)
        {
            return;
        }

        Ensure();

        string groupKey = GroupKey(group);
        ref map<string, ref ServerSentryLBGroupsGroupMemberSnapshot> members = s_GroupSnapshots.Get(groupKey);
        if (!members)
        {
            members = new map<string, ref ServerSentryLBGroupsGroupMemberSnapshot>();
            s_GroupSnapshots.Set(groupKey, members);
        }

        members.Set(memberKey, snap);
    }

    /**
    	\brief Removes member snapshot.
    	\param group \p group used by the operation.
    	\param memberKey \p member key used by the operation.
    */
    static void RemoveMemberSnapshot(LBGroup group, string memberKey)
    {
        if (!group || memberKey == "")
        {
            return;
        }

        Ensure();

        ref map<string, ref ServerSentryLBGroupsGroupMemberSnapshot> members = s_GroupSnapshots.Get(GroupKey(group));
        if (members)
        {
            members.Remove(memberKey);
        }
    }

    static bool ContainsSnapshotMember(LBGroup group, ServerSentryLBGroupsGroupMemberSnapshot snap)
    {
        if (!group || !snap)
        {
            return false;
        }

        // Unknown/unloaded collection state must not be treated as a confirmed
        // removal. SaveGroup/Load hooks will reconcile the snapshot later.
        if (!group.members)
        {
            return true;
        }

        foreach (LBGroupMember member : group.members)
        {
            if (member && MemberKey(member) == snap.memberKey)
            {
                return true;
            }
        }

        return false;
    }

    /**
    	\brief Finds player by steam ID.
    	\param steamId \p Steam ID of the target player.
    	\return \p The resulting player base instance, or null when unavailable.
    */
    static PlayerBase FindPlayerBySteamId(string steamId)
    {
        if (!GetGame() || steamId == "")
        {
            return null;
        }

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == steamId)
            {
                return player;
            }
        }

        return null;
    }

    /**
    	\brief Resolves subgroup name.
    	\param group \p group used by the operation.
    	\param index \p zero-based collection index.
    	\return \p The resulting string value.
    */
    static string ResolveSubgroupName(LBGroup group, int index)
    {
        if (index < 0)
        {
            return "";
        }

        if (group && group.subgroups && index < group.subgroups.Count())
        {
            LBGroupLevelSubgroupInfo subgroupInfo = group.subgroups.Get(index);
            if (subgroupInfo && subgroupInfo.nameOverride != "")
            {
                return subgroupInfo.nameOverride;
            }
        }

        if (LBGroupSettingsConfig.Get && LBGroupSettingsConfig.Get.subGroupNames && index < LBGroupSettingsConfig.Get.subGroupNames.Count())
        {
            return LBGroupSettingsConfig.Get.subGroupNames.Get(index);
        }

        return "Subgroup " + index.ToString();
    }
}

/**
	\brief Extends the DayZ LB group manager class with ServerSentry integration.
*/
modded class LBGroupManager
{
    /**
    	\brief Loads all Groups.
    */
    override void LoadAllGroups()
    {
        super.LoadAllGroups();
        ServerSentryLBGroupsGroupCapture.CaptureAllGroups(this);
    }

    /**
    	\brief Saves group.
    	\param grp \p grp used by the operation.
    */
    override void SaveGroup(LBGroup grp)
    {
        super.SaveGroup(grp);
        ServerSentryLBGroupsGroupCapture.ProcessGroupDiff(grp, "LBGroupManager.SaveGroup");
    }
}

/**
	\brief Extends the DayZ LB group class with ServerSentry integration.
*/
modded class LBGroup
{
    /**
    	\brief Handles the DayZ Load server callback.
    */
    override void OnLoadServer()
    {
        super.OnLoadServer();
        ServerSentryLBGroupsGroupCapture.CaptureGroup(this);
    }

    /**
    	\brief Adds member.
    	\param player \p player entity associated with the operation.
    	\return \p true when the operation succeeds; otherwise false.
    */
    override bool AddMember(PlayerBase player)
    {
        bool result = super.AddMember(player);
        if (result)
        {
            ServerSentryLBGroupsGroupCapture.OnMemberAdded(this, player, "LBGroup.AddMember");
        }
        return result;
    }

    /**
    	\brief Removes member.
    	\param player \p player entity associated with the operation.
    */
    override void RemoveMember(PlayerBase player)
    {
        ServerSentryLBGroupsGroupMemberSnapshot oldSnap;
        if (player && player.GetIdentity())
        {
            LBGroupMember member = GetMemberBySteamid(player.GetIdentity().GetPlainId());
            if (member)
            {
                oldSnap = ServerSentryLBGroupsGroupCapture.BuildSnapshot(member);
            }
        }

        super.RemoveMember(player);

        if (oldSnap && !ServerSentryLBGroupsGroupCapture.ContainsSnapshotMember(this, oldSnap))
        {
            ServerSentryLBGroupsGroupCapture.OnMemberRemoved(this, oldSnap, "LBGroup.RemoveMember(PlayerBase)");
        }
    }

    /**
    	\brief Removes member.
    	\param steamid \p identifier of the steam.
    */
    override void RemoveMember(string steamid)
    {
        ServerSentryLBGroupsGroupMemberSnapshot oldSnap;
        LBGroupMember member = GetMemberBySteamid(steamid);
        if (member)
        {
            oldSnap = ServerSentryLBGroupsGroupCapture.BuildSnapshot(member);
        }

        super.RemoveMember(steamid);

        if (oldSnap && !ServerSentryLBGroupsGroupCapture.ContainsSnapshotMember(this, oldSnap))
        {
            ServerSentryLBGroupsGroupCapture.OnMemberRemoved(this, oldSnap, "LBGroup.RemoveMember(string)");
        }
    }

    /**
    	\brief Removes member.
    	\param member \p member used by the operation.
    */
    override void RemoveMember(LBGroupMember member)
    {
        ServerSentryLBGroupsGroupMemberSnapshot oldSnap;
        if (member)
        {
            oldSnap = ServerSentryLBGroupsGroupCapture.BuildSnapshot(member);
        }

        super.RemoveMember(member);

        if (oldSnap && !ServerSentryLBGroupsGroupCapture.ContainsSnapshotMember(this, oldSnap))
        {
            ServerSentryLBGroupsGroupCapture.OnMemberRemoved(this, oldSnap, "LBGroup.RemoveMember(LBGroupMember)");
        }
    }
}

/**
	\brief Extends the DayZ LB marker class with ServerSentry integration.
*/
modded class LBMarker
{
    /**
    	\brief Sets sub group.
    	\param grp \p grp used by the operation.
    */
    override void SetSubGroup(int grp)
    {
        int oldSubgroup = currentSubgroup;
        super.SetSubGroup(grp);

        LBGroupMember member = LBGroupMember.Cast(this);
        if (member)
        {
            ServerSentryLBGroupsGroupCapture.OnSubgroupSet(member, oldSubgroup, currentSubgroup, "LBMarker.SetSubGroup");
        }
    }
}
