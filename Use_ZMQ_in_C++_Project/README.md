# C++プロジェクトでZMQを使う
#C++, #ZMQ, #cmake, #git

### バージョン
ZMQ: 4.3.3

### これを読むと...
非同期メッセージング通信ライブラリであるZeroMQを，C++プロジェクトに組み込む方法が分かります．

### 動機
その使い勝手の良さの割に情報の少ないZeroMQ．
元がCで書かれたライブラリということもあり，ネットの情報の多くはCかPythonです．
ましてや，C++のZeroMQバインディングは複数あってややこしさに拍車をかけています．
そして残念なことに，ZeroMQの[チュートリアルドキュメント](https://zguide.zeromq.org)はAPIのアップデートに追従できていません．

しかし，そんな三重苦があってもZeroMQの驚くほどシンプルなインタフェースはプロトタイピングにぴったりです．
動画をフレームレートで処理したい場合など，Pythonだけでなくメッセージ指向の通信でC++と連携できたら捗ること請け合い．

そんなこんなで C++でZeroMQを使うにあたって情報収集に意外と苦労したので,記事にまとめた次第です．

## ZeroMQについて
[ZeroMQ](https://zeromq.org)は抽象化された通信ソケットを介して，N対Nの様々な通信モデルを実現可能なライブラリです．
なお，2次情報は古いものが多いので何かあったら[公式APIドキュメント](http://api.zeromq.org/master:_start)を参照しましょう．


ZeroMQはソケット通信を抽象化しているので，接続の際に通信経路とエンドポイントをBind or Connectします．指定する際は `[通信経路]://[エンドポイント]` という形式で書きます．ZeroMQ(4.3.3)は次の通信経路をサポートします．
* inproc (Inter-Thread)
    * `inproc://hoge` (エンドポイントは任意の名前)
* ipc
    * `ipc:///tmp/hoge/0` (エンドポイントはipc用のファイルパス)
* tcp
    * `tcp://*:5555` (Bindする側)
    * `tcp://eth0:4321` (Bindする側)
    * `tcp://127.0.0.1:1234` (Connectする側)
* udp (一部の通信モデルのみ)
* pgm ([Pragmatic General Multicast](https://en.wikipedia.org/wiki/Pragmatic_General_Multicast))

なお，ZeroMQではほとんどで通信モデルのどちらがBindする側になるかを制約しません．ユースケースに合わせて設定しましょう．
加えてZeroMQでは，同じソケットタイプであれば一つのエンドポイントに複数のソケットをBindすることが可能です．特に受信側が複数ある場合，メッセージを「全員が受け取る」か，「排他的に誰か一人が受け取る」かは通信モデルによって変わります．

一方，ZeroMQは次の通信モデルをサポートしており，それぞれに対応したソケットタイプが用意されています．
* Request-reply pattern
    * ZMQ_REQ
    * ZMQ_REP
    * ZMQ_DEALER
    * ZMQ_ROUTER
* Publish-subscribe pattern
    * ZMQ_PUB
    * ZMQ_SUB
    * ZMQ_XPUB
    * ZMQ_XSUB
* Pipeline pattern
    * ZMQ_PUSH
    * ZMQ_PULL
* Exclusive pair pattern  (thread間での1対1通信)
    * ZMQ_PAIR
* Native pattern
    * ZMQ_STREAM
* Client-server pattern (draft)
    * ZMQ_CLIENT
    * ZMQ_SERVER
* Radio-dish pattern (draft)
    * ZMQ_RADIO
    * ZMQ_DISH

名前だけだと分かりづらいので，以下で各通信モデルの特徴を解説．

#### Request-reply pattern
    1. ZMQ_REQ -> ZMQ_REP
    2. ZMQ_REP -> ZMQ_REQ

    という順を守ってメッセージの送受信を行う通信モデル.
    リクエストの送受信とリプライの送受信はそれぞれ独立で，かつ非同期．
    ZMQ_REQのバインド先を束ねた場合，ラウンドロビンで誰か一人が受け取る．<br/>
    ZMQ_ROUTERとZMQ_DERLERを間に挟むことで，通信のルーティングができる．
    ZMQ_ROUTERはZMQ_REQからmultipartメッセージを受け取り，第一部分をPOPして読むことでルーティング先のDEALERを決定する．
    ZMQ_DELAERはZMQ_ROUTERからのメッセージを自分と接続するZMQ_REPに排他的に分配する．多段にすることも可能．

#### Publish-subscribe pattern
    一方的にメッセージを送信する側(ZMQ_PUB)と，一方的にメッセージを受信する側(ZMQ_SUB)に分かれる通信モデル．
    ZMQ_SUBのバインド先を束ねた場合，全てのZMQ_SUBがバインド先に送られてきたメッセージを受け取る．<br/>
    ZMQ_XSUBはZMQ_XPUB側に対して，受信を一時停止することを伝えることができる.

#### Pipeline pattern
    一方的にメッセージを送信する側(ZMQ_PUSH)と，一方的にメッセージを受信する側(ZMQ_PULL)に分かれる通信モデル．<br/>
    ZMQ_PULLのバインド先を束ねた場合，送られてきたメッセージはバインドされているZMQ_PULLのうちのどれか一つにだけ渡される (ラウンドロビン)．

#### Exclusive pair pattern
    ZMQ_PAIR同士を1対1で接続して使う．inprocのみしかサポートしない．<br/>
    他の通信モデルとは異なり，特に制約なく双方向にメッセージを送受信できる．

#### Native pattern
    本来ZeroMQではZeroMQ独自のメッセージフォーマットを用いる ([参考](https://qiita.com/gwappa/items/9677e1ea4adcf2d457f4)) ため，ZeroMQのソケットは基本的にZeroMQ以外のメッセージ (例えばhttp) の送受信に使うことができない．<br\>
    ZMQ_STREAMは，送受信されるメッセージを純粋なtcpメッセージとして取り扱うためのもので，例えばZeroMQを用いてHTTPサーバを立てることが可能 ([参考](https://stackoverflow.com/questions/33114758/))

#### Client-server pattern
    最近追加されたDraft段階の通信モデル．
    Request-reply patternの条件緩和版で，一番最初にZMQ_CLIENTからZMQ_SERVERにメッセージを送る以外は自由なタイミングでメッセージを送信し合うことができるらしい．

#### Radio-dish pattern
    最近追加されたDraft段階の通信モデル．
    Publish-subscribe patternではZMQ_SUB側でメッセージをフィルタリングするが，Radio-dish patternではZMQ_RADIO側が送信するグループを選択し，そのグループに所属しているZMQ_DISHだけがメッセージを受け取る．

このようにZeroMQでは多様な通信経路，通信モデルを利用することができ，しかも後で見るように抽象化され統一されたAPIでこれらを使うことができます．


## C++プロジェクトでのZMQの使用
C++でZeroMQを使う場合，オリジナルのlibzmqとC++ラッパーを導入します．

(参考: [windowsでの導入手順](http://blog.livedoor.jp/tmako123-programming/archives/51046977.html))

### libzmqの導入
Unix系であれば素直にパッケージマネージャを使えば導入できます ( [参考](https://zeromq.org/download/) )．
私の場合はbrewでインストールしました．

### C++ラッパーの導入
C++のラッパーライブラリはいろいろあってややこしい ([参考](https://zeromq.org/languages/cplusplus/))．
* *cppzmq* : ヘッダーオンリーのラッパー. C言語に近いAPIが多い
* *zmqpp* : きちんとオブジェクティブに書かれたライブラリ (微妙にドキュメントが少ない)
* *azmq* : boost ライブラリのASIOインタフェースに寄せたAPI設計がされている (らしい)
* *czmqpp* : minimalでsimpleなAPIを目指しているが，githubの最新が2016年で最近はメンテされていないっぽい

プロトタイピングという意味でおすすめと考えるのは*cppzmq*．ヘッダーオンリーでポータブルで簡単に導入できるのはやはり魅力です．*azmq*はboost自体の導入が大変な印象があるのですが，それを除けば良い候補ではないでしょうか？

しかし，一点だけハマりポイントがありました...

libzmqはbrewでインストールしたからと，[レポジトリ](https://github.com/zeromq/cppzmq)のインストール手順のうち，cppzmqだけを手順通りにビルドしようとすると失敗します．
```
$ git clone https://github.com/zeromq/cppzmq
$ cd cppzmq
$ mkdir build
$ cd buid
$ cmake ..
$ sudo make -j4 install
[ 39%] Built target catch
[ 43%] Linking CXX executable ../bin/unit_tests
Undefined symbols for architecture x86_64:
  "_zmq_msg_group", referenced from:
      zmq::message_t::group() const in message.cpp.o
  "_zmq_msg_routing_id", referenced from:
      zmq::message_t::routing_id() const in message.cpp.o
  "_zmq_msg_set_group", referenced from:
      zmq::message_t::set_group(char const*) in message.cpp.o
  "_zmq_msg_set_routing_id", referenced from:
      zmq::message_t::set_routing_id(unsigned int) in message.cpp.o
ld: symbol(s) not found for architecture x86_64
clang: error: linker command failed with exit code 1 (use -v to see invocation)
make[2]: *** [bin/unit_tests] Error 1
make[1]: *** [tests/CMakeFiles/unit_tests.dir/all] Error 2
make: *** [all] Error 2
```
brewで入れられるlibzmqだと，draft版のAPIが含まれていないらしく，そのAPIに関するテストが通らないみたい．

DRAFT版のAPI込みのlibzmqを自分でビルドすればいいのですが，自分の環境では今度は `fatal error: 'gnutls/gnutls.h' file not found` と怒られる．すでにbrewでgnutlsはインストールしているため `/usr/local/include/gnutls/gnutls.h` は存在しているだけになんだか嫌な感じのするエラーです．．

真面目にやる気がなくなったので，ここはcppzmqもDRAFT版のAPI抜きでビルドすることにしました．

レポジトリ直下のCMakeLists.txtを編集します．オリジナルのこの部分に注目します．
```CmakeLists.txt
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    OPTION (ENABLE_DRAFTS "Build and install draft classes and methods" ON)
else ()
    OPTION (ENABLE_DRAFTS "Build and install draft classes and methods" OFF)
endif ()
if (ENABLE_DRAFTS)
    ADD_DEFINITIONS (-DZMQ_BUILD_DRAFT_API)
    set (pkg_config_defines "-DZMQ_BUILD_DRAFT_API=1")
else (ENABLE_DRAFTS)
    set (pkg_config_defines "")
endif (ENABLE_DRAFTS)
```
どうやら，git管理されているか否かでDRAFT版込みのビルドをするかどうか決めているようです (なんじゃそりゃ)．

ということで，さくっとIF文の条件を変更します
```
if (OFF)
    OPTION (ENABLE_DRAFTS "Build and install draft classes and methods" ON)
else ()
    OPTION (ENABLE_DRAFTS "Build and install draft classes and methods" OFF)
endif ()
if (ENABLE_DRAFTS)
    ADD_DEFINITIONS (-DZMQ_BUILD_DRAFT_API)
    set (pkg_config_defines "-DZMQ_BUILD_DRAFT_API=1")
else (ENABLE_DRAFTS)
    set (pkg_config_defines "")
endif (ENABLE_DRAFTS)
```
これで `$ sudo make -j4 install` が通るようになりました．

> ちなみに `brew install cppzmq` もできるようなのですが，ヘッダファイルが `zmq.hpp` しか追加されず，cppzmqを構成するも一つのヘッダファイルである `zmq_addon.hpp` は導入されないようです．この辺り，詳しいことを知っている方がいたら是非教えて欲しいです．

### 簡単なC++プロジェクトの作成
cppファイル1つをcmakeでビルドする最小構成を試します．
```フォルダ構成
.
| - CMakeLists.txt
| - sample.cpp
```

```CMakeLists.txt
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(cppzmq-sample CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(cppzmq REQUIRED)

add_executable(sample sample.cpp)
target_link_libraries(sample cppzmq)
```

```sample.cpp
#include <zmq.hpp>
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::pub);
    sock.bind("tcp://*:5556");
    while (true) {
        std::cout << "send message..." << std::endl;
        sock.send(zmq::str_buffer("Hello, world"));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
```
特に問題なくビルドできるかと思います．

### 動作確認
ビルドしたアプリを実行してみましょう．これで，C++でメッセージを配信している状態になりました．

動作確認のためにpythonでメッセージを受け取ってみます．
```
$ pip install pyzmq
$ python
>>>> import zmq
>>>> context = zmq.Context()
>>>> socket = context.socket(zmq.SUB)
>>>> socket.connect("tcp://127.0.0.1:5556")
>>>> socket.setsockopt_string(zmq.SUBSCRIBE, "")  # 必ずフィルタを設定する必要がある
>>>> m = socket.recv_string()
>>>> m
'hello world'
```

実際にメッセージを受け取ることができたら成功です．
