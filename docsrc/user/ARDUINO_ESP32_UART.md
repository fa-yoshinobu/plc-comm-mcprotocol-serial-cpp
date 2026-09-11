# Arduino-ESP32 UARTアダプターの使い方

`Esp32UartClient`は、ESP32のUARTでMCプロトコル通信を行うためのクラスです。
単点・連続・ランダム・複数ブロックの読み書きに対応し、同期処理と非同期処理を選べます。
M5ライブラリには依存しません。LCDやボタンはアプリケーション側で扱います。

## 使用環境と設定

Arduino-ESP32とC++17を使用します。
ESP32-S3・ESP32-C3、Arduino-ESP32 2.0.17でビルド確認しています。
Arduino-ESP32 3.xは未検証です。このアダプターはRP2040・AVR・ESP-IDF単独環境では使用できません。

ライブラリに `mcprotocol_serial_arduino_esp32.hpp` が含まれていることを確認してください。
既存のPlatformIO設定に、次の指定を追加します。他の必要なフラグは残してください。

```ini
build_unflags =
    -std=gnu++11
    -std=gnu++14

build_flags =
    -std=gnu++17
    -DMCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES=768
    -DMCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES=768
    -DMCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES=384
```

`-std=gnu++17`を`build_unflags`へ入れると、必要な指定が取り消されます。
バッファ容量の指定は、アプリケーションとライブラリの両方に同じ値を適用してください。

上記は少数点の読み書きに使う設定です。機能を無効にする設定ではありませんが、
一度に扱える点数はフレーム容量によって制限されます。
点数を増やす場合は、フレーム容量と使用するタスクのスタック容量を確認してください。
UARTの受信バッファは、この通信フレーム用バッファとは別です。

## 接続を設定する

UARTの設定は`Esp32UartConfig`、PLCの通信形式は`ProtocolConfig`で指定します。

| UART設定 | 初期値 | 指定内容 |
| --- | --- | --- |
| `baud` | 19200 | 300～115200bps |
| `format` | `SERIAL_8E1` | 7/8データビット、パリティなし/偶数/奇数、ストップ1/2 |
| `rx_pin` / `tx_pin` | -1 | 使用するRX/TXピン。明示指定が必要 |
| `direction` | `External` | 自動方向制御等はExternal、RTSによるRS-485制御はRs485Rts |
| `rts_pin` | -1 | Rs485Rtsでは方向制御ピンを指定。Externalでは-1 |
| `rx_buffer_bytes` | 1024 | UART受信バッファ容量。256～65536バイト |

`Rs485Rts`は、DEと/REをRTSへ接続した半二重RS-485回路向けです。
`External`ではアダプターが方向制御ピンを操作しません。
基板に合ったRS-485/RS-232トランシーバーを使用してください。
CTSフロー制御には対応していません。

以下の例は、STAMPLCのPWR-485とFX5Uの内蔵RS-485を接続する設定です。

| 項目 | 設定 |
| --- | --- |
| UART / ピン | UART1、RX=39、TX=0、DIR=46 |
| 通信速度・データ形式 | 19200bps、8bit、偶数パリティ、ストップ1 |
| PLCプロファイル | `PlcProfile::MelsecIqF` |
| MC伝文 | `ProtocolConfig::c4_binary()`：4Cバイナリ、形式5 |
| サムチェック / 局番 | あり / 0 |

別のボードではピンを変更してください。
PLC側の伝文形式・サムチェック・局番・速度・パリティを一致させます。
バイナリ形式5では8データビットが必要です。

## D100を非同期で定期的に読む

次のプログラムは、D100を符号付き16ビット整数として読み、
読み取り完了から1秒後に次の要求を送ります。
値とエラーはデバッグ用Serialへ出力します。
Serialの接続先はボードとUSB設定に従います。PLC用UART1とは分けてください。

