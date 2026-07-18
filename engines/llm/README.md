# Built-in LLM (no Ollama)

Etemenanki ships **TranslateGemma 4B** (GGUF Q3_K_S) via `llama-cpp-python` on port **11435**.

| File | Role |
|------|------|
| `manifest.json` | Model id, Hugging Face source, server port |
| `models/translategemma-4b-it.Q3_K_S.gguf` | Weights — bundled (~1.8 GiB; fits GitHub 2 GiB release limit) |
| `.install_embedded_mode` | Prefer embedded mode after install |

**Source:** `aoiandroid/translategemma-4b-it-GGUF` ← `google/translategemma-4b-it`.

**Release prep:** `scripts/prepare_release.ps1`

**Recovery:**

```bash
python tools/embedded_llm.py install-embedded
python tools/embedded_llm.py serve
python tools/embedded_llm.py status
```
