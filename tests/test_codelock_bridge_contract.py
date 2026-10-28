"""Static compatibility checks, not a replacement for a DayZ script compile."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
ADDON = ROOT / "ServerSentryCodeLock"
HOOKS = ADDON / "scripts" / "4_World" / "hooks"

# Existing RoomService/Wardog CodeLock methods. In particular, this contract
# deliberately excludes the locally introduced logger context/capture API.
STOCK_METHODS = {
    "CodeLockServerRPC": {
        "ServerRPCHandler": ("void", ["PlayerIdentity", "Object", "int", "ParamsReadContext"]),
        "SetCode": ("void", ["ItemBase", "string"]),
        "EnterCode": ("void", ["ItemBase", "string", "string"]),
        "AdminMenu": ("void", ["ItemBase", "string"]),
        "ClaimCode": ("void", ["CodeLock", "string"]),
        "SendMenuError": ("void", ["string"]),
        "SendShowSetCodeRPC": ("void", ["ItemBase"]),
    },
    "CodeLockLogger": {
        "WriteLog": ("void", ["string", "PlayerIdentity", "vector", "string", "bool", "bool", "float", "float"]),
    },
    "CodeLock": {
        "IsLocked": ("bool", ["string"]),
        "IsOwner": ("bool", ["string"]),
        "LockServer": ("void", ["ItemBase", "string"]),
        "NewUnlockServer": ("void", ["EntityAI", "EntityAI"]),
        "ServerSetOwner": ("void", ["string"]),
        "ServerAddGuest": ("void", ["string"]),
        "ServerClearGuests": ("void", []),
    },
}
for target in ("Fence", "Tent"):
    STOCK_METHODS[f"ActionInteractLockOn{target}"] = {"OnStartServer": ("void", ["ActionData"])}
    STOCK_METHODS[f"ActionAttachCodeLockTo{target}"] = {"OnExecuteServer": ("void", ["ActionData"])}
    STOCK_METHODS[f"ActionDestroyCodeLockOn{target}"] = {"OnFinishProgressServer": ("void", ["ActionData"])}


def without_comments(source):
    # Preserve string literals, including URLs, while removing comments.
    return re.sub(
        r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*[\s\S]*?\*/',
        lambda match: match[0] if match[0].startswith('"') else "",
        source,
    )


def body_after(source, opening):
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1:index]
    raise AssertionError("Unclosed source block")


def modded_classes(source):
    for match in re.finditer(r"modded\s+class\s+(\w+)\s*\{", source):
        yield match[1], body_after(source, match.end() - 1)


def overrides(source):
    pattern = r"override\s+(?:(?:private|protected)\s+)?(\w+)\s+(\w+)\s*\(([^)]*)\)\s*\{"
    for match in re.finditer(pattern, source):
        parameters = [part.split("=", 1)[0].strip().split() for part in match[3].split(",") if part.strip()]
        yield match[1], match[2], parameters, body_after(source, match.end() - 1)


class CodeLockBridgeContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = "\n".join(without_comments(path.read_text(encoding="utf-8-sig")) for path in sorted(HOOKS.glob("*.c")))

    def test_regular_codelock_needs_no_custom_logger_api(self):
        for symbol in ("CodeLockLogContext", "CaptureLockEvent", "WriteLockEvent"):
            self.assertNotRegex(self.source, rf"\b{symbol}\b")

    def test_overrides_match_existing_mod_signatures(self):
        seen = set()
        for class_name, class_body in modded_classes(self.source):
            self.assertIn(class_name, STOCK_METHODS)
            for return_type, name, parameters, _ in overrides(class_body):
                key = (class_name, name)
                self.assertNotIn(key, seen, f"Duplicate hook: {key}")
                seen.add(key)
                self.assertIn(name, STOCK_METHODS[class_name], f"New upstream dependency: {key}")
                self.assertEqual((return_type, [parts[-2] for parts in parameters]), STOCK_METHODS[class_name][name], key)
        expected = {(class_name, name) for class_name, methods in STOCK_METHODS.items() for name in methods}
        self.assertEqual(seen, expected)

    def test_each_hook_forwards_original_arguments_once(self):
        for class_name, class_body in modded_classes(self.source):
            for _, name, parameters, body in overrides(class_body):
                calls = re.findall(rf"\bsuper\.{name}\s*\(([^)]*)\)", body)
                self.assertEqual(len(calls), 1, (class_name, name))
                forwarded = [part.strip() for part in calls[0].split(",") if part.strip()]
                self.assertEqual(forwarded, [parts[-1] for parts in parameters], (class_name, name))

    def test_targets_do_not_depend_on_coordinates_or_colliding_getters(self):
        self.assertNotRegex(self.source, r"\b(?:GetObjectsAtPosition\w*|NearestObject|FindCodeLockAt|GetCodeLock|GetCodeLockRS)\s*\(")
        self.assertIn('CodeLock.Cast(target.FindAttachmentBySlotName("Att_CombinationLock"))', self.source)

    def test_rpc_stream_and_gameplay_are_left_to_original_methods(self):
        self.assertNotRegex(self.source, r"\bctx\s*\.\s*(?:Read|Write)\s*\(")
        self.assertNotRegex(self.source, r"\b(?:RPCSingleParam|ServerDropEntity|PredictiveTakeEntityToTargetAttachmentEx|OpenFence|ToggleAnimation|AddHealth|Delete)\s*\(")

    def test_vanilla_hooks_remain_owned_by_core(self):
        classes = {name for name, _ in modded_classes(self.source)}
        self.assertTrue(classes.isdisjoint({"DigitalCodeLock", "CodeLockComponent", "ActionInteractCodeLockItem", "ActionResetCodeOnCodeLock"}))

    def test_semantic_attempts_are_not_time_deduplicated(self):
        match = re.search(r"static void EmitLockEvent\([^)]*\)\s*\{", self.source)
        self.assertIsNotNone(match)
        body = body_after(self.source, match.end() - 1)
        self.assertNotIn("ShouldEmit", body)
        self.assertIn("ServerSentryApi.EmitCodeLockEvent(data)", body)

    def test_sync_is_filtered_before_original_file_logger(self):
        logger = dict(modded_classes(self.source))["CodeLockLogger"]
        self.assertLess(logger.index("IsSyncLogType(type)"), logger.index("super.WriteLog("))


if __name__ == "__main__":
    unittest.main()
