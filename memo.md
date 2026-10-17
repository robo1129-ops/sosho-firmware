# verB2 ファームウェアリリースノート

## verB からの変更点

### エンコーダの設定変更

今回のアップデートに合わせて、**zmk_encoder** という専用モジュールを新たに作成しました。

#### 旧仕様（verB まで）

以前の双掌では、エンコーダをマウスホイールの回転に割り当てるため、keymapファイルの `behaviors` に以下のような設定を記述していました。

```c
msc_ec_l: sensor_rotation_scroll_L {
    compatible = "zmk,behavior-sensor-rotate-var";
    #sensor-binding-cells = <2>;
    bindings = <&msc>, <&msc>;
    tap-ms = <20>;
};
```

この方式では、1回のスクロール操作に対して **press → release の2イベント**が発生します。  
Xiao nRF52840 Plus に負荷がかかっているとき、稀に release イベントが欠落し、press だけが残ってフリーズする問題がありました。

#### 新仕様（verB2 から）

上記の問題を解消するため、**1イベントでスクロールを完了する**専用モジュール `zmk_encoder` を作成・実装しました。

> 📦 モジュールの詳細はこちら:  
> https://github.com/elekit-official/zmk_encoder

これに伴い、旧 `behaviors` およびそれに関連する記述はすべて削除しています。
