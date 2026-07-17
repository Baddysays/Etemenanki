# Built-in LLM (no Ollama)

Etemenanki can run a local **GGUF** model via `llama-cpp-python` on port **11435**.

| File | Role |
|------|------|
| `manifest.json` | Model id, Hugging Face source, server port |
| `models/*.gguf` | Weights — **bundled by the installer** (not in git) |
| `.install_embedded_mode` | Flag set when embedded mode is preferred |

**Normal install:** `scripts/prepare_release.ps1` downloads the model into `build/Release` before Inno Setup packs it. Users get the model with `EtemenankiSetup-*.exe` — no post-install download.

**Recovery CLI** (if the model is missing):

```bash
python tools/embedded_llm.py install-embedded
python tools/embedded_llm.py serve
python tools/embedded_llm.py status
```