```cpp
#include <Arduino.h>
#include <mcprotocol_serial_arduino_esp32.hpp>

using namespace mcprotocol::serial;

Esp32UartClient plc(1); // UART1をこのクラスが専有
std::int16_t d100 = 0; // 非同期の受信先は完了まで保持する
std::uint32_t nextRead = 0;
bool ready = false;

void onRead(void*, Status status) {
  if (!status.ok()) {
    ready = false;
    Serial.printf("COMM Error: %s (PLC=%04X)\n",
        status.message, static_cast<unsigned>(status.plc_error_code));
    return; // 異常時は通信を停止する
  }
  Serial.printf("D100:%d\n", static_cast<int>(d100));
  nextRead = millis() + 1000;
}

void setup() {
  Serial.begin(115200);

  Esp32UartConfig uart;
  uart.baud = 19200;
  uart.format = SERIAL_8E1;
  uart.rx_pin = 39;
  uart.tx_pin = 0;
  uart.direction = Esp32Direction::Rs485Rts;
  uart.rts_pin = 46;

  const auto protocol = ProtocolConfig::c4_binary(
      PlcProfile::MelsecIqF, SumCheckMode::Enabled,
      RouteConfig{HostStationRoute{}}, TimeoutConfig{3000, 250});

  const Status status = plc.begin(uart, protocol);
  ready = status.ok();
  if (!ready) onRead(nullptr, status);
}

void loop() {
  plc.update(); // 送受信とタイムアウトを進める

  if (ready && !plc.busy() &&
      static_cast<std::int32_t>(millis() - nextRead) >= 0) {
    const Status status =
        plc.async_read_word({DeviceCode::D, 100}, d100, onRead);
    if (!status.ok()) onRead(nullptr, status); // 要求の受付失敗
  }

  // LCD・ボタンなどの更新処理をここで行う
  delay(1);
}
```

`begin()`の成功は初期化の成功であり、PLCと通信できたことを意味しません。
読取要求の完了結果で通信の成功を確認します。
`TimeoutConfig{3000, 250}`は全体の応答期限と受信バイト間の期限をミリ秒で指定します。
全体の期限には物理送信にかかる時間も含みます。

## 同期と非同期の違い

| 方式 | 関数が戻るタイミング | 結果の確認 |
| --- | --- | --- |
| 同期 | 通信完了またはエラー発生後 | 戻り値のStatus |
| 非同期 | 要求を受け付けた直後 | 受付は戻り値、通信完了はコールバックのStatus |

同期関数は応答待ちの間、同じタスクのLCD・ボタン処理を進められません。
画面や入出力を更新し続ける場合は非同期関数を使用します。

非同期処理では次のルールを守ってください。

- `loop()`等から`update()`を継続して呼ぶ。長時間呼ばないと受信処理と期限の判定も遅れる。
- 同時に実行する要求は1件。`busy()`がtrueの間の追加要求はBusyになる。
- 戻り値が失敗なら、その要求の完了コールバックは呼ばれない。
- 受付成功後は、コールバックを指定していれば、成功・エラー・キャンセルのいずれかで1回呼ばれる。
- 受信先とコールバックで参照するデータは、完了またはキャンセル完了まで保持する。
- コールバックは`update()`や`cancel()`を呼ぶタスクで実行される。割り込み処理ではない。
- コールバック内で新しい要求を開始しない。次のloopで開始する。
- 同じアダプターを複数タスクから同時に操作しない。

## 読み書き関数

`address`は`{DeviceCode::D, 100}`のように指定します。
`Span<T>`は配列と要素数を渡すための型です。固定長配列はそのまま渡せます。
関数名が同じでも、読み書き可能なデバイス・点数はPLCと伝文形式によって異なります。

| 同期関数 | データ |
| --- | --- |
| `read_word(address, value)` | uint16_t& または int16_t& |
| `read_words(address, values)` | Span<uint16_t> |
| `read_bit(address, value)` | bool& |
| `read_bits(address, values)` | Span<bool> |
| `write_word(address, value)` | uint16_t |
| `write_words(address, values)` | Span<const uint16_t> |
| `write_bit(address, value)` | BitValue（bool） |
| `write_bits(address, values)` | Span<const BitValue> |

非同期では`async_read_words`、`async_read_bits`、`async_write_words`、
`async_write_bits`を使用し、データの後にコールバックと任意のuserポインターを渡します。
符号付き1ワードには`async_read_word(address, int16_t&, callback, user)`も使えます。
その他の単点操作は、対応する配列版に1要素を渡します。

