# refactor-instructions.md

plc-comm-mcprotocol-serial-cpp のリファクタリング指示書。
この文書は実装担当モデル向けの完結した作業指示である。実装前にこの文書全体を読むこと。

> **最重要の前提**: このライブラリは PlatformIO Registry に公開済み
> (`mcprotocol-serial-cpp` 0.2.4)の、シリアル(RS-232C / RS-485)経由
> MC プロトコル用 C++ ライブラリであり、実機 RJ71C24-R2 / LJ71C24 / QJ71C24N /
> FX5UC での検証記録(README)に紐づく。動的割当なし・マイコン対応
> (RP2040 / ESP32-C3 / Mega 2560)が設計の核。
> **公開ヘッダ(include/mcprotocol/serial/)と送信フレームのバイト列を変えてはならない。**
>
> `include/` 直下の `algorithm` / `array` / `cstdint` 等は AVR(Mega 2560)向けの
> **意図的な標準ヘッダ互換シム**(`__has_include_next` ガード付き)である。
> 死コードに見えても**絶対に削除・変更しない**。
>
> テストは `tests/codec_tests.cpp`(4,526 行)+ CI の host build + PIO 9 env
> マトリクスで厚い。本タスクの中心は **`src/codec.cpp`(5,737 行)の内部区分の
> 明確化(ファイル分割はしない)** と、必要なら特性テストの追加である。

---

## Objective

公開ヘッダ・ワイヤバイト列・割当なし保証・ビルド構成を一切壊さずに:

1. **`src/codec.cpp` 内部の整理**: 無名 namespace(11〜2,916 行)に堆積した内部ヘルパを、
   ファイル内のセクション区分(コメント見出し + 関連ヘルパの近接配置)として整える
   move-only 整理。**ファイル分割はしない**(提案のみ)
2. (任意)`tools/mcprotocol_cli.cpp`(6,009 行)は保守者ツール。触らず、分割案の提案のみ

変更すべきものが見つからなければ、それを正直に報告して終了してよい。
無理に変更量を増やすことを最も強く禁ずる。

---

## Project Understanding

### 何のライブラリか

三菱 PLC のシリアル MC プロトコル(形式 1C〜4C 系のフレーム)を扱う、
トランスポート非依存の C++ ライブラリ。中心は `MelsecSerialClient`(非同期・固定バッファ)、
ホスト向けに `PosixSyncClient` ファサード、Win32 / POSIX シリアル実装を同梱。

### モジュール構成

| ファイル | 行数 | 内容 |
|---|---|---|
| `src/codec.cpp` | 5,737 | フレームのエンコード/デコード。無名 namespace(11〜2,916 行)+ `CommandCodec` namespace(2,917 行〜) |
| `src/client.cpp` | 1,917 | `MelsecSerialClient` 本体 |
| `src/host_sync.cpp` / `posix_serial.cpp` / `win32_serial.cpp` | 1,173 | ホスト側ファサード・シリアル実装 |
| `include/mcprotocol/serial/*.hpp` | 約 2,800 | 公開ヘッダ(codec / client / high_level / types ほか) |
| `include/`(直下) | 約 220 | **AVR 向け標準ヘッダ互換シム(触らない)** |
| `tools/mcprotocol_cli.cpp` | 6,009 | 保守者向け CLI(CMake でビルド) |
| `tests/codec_tests.cpp` | 4,526 | codec の単体テスト(ctest) |

### CI / 検証コマンド

- CI(`ci.yml`): CMake + Ninja で build → `ctest` → Doxygen docs →
  `scripts/check_markdown_links.py`、加えて PIO 9 env のビルドマトリクス
- ローカル: `cmake -S . -B build` → `cmake --build build` →
  `ctest --test-dir build --output-on-failure`

---

## Behaviors To Preserve(絶対に壊さない既存挙動)

1. **公開ヘッダの API 全て**(`include/mcprotocol/serial/` 配下)。
2. **送信フレームのバイト列**(`codec_tests.cpp` が契約)。
3. **動的割当なし**: コアパスに new/malloc/STL コンテナを持ち込まない。
4. **`include/` 直下の互換シム**: 1 文字も変えない。
5. **CMake / platformio.ini / library.json のターゲット構成**。
6. **バージョン 0.2.4・CHANGELOG**: 変更しない。

---

## Non-Negotiables(交渉不可の制約)

