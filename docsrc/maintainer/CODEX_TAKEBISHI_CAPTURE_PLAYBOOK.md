# Codex Takebishi Capture Playbook

Audience: maintainers using Codex to drive Takebishi OPC Server capture work.

This page is a practical instruction sheet for asking Codex to help with Takebishi-based MC
protocol capture and comparison.

## Goal

Use Takebishi OPC Server as a black-box implementation and capture what MC command family it emits
for a controlled set of tags.

The current purpose is not "make Takebishi work" but:

- force a narrow tag set
- observe which command family appears on the wire
- compare that family with this repository's implementation

## Ground Rules

- Do not run parallel UART traffic.
- Capture one hypothesis at a time.
- Keep one active scenario per run.
- Stop unrelated groups or polling while capturing.
- Prefer plain addresses first: `D`, `M`, `B`, `X`.
- Treat `Y` as optional because it can affect real outputs.
- Do not mix contiguous and sparse probes in the same capture run.

## Probe Tag Pool

Use [../../cap/takebishi_probe_tags.csv](../../cap/takebishi_probe_tags.csv) as the import source.

That file gives a shared tag pool with these address families:

- `D100-D105`, `D110-D112`, `D200`, `D210`
- `D100_DR`, `D105_DR`
- `M100-M115`, `M200-M231`
- `B100`, `B105`
- `X010`, `X130`, `X180`

Known device-type mapping from the exported Takebishi project:

- `X = 1`
- `Y = 2`
- `M = 3`
- `B = 4`
- `D = 10`

Important:

- For hexadecimal devices, `DevNo` in the CSV is the numeric value, not the text digits.
- Examples from the exported project and `pak4` capture:
  - `X10 -> DevNo 16`
  - `X130 -> DevNo 304`
  - `X180 -> DevNo 384`
  - `YA0 -> DevNo 160`
  - `Y220 -> DevNo 544`
  - `B100 -> DevNo 256`
  - `B360 -> DevNo 864`

## Scenario Map

Use only the tags needed for the scenario being captured.

### `0403` random-read

Word single:

- `D100`

Word dense:

- `D100`
- `D101`

Word sparse:

- `D100`
- `D105`

Word + dword:

- `D100`
- `D100_DR`
- `D105_DR`

Bit single:

- `M100`

Bit dense:

- `M100`
- `M101`

Bit sparse:

- `M100`
- `M105`

Alternate bit devices:

- `B100`
- `B105`
- `X010`
- `X130`
- `X180`

### `1402` random-write-words

Single:

- `D100`

Dense:

- `D100`
- `D101`

Sparse:

- `D100`
- `D105`

Dword:

- `D100_DR`
- `D105_DR`

### `1402` random-write-bits

Single:

- `M100`

Dense:

- `M100`
- `M101`
- `M102`
- `M103`

Sparse:

- `M100`
- `M105`
- `M110`
- `M115`

Alternate bit devices:

- `B100`
- `B105`

### `0406/1406` multi-block

Word blocks:

- `D100`
- `D101`
- `D110`
- `D111`
- `D112`

Bit blocks:

- `M100-M115`
- `M200-M231`

Mixed:

- `D100`
- `D101`
- `D110`
- `D111`
- `D112`
- `M100-M115`
- `M200-M231`

## What To Ask Codex To Do

Codex should help with:

- choosing a narrow scenario
- checking that the scenario is not mixed with unrelated tags
- interpreting the exported project settings
- comparing captured TX/RX against this repository's request shapes
- deciding whether the result points to `0401`, `0403`, `1401`, `1402`, `0406`, `1406`, or another family

Codex should not:

- assume Takebishi is using the same command family the repository uses
- assume a successful OPC read means a specific family was used
- treat helper behavior as evidence for native family behavior

## Copy/Paste Prompt

Use this prompt as the starting point for Codex:

```text
Takebishi OPC Server を使って MC protocol の実通信 capture を取りたい。

前提:
- UART は直列実行。並列アクセスしない
- 1回の capture では 1 仮説だけ見る
- unrelated な tag/group/polling は止める
- `cap/takebishi_probe_tags.csv` を probe 用 tag pool として使う

今回見たい仮説:
- [ここに 0403 / 1402 words / 1402 bits / 0406 / 1406 のどれか1つを書く]

今回有効にする tag:
- [ここに tag 名を列挙する]

今回の操作:
- [read なのか write なのか、同時アクセスなのかを明記する]

やってほしいこと:
1. この tag の組み合わせで狙っている command family が妥当か確認
2. 混ぜるべきでない tag が入っていたら指摘
3. capture が取れたら、この repo の実装とどう比較すべきか整理
4. 必要なら次の最小差分シナリオを1つだけ提案

制約:
- 推測だけで family を断定しない
- takebishi 固有最適化の可能性を残す
- helper と native を混同しない
```

## Example Prompts

Example: `0403` sparse word read

```text
Takebishi OPC Server の capture で 0403 random-read を狙いたい。

有効にする tag:
- D100
- D105

操作:
- この2 tag を同時 read

見てほしいこと:
1. この構成で 0403 を狙うのが妥当か
2. 0401 に落ちやすい要因があるか
3. capture 後に、この repo の `random-read D100 D105` とどう比較すべきか
```

Example: `1406` mixed multi-block

```text
Takebishi OPC Server の capture で 1406 multi-block write を狙いたい。

有効にする tag:
- D100
- D101
- D110
- D111
- D112
- M100-M115
- M200-M231

操作:
- 上の tag に対して同時 write

見てほしいこと:
1. この構成で 1406 に寄るか
2. 0401/1401 の単純連続 write に崩れそうな点があるか
3. capture 後に block count と bit packing をどう見るべきか
```

## Notes

- On FX5U, this repository already confirmed native `0406/1406` after binary fixes.
- The remaining high-value unresolved families are `0403`, `1402`, and `0801/0802`.
- `0801/0802` is usually a poor first capture target because ordinary OPC polling often does not
  choose monitor commands.