符号付き読取では、コールバックの前に符号を解釈します。
エラー時は符号付き出力を更新しません。いずれの読取も成功時だけ値を使用してください。

## ランダム・複数ブロック・32ビット値

| 同期関数 | 引数 |
| --- | --- |
| `random_read` | RandomReadRequest、WORD出力、DWORD出力 |
| `random_write_words` | RandomWriteWordItem配列、RandomWriteDWordItem配列 |
| `random_write_bits` | RandomWriteBitItem配列 |
| `multi_block_read` | MultiBlockReadRequest、WORD出力、ビット出力、MultiBlockReadBlockResult出力 |
| `multi_block_write` | MultiBlockWriteRequest |

各関数には`async_`を付けた非同期版があります。
引数の後ろにコールバックと任意のuserポインターを追加します。

ランダムアクセスは離れた番地をまとめて指定します。
WORDとDWORDの結果は別々の配列に、指定順で格納されます。
ビットデバイスのランダム読取はワード単位のビット列として扱い、
ランダムビット読取専用の関数はありません。

複数ブロックは「D100から2点」「D200から3点」などをまとめて指定します。
結果の`data_offset`と`data_count`は、対応するWORDまたはビット出力配列内の位置と数です。
**ビットブロックのpoints=1は16ビットです。出力には16要素が必要です。**

通常のDレジスタのDWORDは連続2ワードを使い、先頭が下位ワードです。
floatはDWORDと同じ32ビット列を`memcpy`で相互変換します。数値キャストは使用しません。

対応外の要求や容量を超える要求はエラーになります。
アダプターが複数要求へ自動分割することはありません。
各方式のコード例は[用途別サンプル](../../examples/esp32_uart_usage/README.md)を参照してください。

## UARTの専有と終了

`Esp32UartClient plc(1)`はUART1を初期化して使用します。
同じポートをSerial1・Modbus・別のアダプターから使用しないでください。
別のドライバーが使用中なら`begin()`はBusyを返します。
ピンもSPI/I2C等と競合しないように割り当てます。

M5StamPLCのUART1を使う場合は、公式ライブラリのModbusスレーブ機能を無効にしてから
M5StamPLCを初期化してください。LCDを含む設定例は
[async_lcd.cpp](../../examples/esp32_uart_usage/async_lcd.cpp)にあります。

- `cancel()`：実行中の要求をキャンセルする。送信中なら物理送信も停止する。
- `end()`：待機中のUARTを閉じる。実行中はBusyを返すため、先にcancelする。
- `configure(protocol)`：待機中で正常なUARTの通信形式を変更する。復旧要求は解除しない。

受信先やコールバック用データを破棄する前に、要求を完了またはキャンセルしてください。
コールバック内でアダプターを破棄しないでください。

## エラーと通信再開

`Status::ok()`で成功を確認します。
`code`はエラー種別、`message`は説明、`plc_error_code`はPLCエラー時の詳細コードです。
書込結果を確定できない場合は`OperationOutcomeUnknown`になることがあります。

`requires_transport_reset()`がtrueなら、そのまま次の要求を送れません。
UARTが閉じている場合もtrueになります。
初期化に失敗した場合は原因を修正して`begin()`を行います。

一度beginに成功したUARTを復旧する場合は、次の順序で操作します。

1. 新しい要求を止め、配線・設定・PLC側の状態を確認する。
2. PLCや通信モジュールの手順に従い、前回の遅延応答が今後届かない状態を確保する。
3. `recover(protocol)`でUARTと通信設定を初期化し直す。
4. 戻り値の成功を確認してから、新しい要求を開始する。

UARTの開き直し、受信バッファの破棄、MCUのリセットだけでは、
線上やPLC側に残る遅延応答を排除できません。一律の待機時間での保証もありません。
アダプターは自動復旧・自動再送を行いません。

書込応答が届かなくても、PLCでは書き込み済みの場合があります。
書込結果が不明な要求をそのまま再送しないでください。
実装例は[ESP32-C3の復旧例](../../examples/platformio_esp32c3_arduino_async_polling_reconnect/README.md)を参照してください。
