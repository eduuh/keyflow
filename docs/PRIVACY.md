# Privacy Guarantee

**keyflow** collects **ZERO data**. No exceptions.

---

## What keyflow Does

✅ **Reads config.json** at startup (one-time)
✅ **Processes keystrokes in RAM** only
✅ **Sends modified keys to OS** (local only)

---

## What keyflow Does NOT Do

❌ **No keylogging** - Keystrokes never written to disk
❌ **No network calls** - Completely offline
❌ **No telemetry** - No usage statistics
❌ **No crash reporting** - No error uploads
❌ **No analytics** - No tracking whatsoever
❌ **No cloud sync** - All local
❌ **No update checks** - Manual updates only

---

## Code Verification

### Static Analysis

Verify no forbidden operations in source code:

```bash
# Check for file writes (should only find ConfigLoader)
grep -r "ofstream\|fwrite" src/

# Check for network code (should be empty)
grep -r "socket\|http\|curl\|winsock" src/

# Check for telemetry (should be empty)
grep -r "telemetry\|analytics\|tracking" src/
```

### Runtime Monitoring

Monitor keyflow during execution:

```bash
# Windows: Use Process Monitor
# Filter: Process Name = keyflow.exe
# Check: File writes (should only read config.json)
# Check: Network activity (should be none)
```

---

## Data Flow

```
Keyboard → Driver → RAM Processing → Driver → Windows
                          ↑
                    All processing
                    happens here
                    (no disk, no network)
```

**Config read:** Once at startup only
**Key processing:** RAM only
**Key injection:** Direct to OS

---

## Open Source Audit

All source code is public:

- **src/hardware/** - Driver interaction only
- **src/pipeline/** - In-memory processing
- **src/processors/** - Key transformation logic
- **src/config/** - Config loading (read-only)

**No hidden code. No obfuscation. Full transparency.**

---

## Why Trust keyflow?

1. **Open source** - Audit the code yourself
2. **Minimal dependencies** - Interception driver only
3. **Local processing** - No external services
4. **Community vetted** - Peer review
5. **Privacy-first design** - Architecture enforces privacy

---

## Future Commits

We guarantee that **no future version** will add:
- Network communication
- Data collection
- Telemetry
- Cloud features
- Tracking

**Privacy is not negotiable.**

---

## Questions?

- Read the source: `src/`
- Ask on GitHub: Issues/Discussions

**Your keystrokes are yours. Period.**
