# platform-sample

プラットフォーム依存の処理をサンプル実装する。

- POSIX
- スレッド
- ソケット通信

## Linux

gcc か clang を選択できる。

```bash
> mkdir build_clang
> cd build_clang

> cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/x86_64.gcc.toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=1
> cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/x86_64.gcc.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=1

> cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/x86_64.clang.toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=1
> cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/x86_64.clang.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=1

> make
```

## Android

Android NDKとninjaを使用して、Windowsのコマンドプロンプトでビルドと実行を実施する。

Android Studio の SDK Manager からインストールしたNDKとninjaを使用する。

NDK: ツールチェインの指定で必要。
ninja: プログラムの指定で必要。SDK Manager で CMake をインストールしないといけない。

```bash
# ビルド実行 x86_64 Debug
> python android_run.py build

# エミュレータ起動 Pixel_3a_API_26決め打ち
# 注意：ターミナル占有するので別ターミナルで実行すること
> python android_run.py emulator

# 実行
> python android_run.py run tests/MyThreadTest
> python android_run.py run tests/MySocketTest
```
