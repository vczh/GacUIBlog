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
      <Button ref.Name="buttonOK" Text="OK">
    </Cell>

    <Cell Site="row:1 column:2">
      <Button ref.Name="buttonCancel" Text="Cancel">
    </Cell>
  </Table>
</Window>
```

这就是当时XML的样子，跟今天的XML几乎是没什么区别的，只是很多功能都不存在。这个窗口画了一个两行三列的表格，第一行整行放了个`<Label>`，右下角放了个`<Button>`，窗口变大的时候，按钮永远粘着右下角。窗口变小的时候，如果按钮挤到了那行字的空间，那么GacUI就会阻止你继续把窗口变小。而且表格的每一个元素之间的间隔，还有距离窗口边缘的间隔都保留在5个像素。在支持高DPI窗口之后，对5个像素的解读就是100%缩放下的5像素，改到了200%那就变成10个像素了。字体大小和几何图形的边缘也类似，所以GacUI的程序不管DPI改成什么样子，几乎都是没什么变化的。与GDI渲染器不同的是，Direct2D渲染器的效果并不模糊，因为Direct2D支持浮点尺寸，GDI不行。

这个设计跟localization还有关系，考虑到不同语言的OK和Cancel可能长度还不一样，这种使用`<Table>`的方法会让Cancel的文字变长之后自动把OK挤开。

那如何响应按钮事件呢？在当时只能通过给按钮标记`ref.Name`，运行之后用这个名字从加载后的窗口中查询到这个对象，强制转换成`vl::presentation::controls::GuiButton`，最后再把事件挂上去。

在折腾了好几个月之后，GacUI就第一次实现了从XML加载窗口。但是这种写法的缺点太多了，不仅反射带来exe体积的膨胀，而且UI的属性也只能是初始化的时候写死的，距离MVVM的目标那还是差远了。

## 2. 在XML中添加Workflow脚本的支持，实现数据绑定和事件处理

加上数据绑定，性质就完全不同了。

## 3. 使用XML创建新的类型

## 4. 把整个XML编译成一个Workflow Assembly

## 5. 把Workflow编译成C++，大幅缩小exe体积

## 控件皮肤设计的演变

## 尾声
