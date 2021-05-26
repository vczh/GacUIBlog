# GacUI：一切的开始

大概是十年前，[我第一次做出了代码编辑器的智能提示](http://www.cppblog.com/vczh/archive/2010/11/07/132876.html)。当时对函数式语言的兴趣很浓厚，就琢磨起怎么实现generic和type class。做完了之后就心痒痒要做个编辑器。那个时候还年轻，精力旺盛，写程序的时候激情四射，自己琢磨了个把月，很快也就把功能实现了。当然比起其他IDE那只是个玩具，不过学习编程嘛，就是从挑战这些big clean problem中得到进步的。于是我就在想，既然智能提示都做了，那代码折叠要不要做，重构要不要做，等等等等这些功能，要是自己都试一遍，那该多有趣啊。但是摆在面前的问题，就是没有趁手的UI库可以用。由于读书的时候试图高过游戏开发的关系，我也做过简单的UI库，于是就想要不就试试吧。UI库要是做出来了，再用他写一个该UI库的开发工具，那人生就进入了一个新的阶段。

10年之后，终于搞出了 GacUI ( [gaclib.net](http://gaclib.net) )，具有丰富的控件和排版功能，能在Windows和macOS上跑，换肤无比简单，动画本地化和语言排版手到擒来，还有强大的控件布局、数据绑定以及MVVM的功能等等。虽然梦想还没完全实现，但是这一路走来可不简单。

在这之前不久我就已经在做一个[热身项目](http://www.cppblog.com/vczh/archive/2009/08/20/93951.html)。这个项目主要就是画针对UI的矢量图。当时就在想，开发UI的时候各种布局都要自己写（其实是宇宙第一WPF还没深入了解，那年的CSS也不适合干这个，其他UI库也都很原始），那我能不能把控件的位置和一些属性都写成一个表达式，只要里面的变量内容变了，表达式会自己更新结果然后修改图形，岂不是大大简化了开发？WPF的binding都只能写简单的点点表达式，感觉不太够用。于是就有了这个东西。

想法是很简单的，图元就是一个个的XML标记，标记之间可以互相嵌套。我也可以声明一个template，这个template就是一堆图元，只是留下了一些属性让外面配置，用于动态更新里面的表达式，改变显示的内容。有了template之后，每一个地方都可以使用这些template，给不同的属性。现在看起来，这个想法感觉就像WPF的control template，只是各方面都很原始。

```XML
<irconfig>
  <resources> ... </resources>
  <templates>
    <!-- 创建一个template名字叫做background -->
    <template name="background">
      <!-- 给它增加一个int属性x，这个x就可以在template内部使用 -->
      <property name="x" type="int" default="0"/>
      ...
      <content>
        <something name="client"...>
          <!-- 只要client.width或者x改了，新的结果就会赋值给attribute -->
          <another attribute="(client.width - $x) / 2"/>
          ...
        </something>
      </content>
    </template>

    <template name="button">
      <rectangle ...>
        <!-- 在这里创建一个background的副本 -->
        <instance reference="background">
          <!-- 修改属性x，123也可以是一个表达式 -->
          <setter name="x" value="123"/>
        </instance>
      </rectangle>
    </template>
  </templates>
</irconfig>
```

简单的东西总是很容易就做出来了，虽然后来事实证明，当初还是有点小看了了动态更新表达式结果的算法。我知道有一些基于JavaScript的UI库，他也想做数据绑定，但是无奈JavaScript的编译器却不是集成到工具链里面，于是一个表达式依赖于哪些输入就只能靠猜和轮询，搞得很难看。我出于自我进步而挑战这些问题，又怎么能满足于这种想法呢？

于是整个UI架构的雏形开始在脑子里成型。典型的MVVM，自然就是设计一个界面的时候，他会跟一个interface的示例交互。譬如说，点个按钮就会调用该interface的函数，函数可能又会动到interface的一些属性。界面里面的控件的属性使用到了包含interface的属性的表达式，那么interface的属性一变，界面就会自动更新。当然数据绑定的机制最好是universal的，不仅可以绑定到interface的属性，也可以绑定到别的控件的属性，最好这个表达式可以随便你写，而不只局限于一些简单的操作符什么的。

举个[简单的例子](https://github.com/vczh-libraries/Release/blob/master/Tutorial/GacUI_Xml/Binding_Bind/UI/Resource.xml)。一个算加法的程序，需要三个文本框。前两个文本框的内容变了，第三个文本框马上显示两个数字的和：

![](Images/Binding_Bind.gif)

我们布局之类的干扰，只看这三个文本框的话，那GacUI就是这样完成这个任务的：

```XML
<SinglelineTextBox ref.Name="textBoxA" Text="1"/>

<SinglelineTextBox ref.Name="textBoxB" Text="2"/>

<SinglelineTextBox ref.Name="textBoxC" Readonly="true" Text-bind="(cast int textBoxA.Text) + (cast int textBoxB.Text) ?? 'ERROR'"/>
```

首先我们有文本框textBoxA（初始化1）、textBoxB（初始化2）以及textBoxC。textBoxC的Text属性是由一个动态更新的表达式构成的：

```
(cast int textBoxA.Text) + (cast int textBoxB.Text) ?? 'ERROR'
```

意思非常直白，在textBoxA和textBoxB随便哪个被修改之后，我们尝试把文本框的内容parse成数字，然后相加。如果输入的不是数字，那就输出ERROR。

在GacUI提供的反射机制中，文本框的GetText函数、SetText函数以及TextChanged事件可以被注册到一起成为一个属性。于是编译器就可以轻易得知，为了动态更新这个表达式的结果，textBoxA.TextChanged和textBoxB.TextChanged是需要被挂上callback的。在callback里面调用这个表达式就好了。

当然具体实现起来比这个要复杂的多。譬如说我是不是可以把(cast int textBoxA.Text)给cache起来，这样textBoxB.TextChanged里面我可以少做一次cast，提高程序运行的性能。如果表达式是F(Foo.Bar.Baz.Value)，Bar本身也可能会被修改，Baz本身也可能会被修改。为了监视Value的变化，Foo.BarChanged以及Foo.Bar.BazChanged都要被挂callback。于是在Foo.BarChanged里面，Bar变了Baz肯定就变了，我们要把callback从旧的Foo.Bar.BazChanged以及Foo.Bar.Baz.ValueChanged上断开，挂到新的Foo.Bar.BazChanged以及Foo.Bar.Baz.ValueChanged上面去。为了做到这一点，Foo.Bar和Foo.Bar.Baz天然已经被cache起来了，那读Value表达式的时候当然要从cache的Baz对象上面读。后面还有异常处理、函数调用和其他复杂表达式等等各种问题。比WPF只能写Foo.Bar.Baz.Value那要好用得多了。

如果不绑定到控件属性上，绑定到ViewModel的属性上也是一个道理。ViewModel在窗口初始化的时候传进去，成为了窗口的一个属性。访问ViewModel的属性，其实就是在访问窗口的属性的属性的属性等等等，跟访问textBox并没有什么不一样。

感觉说的已经比较远了。以前一直有人问我为什么要做GacUI，这就是我的解答。我打算再接下来的几篇文章中，好好的说一下GacUI各种丰富的设计细节的由来以及背后的故事，包括：

- 如何做到跨平台
- 如何实现支持多语言的文字渲染
- 控件与皮肤的架构上的演变
- 各种灵活的布局的设计
- XML表达的界面是如何跑起来的，以及这些年架构上的演变
- 脚本里面类似C#的yield和await是如何实现的
- 等等

虽然我鼓励用户只把UI特效放进XML，用户逻辑放进ViewModel便于测试，但是特效本身可能就需要很多代码。因此我给GacUI专门设计了一个跟C++对象交互的脚本语言。虽然这个脚本语言可以依赖反射的特性直接运行，但是为了给用户提供一个去掉反射的选项（反射占用的资源实在太大），我把XML整个编译成了这个脚本，再把这个脚本编译成了C++，于是跑起来的时候没有任何额外负担，Release模式下丝般顺滑，一个界面放一大堆table，table里放几千个cell都感觉不到延迟。

曾经有个人使用GacUI做了一个包含下载界面的程序，下载进度条就是一大堆方块（中年人可能会很熟悉哈哈哈）。我发现用户经常用各种出乎我意料的方法使用GacUI，我就问他这么多东西怎么搞的，是通过D2DElement拿到设备对象自己画的吗？他说不是，就是带了几千个格子的table。我整个人都惊呆了。
