# GacUI：XML Resource

GacUI XML Resource大约在2013年左右开始成形，但是最终的功能是在2018年左右才固定下来的。在这个阶段里，GacUI XML Resource一共经历了五个版本，而且每个版本之间的差异还很大。这里的差异主要指的是如何处理XML，而写法上却没什么变。

## 1. 当XML窗口真的是一个资源

早期GacUI是一个纯粹的C++库，所有接口设计的决定都是围绕着如何让C++操作起来更简便而做出的。而GacUI XML Resource最初仅仅是为了免去加载资源的烦恼。毕竟一个应用程序光是图标就有一大堆，一个一个加载进来实在是太麻烦了。所以GacUI XML Resource允许你写一个XML文件当目录，在启动应用程序的时候，所有的资源都会跟随着XML一起进来。这个时候不管是XML还是文件们都是分离的。

但是随着demo的复杂度与日俱增，我发现C++在表达UI的这方面还是有硬伤，总的来说就是代码里面的噪音太多了。在考察了世界上各种XML或者JSON的UI语言之后，我毅然选择了XML。在当时JSON作为一个潮流声势浩大，我还跟很多人都讨论过语言的细节问题，最后还是觉得XML对GacUI最合适。但是开发窗口的资源就面临着一个无法避免的问题：反射。今天[Gaclib的C++对象反射](https://github.com/vczh-libraries/VlppReflection)我认为已经比较成熟了，甚至这个反射库允许你在Workflow脚本里面创建新的类型并对C++的类型做多重继承，反射库还提供了C++代码生成的hint，整个脚本可以完全翻译成C++，那么在运行的时候就不需要反射的支持了。`GacBuild.ps1`正是依赖了这一个功能做出来的。

有了反射，我就可以在运行时用字符串当C++类名来创建控件，可以访问每一个控件的属性，可以序列化和反序列化所有struct、enum和其他值类型等等。最后只需要在加载UI资源的时候做一些调整就好了。这些调整主要来源于C++和XML的用法不同，为了C++优化的接口总会在一些细节上让XML用起来不太舒服。在前面的博客中已经提到了，布局自己是一棵树，而把控件放进布局的时候实际上是把控件控制的那个布局图元子树的根节点放进去，这个区别在XML就抹平了，但是C++操作起来是不一样的。有些属性在C++里面表现为数组，有些表现为列表，这主要是出于performance上的考虑。就像`<Table>`是不鼓励你频繁更改表格结构的，所以它会需要你先告诉表格有多少行列，然后再给行列设置属性。而XML写在那里是不会变的，做这种要求就是无稽之谈了，编译器数一下`Rows`和`Columns`下面各有多少XML tag就好了。

```XML
<Window Text="GacUI">
  <Table AlignmentToParent="left:0 top:0 right:0 bottom:0" BorderVisible="true" CellPadding="5" MinSizeLimitation="LimitToElementAndChildren">
    <att.Rows>
      <CellOption>composeType:Percentage percentage:1.0</CellOption>
      <CellOption>composeType:MinSize</CellOption>
    </att.Rows>
    <att.Columns>
      <CellOption>composeType:Percentage percentage:1.0</CellOption>
      <CellOption>composeType:MinSize</CellOption>
      <CellOption>composeType:MinSize</CellOption>
    </att.Columns>

    <Cell Site="row:0 column:0 columnSpan:3">
      <Label Text="Welcomg to GacUI!"/>
    </Cell>

    <Cell Site="row:1 column:1">
      <Button ref.Name="buttonOK" Text="OK"/>
    </Cell>

    <Cell Site="row:1 column:2">
      <Button ref.Name="buttonCancel" Text="Cancel"/>
    </Cell>
  </Table>
</Window>
```

这就是当时XML的样子，跟今天的XML几乎是没什么区别的，只是很多功能都不存在。这个窗口画了一个两行三列的表格，第一行整行放了个`<Label>`，右下角放了个`<Button>`，窗口变大的时候，按钮永远粘着右下角。窗口变小的时候，如果按钮挤到了那行字的空间，那么GacUI就会阻止你继续把窗口变小。而且表格的每一个元素之间的间隔，还有距离窗口边缘的间隔都保留在5个像素。在支持高DPI窗口之后，对5个像素的解读就是100%缩放下的5像素，改到了200%那就变成10个像素了。字体大小和几何图形的边缘也类似，所以GacUI的程序不管DPI改成什么样子，几乎都是没什么变化的。与GDI渲染器不同的是，Direct2D渲染器的效果并不模糊，因为Direct2D支持浮点尺寸，GDI不行。

这个设计跟localization还有关系，考虑到不同语言的OK和Cancel可能长度还不一样，这种使用`<Table>`的方法会让Cancel的文字变长之后自动把OK挤开。

那如何响应按钮事件呢？在当时只能通过给按钮标记`ref.Name`，运行之后用这个名字从加载后的窗口中查询到这个对象，强制转换成`vl::presentation::controls::GuiButton`，最后再把事件挂上去。

在折腾了好几个月之后，GacUI就第一次实现了从XML加载窗口。但是这种写法的缺点太多了，不仅反射带来exe体积的膨胀，而且UI的属性也只能是初始化的时候写死的，距离MVVM的目标那还是差远了。

## 2. 在XML中添加Workflow脚本的支持，实现数据绑定和事件处理

加上数据绑定，性质就完全不同了。这也是[Workflow脚本语言](https://github.com/vczh-libraries/Workflow)的来源。虽然Workflow必须写完一个模块才可以编译，但是如果在XML里面支持表达式，那只要把没一个表达式都改写成一个函数，多少个表达式就出来多少个模块，那也是可以运行起来了。虽然数据绑定实现起来有点绕，但是总的来说只要注册反射的时候，把属性和事件捆绑在一起就好了。`VlppReflection`提供了这一功能。

数据绑定在XML的语法也很直接。一个属性的赋值可以是`Attribute="Value"`，但是只要加上不同的binder，就可以对值产生不同的解读。譬如说可以在菜单的列表项里面引用同一个XML或者别的XML带的图片文件：`Image-uri="res://Path/To/The/Image.png"`。最简单的数据绑定就是一个Workflow计算出来的一次性答案：`Text-eval="let today = Sys::GetLocalTime() in ($'$(today.year)年$(today.month)月$(today.day)日')"`。当然了，GacUI提供了localization的API可以用，显示年月日不需要真的这么麻烦。如果使用`-bind`，那么GacUI就会知道，你是想实时跟踪这个表达式。

你可以做一个程序，两个文本框输入数字，第三个文本框显示结果。而你只需要这样写：

```XML
<SinglelineTextBox Readonly="true" ref.Name="textBox3" Text-bind="(cast int textBox1.Text) + (cast int textBox2.Text) ?? '请输入整数'">
```

那么只要两个标记为`ref.Name="textBox1"`和`ref.Name="textBox2"`的文本框内容一改，那这个文本框的内容就会跟着改为他们俩的和。如果输入的内容不是数字，那么`cast`表达式会抛异常，然后被`??`操作符接住，显示`请输入数字`。非常智能。不过真的要严格使用MVVM的话，其实抽象的更多一点，把ViewModel的两个属性绑定到文本框上，然后第三个文本框从ViewModel的属性绑定回来，`请输入整数`这个数字还要放在`<LocalizedStrings>`里面支持多国语言的翻译，等等。

在处理这个表达式的时候，因为文本框的`GetText`函数、`SetText`函数和`TextChanged`事件被捆绑到了一起，那么GacUI自然就知道实时跟踪这个表达式需要给两个`TextChanged`都挂上回调函数。在文本框的内容被用户输入的同时，事件会被触发，然后不管调用的是拿一个回调函数，这行代码都会重新运行一遍然后调用`textBox3->SetText`。具体实现的时候由于表达式可以很复杂，所以细节上要比这里说的麻烦很多，具体可以参考[考不上三本也会实现数据绑定（一）](https://zhuanlan.zhihu.com/p/26855349)、[（二）](https://zhuanlan.zhihu.com/p/27111228)、[（三）](https://zhuanlan.zhihu.com/p/63909344)。

而在XML添加回调函数的道理也是差不多的。譬如说上一段的例子里，`buttonOK`点一下就把自己关掉，就可以写成：

```XML
<Window ref.Name="self" ...>
  ...
  <Button ref.Name="buttonOK" Text="OK" ev.Clicked-eval="self.Close();"/>
  ...
</Window>
```

事件处理被我规定为只能写一个语句，所以多个语句就需要使用大括号。而觉得一行写不下去也可以，`<ev.Clicked-eval>`可以单独变成一个tag，然后把代码用`<![CDATA[ ... ]]>`包起来就可以了。

实现到这里，GacUI的加载变得非常慢。除了要根据XML的内容使用反射初始化窗口以外，还需要把XML的每一个表达式和事件回调都编译成单独的Workflow module，然后运行起来。exe体积大也是一个缺点，启动速度慢也是一个缺点，两个缺点加在一起，就可以让很多人打消使用GacUI的念头了。这当然是不行的。

## 3. 把整个XML编译成一个Workflow Assembly

问题要一个一个解决。exe体积大只能不要反射，到了这一步还不可行。而启动速度慢的问题，则可以使用编译与启动分开来解决。而要这么做，那么在运行时一边读XML一边反射的方法就不可行了。那么怎么办呢？

当时做出了一个决定，就是把整个GacUI XML Resource凡是非文件资源的部分都合并成一个单独的Workflow module。通俗一点也就是说，你写一个

```XML
<Window ref.Class="path::to::my::MainWindow">
</Window>
```

那我也就不把XML拆开了，直接翻译成

```Workflow
module path_to_my_MainWindow;
using presentation::control::*;

namespace path
{
    namespace to
    {
        namespace my
        {
            class MainWindow : GuiWindow
            {
                ...
            }
        }
    }
}
```

然后Workflow module支持吧编译后的类型和指令都序列化成二进制，那么只要我把这个二进制放进资源里面，那你把编译后的GacUI XML Resource加载进来，反射出这个`path::to::my::MainWindow`的类然后调用它就可以了。至少这样把编译XML的时间拿掉了，启动速度大幅增加。

但是这样又带来一个问题，那我想用C++给`buttonOK`挂事件怎么办？于是我给GacUI的一些类加上了一个叫做`GuiInstanceRootObject`的类型。这个类型现在还存在，只是功能跟当初完全不同了。主要的想法是这样的，当你创建一个UI资源的时候，你从`<Window>`开始时比较合理的，但是从`<Button>`这样的东西开始就有一点无稽之谈的感觉了。所以我规定了只有`Window`、`CustomControl`、`TabPage`和`XXXTemplate`（当时还不存在）等等这样的类型，才能作为XML的根节点。于是我就多了一个地方，可以把所有标注了`ref.Name`的对象都存进去。那么你仍然可以对着`MainWindow`查询一番`buttonOK`，然后强制转换成`GuiButton`，挂上事件。

## 4. 把Workflow编译成C++，大幅缩小exe体积

于是到了这里，我终于可以做出摆脱反射的重要一步了。就算你不需要在你的应用程序里面编译XML，那还是要把编译后的脚本跑起来，就免不了反射。为了完全脱离反射，那么就连脚本都不能跑。唯一的方法就是把`Workflow`编译成C++了。今天的GacUI，把[这样的XML](https://github.com/vczh-libraries/Release/tree/master/Tutorial/GacUI_HelloWorlds/MVVM/UI)，翻译成了[这样的C++代码](https://github.com/vczh-libraries/Release/tree/master/Tutorial/GacUI_HelloWorlds/MVVM/UI/Source)。细心的读者可能会发现，编译出来的C++代码里仍然包含有注册反射的内容。不过我把它单独分离到一个文件里，就是为了让想用反射的人可以用反射，不想用反射直接当这几个文件不存在就好了。而GacUI源代码的设计，会让你打开`VCZH_DEBUG_NO_REFLECTION`的时候包含了反射的代码就有编译错误，而不打开`VCZH_DEBUG_NO_REFLECTION`的时候不包含反射代码也有编译错误，让你时刻知道自己的程序里到底有没有反射，而不会因为巧合而做出决定。

具体Workflow怎么翻译成C++我就不在这里罗嗦了，Workflow也是一门只有智能指针而没有垃圾收集的语言，所以实际上只要翻译出来的语法和语义都对上就好了，没有什么特别困难的地方。在生成的代码里可能大家会发现我大量调用了`::vl::__vwsn::This`函数。Workflow的语义会让你调用`object.Method`的时候，如果`object`是`null`就当场抛异常，跟`C#`一样。而C++显然是没有这个功能的，所以我只好把这段逻辑放进`::vl::__vwsn::This`函数里。而生成的代码之所以这么长，是因为我在每一个地方都使用了包含所有namespace的全名，咋一看会比较乱。

到了这里，`buttonOK`如何用C++挂事件的事情就得到了完美解决。只要你给`<Window>`写上了`ref.CodeBehind="true"`，然后`ev.Clicked`不要用`-eval`而是直接赋值一个函数名`="buttonOK_Clicked"`，那么GacUI就会单独为这个UI生成一对C++代码。在生成的代码里就有这样的内容：

```C++
void MainWindow::buttonOK_Clicked(::vl::presentation::compositions::GuiGraphicsComposition* sender, ::vl::presentation::compositions::GuiEventArgs* arguments)
{/* USER_CONTENT_BEGIN(::path::to::my::MainWindow) */
}/* USER_CONTENT_END() */
```

在这两行里面的代码是不会被覆盖的，而修改了其他地方的代码则会被覆盖。体验就跟上个世代的经典UI库一样。

既然Workflow module已经变成了C++，那就不存在加载程序的时候执行脚本的这个过程了，跑的全都是C++，自然也不需要反射。而且数据绑定的表达式由于从脚本变成了C++，不仅性能提高了很多，而且崩溃了也能够利用VC++当场调试。不过前提是要熟悉C++代码是怎么生成的，不染可能不知道这段C++代码对应的是脚本里面的什么内容。

## 5. 使用MVVM

到了这一步，MVVM就手到擒来了。在上一段的例子里面，就有一个使用MVVM的HelloWorld程序。声明一个ViewModel，也就是在XML里面用Workflow声明一个interface。生成代码后它会变成一个C++的abstract class。你只要用C++实现它，new出你的窗口的同时交给他，那么剩下的数据绑定的工作都由XML来完成。

GacUI允许的绑定多种多样，树形既可以是一个对象，也可以是一个集合，甚至还可以是一棵树。这使得MVVM的理想可以被完全实现：

- ViewModel包含了UI的逻辑，却不包含UI，可以被单独进行单元测试。
- View既可以是UI也可以是单元测试。
- UI作为一个View，只有薄薄的一层，几乎都用声明式语言来写，引入错误的可能性不高。
- 复杂的UI特效则通过在XML里面使用Workflow来完成。ViewModel可以被mock，所以UI部分也可以单独测试。有利于分工合作。

不过由于缺乏`GacStudio`，想让非专业程序员来开发GacUI的UI目前难度还比较高，这是2.0要做的事情之一。

## 尾声

由于时间久远，记忆都比较模糊了。为了写这个博客的系列，我又重新把以前的代码翻了出来。一开始GacUI只是作为编译器的一个side project出现，代码随便放。后来由于内容逐渐增多，我把它单独拿了出来放在了Codeplex。后来眼看Codeplex就要凉了，所以又把他同步到了Github上。后来发现同步还是有点麻烦，于是干脆挪到Github上开发。后来随着GacUI的内容越来越多，单独一个repo显得有点臃肿，于是干脆开了一个[vczh-libraries organization](https://github.com/vczh-libraries)。现在这个organization里面东西应有尽有，有做了一般的Linux移植`XGac`，有基本已经可以在macOS上跑起来的`iGac`，甚至文档网站`WebsiteSource`和用来index并生成GacUI文档页面的C++编译器前端`Document`都在里面。

GacUI近两年进度比较缓慢，主要就是花了很多时间补充文档，还有开发这个C++编译器上了。现在已经可以在文档网站上阅读GacUI的很多资料了。一开始进目录[看文档](http://www.gaclib.net/doc/current/gacui/components/controls/basic/home.html)。不过文档毕竟不是源代码，具体到一个函数到底多少个参数分别是什么类型指针是裸指针还是智能指针这些细节，就可以在[生成的C++文档](http://www.gaclib.net/doc/current/gacui/ref/VL__PRESENTATION__CONTROLS__GUICONTROL.html)上看到。而如果这样还不满足的话，你甚至可以直接在网站上看到[GacUI的源代码](http://www.gaclib.net/CodeIndexDemo/Gaclib/SourceFiles/GuiBasicControls.h.html#NI$vl::presentation::controls::GuiControl)。这个源代码页面的开发呕心沥血，任何标识符都可以朔源，这还是归功于[花了两年写的C++编译器前端](https://github.com/vczh-libraries/Document)。如果这样还觉得有点难懂，还有[大量的例子](https://github.com/vczh-libraries/Release/tree/master/Tutorial)等你来看。

19年之前是基本没有文档的，而又不少人只通过阅读这些例子就基本掌握了GacUI的使用方法。而且GacUI的设计并没有多浓厚的C++味道，反而有利于很多熟悉其它语言的程序员来使用。甚至还有[macOS的移植](https://github.com/vczh-libraries/iGac)就是两个网友在完全没有文档的前提下，甚至不需要问我多少问题，就把他做出来了。这让我感觉这么多年学习编程并不是浪费的，自己做出来的设计切切实实地做到了程序员友好，也是对我多年来努力的肯定。

