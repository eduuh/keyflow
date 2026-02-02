# Keyflow Testing Instructions

## Prerequisites
- Run Command Prompt as **Administrator** (required for Interception driver)
- Have a text editor ready (Notepad, VS Code, etc.)

## Test 1: Single-Instance Enforcement

**Test that only one Keyflow instance can run:**

1. Open Command Prompt as Administrator
2. Navigate to: `cd C:\Users\edwinmuraya\repos\keyflow\build\release\bin\Release`
3. Start Keyflow: `keyflow.exe`
4. Check system tray for Keyflow icon (should appear)
5. Open **another** Command Prompt as Administrator
6. Try to start second instance: `keyflow.exe`
7. **Expected Result**: Second instance should show error:
   ```
   [Main] Another instance of Keyflow is already running
   [Main] Only one instance can run at a time
   ```
8. Close the first instance (Ctrl+Escape or right-click tray icon → Exit)

✅ **PASS**: Only one instance runs at a time
❌ **FAIL**: Multiple instances can run simultaneously

---

## Test 2: Phantom Capitalization Bug Fix

**Test that fast typing after shift-injecting combos works correctly:**

### Setup
1. Start Keyflow (as Administrator)
2. Open Notepad or any text editor
3. The config has these noModCombos that inject Shift:
   - `` ` → ? `` (backtick becomes question mark)
   - `1 → !` (1 becomes exclamation mark)
   - `4 → "` (4 becomes double quote)
   - etc.

### Test Case 1: Fast Typing After Backtick
**Before Fix**: `?Abc` or `?ABc` (phantom capitals)
**After Fix**: `?abc` (all lowercase)

1. Type very fast: `` ` a b c ``
2. **Expected Output**: `?abc` (all lowercase letters)
3. Repeat several times to ensure consistency

### Test Case 2: Multiple Combos in Sequence
**Before Fix**: `?!A` (phantom capital on third character)
**After Fix**: `?!a` (lowercase)

1. Type very fast: `` ` 1 a ``
2. **Expected Output**: `?!a`
3. Repeat several times

### Test Case 3: Physical Shift Still Works
**Test that real Shift key still functions normally:**

1. Type: `Shift+` ` ` then `a b c` (hold Shift for backtick only)
2. **Expected Output**: `~abc` (tilde then lowercase)
3. Type: Hold `Shift`, then `` ` a b ``
4. **Expected Output**: `~AB` (all uppercase while Shift held)

### Test Case 4: Rapid Combo Test
**Test multiple combos in quick succession:**

1. Type quickly: `` ` ` ` a b c `` (three backticks then letters)
2. **Expected Output**: `???abc` (no capitals)

### Test Case 5: Numbers and Symbols
**Test other shift-injecting combos:**

1. Type: `1 2 3 a b c` (fast)
2. **Expected Output**: `![abc` (no phantom caps)

✅ **PASS**: All lowercase letters after combos, no phantom capitalization
❌ **FAIL**: Letters become capitalized unexpectedly

---

## Test 3: System Stability

**Verify the fix doesn't break anything:**

1. Test normal typing (no combos): `the quick brown fox`
2. Test with layers (hold RALT): arrows, symbols work correctly
3. Test with layers (hold LALT): numpad works correctly
4. Exit cleanly: Ctrl+Escape (no stuck keys)

---

## Reporting Results

After testing, note:
- ✅ Single-instance works: Yes/No
- ✅ Phantom caps fixed: Yes/No
- ✅ Physical Shift works: Yes/No
- ✅ No regressions: Yes/No
- Any unexpected behavior: (describe)

---

## Quick Test Command

For a quick validation before manual testing:
```bash
cd C:\Users\edwinmuraya\repos\keyflow\build\release\bin\Release
keyflow.exe --validate
```

Should show:
```
[Config] ✅ Validation successful
[Config] Config name: Key ~ Flow Default layout
[Config] Remappings: 39
[Config] NoModCombos: 15
[Config] Layers: 2
```