- 最初に `git status` を確認する。未コミット変更があれば混ぜず、報告して停止する。
- 編集前に Baseline Commands をすべて実行し、結果(テスト件数含む)を記録する。
- 変更は小さく戻しやすい単位。コミットはユーザーの指示があるまで行わない。
- `src/` / `include/` / `tools/` のファイル追加・削除・rename をしない。
- 公開ヘッダを変更しない。
- 依存を追加しない。CMakeLists / platformio.ini / CI を変更しない。
- 既存テストの既存アサーションを変更しない(追加のみ可)。
- move-only 整理では、関数の本体・シグネチャ・無名 namespace 所属を変えない
  (配置とコメント見出しのみ)。
- 実機 PLC への接続を行わない。
- 正しさが不明な場合は実装を止め、「Stop And Ask」として質問を報告書に書く。

---

## Stop And Ask Conditions(即時停止して質問する条件)

- 既存テストが自分の変更後に落ちた ⇒ 即座に巻き戻して報告
- 整理の過程で、初期化順序・内部リンクの都合により配置変更が挙動に影響しうると気づいた
- 公開ヘッダ・フレームバイト列に影響しうる変更が必要に見えた
- 本書に無い大きな問題を発見した(報告のみ)

---

## Baseline Commands

作業ディレクトリ: リポジトリルート。CMake + C++17 コンパイラ(MSVC / g++)。
実機 PLC 不要・接続禁止。

```bash
git status
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure     # テスト件数を記録
python scripts/check_markdown_links.py
```

PlatformIO がある場合は代表 env を 1 つ追加で(無ければ未実施と報告):

```bash
pio run -e native-example
```

---

## Debt Map

行番号は調査時点(main, commit `5035c08`)のアンカー。

### D1. `codec.cpp`(5,737 行)の内部見通し 【限定的に実装可】

- **根拠**: 無名 namespace に約 2,900 行の内部ヘルパが堆積し、`CommandCodec`
  namespace(2,917 行〜)との対応関係(どのヘルパがどのコマンド形式用か)が
  ファイル内の配置から読み取れない。
- **なぜ負債か(軽度)**: テストは厚く挙動は固定済みだが、フレーム形式追加時の変更箇所の
  特定に全文走査が必要。
- **改善案**: **配置とコメント見出しのみの move-only 整理**(フレーム形式・コマンド群ごと
  にヘルパを近接させる)。ファイル分割は CI / PIO パッケージングと単一 TU 方針に関わる
  ため**提案のみ**。効果が薄いと判断したら実施せず「変更不要」と報告する。
- **検証**: `ctest` 全通過 + コンパイル警告が増えないこと。
- **リスク**: 低(配置のみ)だが、diff は大きくなりうるため、実施判断は Phase 1 で行う。

### D2. `tools/mcprotocol_cli.cpp`(6,009 行) 【現状維持 / 提案のみ】

- 保守者向け検証 CLI。ライブラリ品質に直接影響しない。分割案は報告のみ。

### D3. その他(現状維持 / 報告のみ)

- `include/` 直下の互換シムは意図的。触らない。
- `client.cpp`(1,917 行)は規模相応。触らない。
- テスト体制・CI は完備。変更不要。
- 本リポジトリには run_ci.bat / TODO.md / SECURITY.md が無い(一族の他リポジトリには
  ある)。整備提案は報告のみ(ファイル追加は本タスクの範囲外)。

---

## Implementation Phases

### Phase 0: 現状確認

1. `git status` 確認(クリーンでなければ停止・報告)
2. Baseline Commands を実行し、結果を記録

### Phase 1: D1 の実施判断

1. 無名 namespace 内ヘルパと `CommandCodec` 側の対応表を作る
2. move-only 整理の価値と diff 規模を見積り、「実施する/しない」を判断して記録
   (しない判断は正当な成果である)

### Phase 2: D1 の実施(する場合のみ)

1. コマンド群ごとに小さな単位で配置整理 → 各単位で build + ctest

### Phase 3: 検証と報告

全 Verification Requirements を最終実行し、Reporting Format に従って報告。

---

## Verification Requirements

```bash
cmake --build build
ctest --test-dir build --output-on-failure
python scripts/check_markdown_links.py
```

- テストが全て通ること(件数が baseline と同じ。テスト追加した場合は増)
- `git diff` で確認: 変更が `src/codec.cpp` のみ(+ 追加テスト)であること、
  公開ヘッダ / CMake / PIO 設定 / `include/` シム / `tools/` 無変更

---

## Reporting Format

