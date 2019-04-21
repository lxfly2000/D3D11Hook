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
|文件名|D3D11Hook64.dll|D3D11Hook.dll|
|函数名|HookProc|_HookProc@12|

对于钩子类型，根据我查到的信息理论上应该选择[5]，但实测是只要不是[0]和[1]应该是都可以的。

另外如果你觉得x86的函数名太难看的话可以在代码中加入`#pragma comment(linker,"/Export:HookProc=_HookProc@12")`的选项重新编译，这样就可以指定任何函数名了。

## 与录像/直播软件共同使用时注意
因为这些软件同样也可能用到了Hook，所以在运行本程序时可能会受到这些软件的影响。
* 对于 OBS Studio
  * 因为该Hook可能存在一些问题，在 OBS Studio 启动前运行Hook会导致 OBS Studio 启动时卡住，此时需要到任务管理器中强制终止“get-graphics-offsets64”进程，这时 OBS Studio 主界面才会显示出来，所以建议先启动 OBS Studio，再运行Hook。
  * 如果你想在直播或录像中将Hook在左上角添加的FPS文字也包括进去，需要在OBS的来源→右键游戏捕获→属性中将“捕获第三方（如：steam）覆盖”项目钩选。
* 其他软件暂未测试，不过也推荐先启动录像/直播软件再开启Hook。

# :warning:特别注意:warning:
如果启动的游戏是以管理员启动的，那么DLL加载工具同样需要以管理员启动，否则Hook会没有效果。

## 实测运行情况
* [dxtkwithd2d 测试程序](https://github.com/lxfly2000/dxtkwithd2d)：正常显示
* [Puyo Puyo Tetris](http://puyo.sega.com/tetris/)：正常显示
* 我自己写的一个很简单的 D3D11 程序：可能是程序不够严谨，在启动Hook的时候程序画面异常，无法正常显示原有的内容
* [OBS Studio](https://obsproject.com/)：会在程序部分控件上看到FPS的文字，也算作是正常显示
* [Unity](https://unity3d.com)及Unity引擎制作的游戏：正常显示
* [DxLib](http://dxlib.o.oo7.jp)制作的游戏：正常显示，但是在调整窗口大小后会无法显示

## 制作
[lxfly2000](https://github.com/lxfly2000)

### 其他
本程序是仿照某个N年前的游戏辅助程序做的，我曾经用过这款软件，功能相当不错，能显示的信息比较全面，只是这软件时间较久，在最新的Win10系统上貌似无法使用，仅支持D3D8和D3D9，而且后续版本还要收费（流氓软件），怎么想都不划算，所以才有了这个项目。目前这个程序只是实现了很基础的功能，~~下一步会考虑做OpenGL的Hook。~~[GLHook](https://github.com/lxfly2000/GLHook)制作已完成。
