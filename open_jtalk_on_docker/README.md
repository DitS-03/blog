# OpenJTalkをdockerで使う
#Docker, #OpenJTalk

### バージョン
TBD

### これを読むと...
OpenJTalkをDockerで動作させる手順を知ることができる．

### 動機
[u6kさんのコンテナ](https://hub.docker.com/r/u6kapps/open_jtalk/)が素晴らしかったので，ワンライナーで音声を合成&再生する手順をまとめてみました！

びっくりするくらい低遅延で使えて最高です！

## 環境構築手順
### 事前準備
* Docker
* sox
* Git Bash (Windowsのみ: Powershellではエンコードの問題かうまく動作しなかった)

#### Docker
[公式サイト](https://www.docker.com/get-started)でインストールすればOK．
Windowsの場合はWLS周りのセットアップを追加で要求されることがあります．エラーメッセージなどを読んで随時対応しましょう．

#### sox
soxは音声データをCLIで処理する十徳ナイフ的なツールです．
macであれば`homebrew`([リンク](https://brew.sh/index_ja))，windowsであれば`scoop`([リンク](https://scoop.sh))などでインストールできます．

* brewの場合
```
brew install sox
```

* scoopの場合
```
scoop install sox
```

加えて、Windowsの場合にはsoxの使うAudio Driverを環境変数で設定する必要があります．
windowsの「環境変数の設定」で`AUDIODRIVER`という環境変数を作り、`waveaudio`を設定してください．
(なお，使用可能なAudioDriverは`sox -h`で確認できます)


### 実行コマンド
```
echo こんにちは | docker run -i --rm u6kapps/open_jtalk | sox -t wav -r 48000 -b 16 -c 1 -e s - -d
```

これだけ(1回目はコンテナのインストールがあります)．

### もっと低遅延にしたい場合
上記のコマンドだと、毎回dockerコンテナを立ち上げては削除しているので結構な無駄が発生しています．
そこで、コンテナ自体は事前に立ち上げておき、`docker exec`を使ってコマンドだけを再実行するようにするとより高速に実行されます．

```
# jtalkerという名前で、shellプロセスを立ち上げて放置
docker run -i -d --rm --name jtalker u6kapps/open_jtalk sh

# 起動中のjtalkerで音声合成を行う
echo こんにちは | docker exec -i jtalker /usr/local/bin/entrypoint.sh | sox -t wav -r 48000 -b 16 -c 1 -e s - -d
```

#### 性能比較
上記の2種類のコマンドをtimeコマンドで比較してみました (再生完了までを含む)．
* MacBook Air (Retina, 13-inch, 2020)
  * 1.2 GHz クアッドコアIntel Core i7
  * 16 GB 3733 MHz LPDDR4X
* macOS Big Sur 11.2.2
* fish, version 3.2.2
* Docker version 20.10.5, build 55c4c88
* 合成音声の再生時間: 1.60 s

|             | 1回目  | 2回目  | 3回目  |
|-------------|--------|--------|--------|
| docker run  | 2.50 s | 2.50 s | 2.50 s |
| docker exec | 2.19 s | 2.20 s | 2.18 s |

私の環境では300 msほどの短縮になりました．

### 長文を入力した場合の遅延
せっかくなので「マクベス」の有名な一節「*人生は動く影、所詮は三文役者。色んな悲喜劇に出演し、出番が終われば消えるだけ。*」を読んでもらいました (合成音声の再生時間: 8.13s)．

|          | 1回目  | 2回目  | 3回目  |
|----------|--------|--------|--------|
| 計測結果 | 9.09 s | 9.09 s | 9.09 s |
| 遅延時間 | 0.96 s | 0.96 s | 0.96 s |

### 感想
再生開始までのオーバーヘッドが長文であっても1秒弱の遅延に収まるというのはかなり嬉しいですね！
人が自然に感じる応答遅延は1秒らしく，音声認識等を含めるとその制約を切るのは難しいですが，ちょっとした音声対話システムを構築するには十分すぎる性能ではないでしょうか？
