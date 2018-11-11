# D3D11Hook
用于 Direct3D 11 游戏的界面覆盖显示，比如可以在画面上显示时间、按键操作、FPS等，当然也可以增加原来的游戏没有的功能如截图、录像、变速之类的。（有点类似外挂的感觉）

## Release
[Release](https://github.com/lxfly2000/D3D11Hook/releases) 中的文件是我在自己机器上编译好的DLL，你可以下载来直接用，也可以下载代码根据你的需要修改功能。但是我不保证DLL能够稳定运行，也许有Bug，毕竟我自己做这东西的时候也是晕晕乎乎的，很多代码都是从网上找来，理解不了的代码发现只要符合我的预期就没深入研究。当然如果你遇到了Bug还是推荐在 [issue](https://github.com/lxfly2000/D3D11Hook/issues) 中告诉我，我会尽力修复的。

## 编译
代码克隆下来后用VS打开，把子模块的C++运行库全部改为“**多线程（调试）**”，即`/MT(d)`，不然VS编译时会出警告，编译后生成DLL文件即为所需的Hook文件，该DLL不需要其他DLL的依赖。

## 使用方法
使用该DLL需要一个DLL加载器（DLL注入工具），网上的加载器我找了一下都没有选择函数的功能，或许那些工具都是用的远程线程注入的方式运行的，因此推荐使用我[另一个项目里的加载器](https://github.com/lxfly2000/hookmidi)，把那个下载来后在对话框里做相应配置就可以用了。如果你要用我编译的那个DLL，请按如下配置使用：

|配置项|x64|x86|
|:-:|:-:|:-:|
|文件名|`D3D11Hook64.dll`|`D3D11Hook.dll`|
|函数名|`HookProc`|`_HookProc@12`|

对于钩子类型，根据我查到的信息理论上应该选择[5]，但实测是只要不是[0]和[1]应该是都可以的。

另外如果你觉得x86的函数名太难看的话可以在代码中加入`#pragma comment(linker,"/Export:HookProc=_HookProc@12")`的选项重新编译，这样就可以指定任何函数名了。

## 制作
[lxfly2000](https://github.com/lxfly2000)

## 更新历史
v1.0 2018-11-11