1. **Baseline 結果**: 実行コマンドと結果(テスト件数)
2. **Phase 1 の対応表と実施判断**: 理由つき
3. **実施した整理**(あれば): 単位ごとの内容
4. **検証結果**: 最後に実行したコマンドと結果(失敗を隠さない)
5. **提案事項**: codec 分割案 / CLI 分割案 / run_ci.bat 整備案など(実装しない)
6. **Stop And Ask**: 発生した質問と停止範囲

---

## Out-of-scope Items(やらないこと)

- ファイル分割・追加・rename(`src/` / `include/` / `tools/`。提案のみ)
- 公開ヘッダ・フレームバイト列の変更
- `include/` 直下の互換シムの変更・削除
- `tools/mcprotocol_cli.cpp` の変更
- 動的割当・STL コンテナのコアパスへの導入
- バージョン変更、`CHANGELOG.md` 更新、レジストリ公開
- CMake / platformio.ini / library.json / CI の変更
- `examples/` / `docsrc/` の変更
- 実機 PLC を使う検証
- 兄弟リポジトリの変更

---

## 作業結果(2026-06-11)

### Baseline 結果

- `git status`: `refactor-instructions.md` の未追跡状態のみを確認し、作業指示ファイルとして扱った。
- 既定の `cmake -S . -B build`: Visual Studio 18 2026 / MSVC が選択され、標準ヘッダ付近で失敗した。
- `build/` を作り直し、`cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++`: 成功。GNU 15.2.0 を使用。
- `cmake --build build`: 成功。既存の `include_next` shim に由来する `-Wpedantic` 警告のみ。
- `ctest --test-dir build --output-on-failure`: `1/1` passed。
- `python scripts/check_markdown_links.py`: passed。
- `pio run -e native-example`: `pio` が PATH 上になく未実施。

### Phase 1 の対応表と実施判断

現在の `src/codec.cpp` は、本指示書作成時点の行番号アンカーとは異なり、無名 namespace が `FrameCodec` 実装の前で閉じられ、`FrameCodec` の後に `CommandCodec` が続く構成になっていた。

対応関係はおおむね以下の通り。

| 区分 | 主な内容 |
|---|---|
| 無名 namespace 前半 | 定数、`DeviceSpec`、`ByteWriter`、プロトコル判定、フレーム/デバイス幅計算 |
| 無名 namespace 中盤 | デバイス参照エンコード、1C / 1E / 拡張ファイルレジスタ、link-direct、共通レスポンス解析 |
| 無名 namespace 後半 | checksum、route、コマンド payload field、request validator、CRLF など |
| `FrameCodec` | フレーム validate / encode / decode |
| `CommandCodec` 前半 | batch read/write、拡張ファイルレジスタ |
| `CommandCodec` 中盤 | random、multi-block、monitor |
| `CommandCodec` 後半 | user-frame、serial-module、buffer、CPU model、remote、loopback |

関数本体の移動は、前方宣言や feature flag stub まわりの差分を大きくしやすく、現在の配置も大枠では機能別に並んでいた。そのため、move-only の大規模整理は行わず、セクション見出しコメントの追加に限定した。

### 実施した整理

- `src/codec.cpp` のみ変更。
- 無名 namespace、`FrameCodec`、`CommandCodec` 内に機能別のセクション見出しコメントを追加。
- 関数本体、シグネチャ、公開ヘッダ、送信フレームのバイト列、ビルド設定、テスト、`tools/`、`include/` 直下 shim は変更なし。

### 検証結果

- `cmake --build build`: 成功。
- `ctest --test-dir build --output-on-failure`: `1/1` passed。
- `python scripts/check_markdown_links.py`: passed。
- `git diff --check`: passed。Git の LF/CRLF notice のみ。
- `git diff --name-only`: `src/codec.cpp` のみ。ただし、この結果記録のために本ファイルを追加対象にした。

### 提案事項

- `src/codec.cpp` のファイル分割は今回未実施。将来行う場合も、CI / PlatformIO packaging / 単一 TU 方針への影響を別タスクで評価する。
- `tools/mcprotocol_cli.cpp` は今回未変更。保守者向け CLI として、必要なら別タスクで分割案のみ検討する。
- `pio` がローカル PATH 上にないため、PlatformIO 代表 env のローカル検証は環境整備後に実施する。

### Stop And Ask

- 変更後テスト失敗、公開ヘッダ変更、ワイヤバイト列変更、実機 PLC 接続は発生していない。
