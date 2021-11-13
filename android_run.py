import os
import sys
import subprocess

ANDROID_HOME = os.path.join(os.environ['USERPROFILE'], "AppData\\Local\\Android\\Sdk")
ANDROID_TOOLS_PATH = os.path.join(ANDROID_HOME, "tools")
ANDROID_PLATFORM_TOOLS_PATH = os.path.join(ANDROID_HOME, "platform-tools")
ANDROID_NDK_PATH = os.path.join(ANDROID_HOME, "ndk\\22.1.7171670")
TOOLCHAIN = os.path.join(ANDROID_NDK_PATH, "build\\cmake\\android.toolchain.cmake")
ANDROID_PLATFORM = "16"

NINJA = os.path.join(ANDROID_HOME, "cmake\\3.18.1\\bin\\ninja.exe")
EMULATOR = "emulator.exe"

AVD_NAME = "Pixel_3a_API_26"

def main():
  Log("Start " + __file__)
  args = sys.argv
  if len(args) == 1:
    Error("Usage: python " + __file__ + " [build|run|emulator] [option]")
  if args[1] == "build":
    Build_ninja("x86_64", "Debug")
  elif args[1] == "run":
    Run(args[2])
  elif args[1] == "emulator":
    Emulator()
  else:
    Error("Usage: python " + __file__ + " [build|run|emulator] [option]")

#------------------------------------------------------------
# 外部コマンド実行
def Do(cmd):
    command = " ".join(cmd)
    Log("Run: " + command)
    result = subprocess.call(command, shell=True)
    if result != 0:
        Error(command + "  result=" + str(result))

#------------------------------------------------------------
# エラー出力
def Error(msg):
    Log("Error !!! " + msg)
    os.sys.exit(0)

#------------------------------------------------------------
# ログ出力
def Log(msg):
    print("* " + msg)

#------------------------------------------------------------
# ninjaビルド
#    host       "x86_64" or "arm64-v8a"
#    build_type "Debug" or "Release"
def Build_ninja(host, build_type):
  os.makedirs("build_android", exist_ok=True)
  os.chdir("build_android")
  # cmake generator
  cmd = ["cmake -G \"Ninja\""]
  cmd += ["-DCMAKE_TOOLCHAIN_FILE=" + TOOLCHAIN]
  cmd += ["-DANDROID_ABI=" + host]
  cmd += ["-DANDROID_PLATFORM=" + ANDROID_PLATFORM]
  cmd += ["-DCMAKE_MAKE_PROGRAM=" + NINJA]
  cmd += [".."]
  Do(cmd)
  # cmake build
  cmd = ["cmake --build ."]
  cmd += ["--config " + build_type]
  Do(cmd)

#------------------------------------------------------------
# エミュレータ起動
def Emulator():
  os.environ['ANDROID_HOME'] = ANDROID_HOME
  os.environ['PATH'] = '{};{};{}'.format(ANDROID_TOOLS_PATH, ANDROID_PLATFORM_TOOLS_PATH, os.environ['PATH'])
  os.chdir(ANDROID_TOOLS_PATH)
  cmd = [EMULATOR]
  cmd += ["@" + AVD_NAME]
  #cmd += ["-list-avds"]
  Do(cmd)

#------------------------------------------------------------
# 実行
def Run(program):
  os.chdir("build_android")
  cmd = ["adb root"]
  Do(cmd)
  cmd = ["adb push"]
  cmd += [program]
  cmd += ["/data/local/" + program]
  Do(cmd)
  cmd = ["adb shell chmod 777"]
  cmd += ["/data/local/" + program]
  Do(cmd)
  cmd = ["adb shell"]
  cmd += ["/data/local/" + program]
  Do(cmd)

if __name__ == '__main__':
    main()